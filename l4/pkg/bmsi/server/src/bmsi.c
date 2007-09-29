/*
 * \brief   BMSI server implementation
 * \date    2007-07-14
 * \author  Christelle Braun <cbraun@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007 Christelle Braun <cbraun@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the BMSI package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <stdio.h>
#include <malloc.h>

#include <l4/dm_mem/dm_mem.h>
#include <l4/names/libnames.h>
#include <l4/loader/loader.h>
#include <l4/loader/loader-client.h>
#include <l4/generic_ts/generic_ts.h>
#include <l4/ipcmon/ipcmon.h>
#include <l4/log/l4log.h>
#include <l4/env/errno.h>
#include <l4/lyon/lyon.h>
#include <l4/util/base64.h>

#include <l4/bmsi/bmsi.h>
#include <l4/crypto/sha1.h>

#include "bmsi-server.h"
#include "bmsi-local.h"

/* Map used to register the PDBuilder instances. */ 
static IPDBuilder *PDBs_Map[MAX_PDBS];

/* Map used to register the PD instances. */
static IPDomain *PDoms_Map[MAX_PDOMS];

/* The thread IDs of LOADER and DM_PHYS. */
static l4_threadid_t loader_id = L4_INVALID_ID;
static l4_threadid_t dm_phys_id = L4_INVALID_ID;


int
bmsi_build_component (CORBA_Object _dice_corba_obj,
                      int pdid,
                      char L4Lx,
                      const char *Name,
                      int Mem,
                      const char *Params,
                      CORBA_Server_Environment *_dice_corba_env)
{
    /*
     * This function does the following:
     *   1. Create the loader config script.
     *   2. Determine thread IDs of loader and file provider.
     *   3. Copy the created loader config script into a new dataspace and
     *      transfer ownership of this dataspace to the loader.
     *   4. Call loader and let it start the PD as described in the created
     *      loader config script.
     */

    /* Check that this PD has already been created. */
    if(!(pdid>=0 && pdid<MAX_PDOMS && PDoms_Map[pdid]!=NULL))
    {
        LOG("ERROR: PD(%d) has not been registered yet !",pdid); 
        return -L4_EINVAL;
    }
    else
        LOGd(DBG_BMSI, "PD(%d) was registered !",pdid); 

    /* 
     * Compute the string corresponding to the loader config file that will 
     * be written in the dataspace.
     */
    char pd_cfg[4096];
    char tmp_str[3072];

    /* Check whether a parent lyon_id is available. Otherwise, it is lyon_nil_id. */
#if 0
    lyon_id_t ParId;
    memcpy(ParId, lyon_nil_id, sizeof(lyon_nil_id));

    if(PDoms_Map[pdid] && PDoms_Map[pdid]->LyonParId && PDoms_Map[pdid]->LyonParId!=lyon_nil_id)
    {
        memcpy(ParId, PDoms_Map[pdid]->LyonParId, sizeof(PDoms_Map[pdid]->LyonParId));
    }
#endif

    /* Compute the lyon_id of the created task. It is encoded in base64. */
    crypto_sha1_ctx_t sha1_ctx;
    lyon_id_t ChildId;
    char *ChildId64;

    sha1_digest_setup(&sha1_ctx);
    sha1_digest_update(&sha1_ctx, Name, strlen(Name));
    sha1_digest_update(&sha1_ctx, PDoms_Map[pdid]->Params, strlen(PDoms_Map[pdid]->Params));
    sha1_digest_final(&sha1_ctx, ChildId);
    base64_encode(ChildId, sizeof(ChildId), &ChildId64); 


    int n;

    /* Create first part of the loader config script (special version for L4Linux PDs). */
    if(L4Lx)
    {
        n = snprintf(tmp_str, sizeof(tmp_str),
                     "task \"%s\" \"%s no-scroll earlyprintk=yes mem=%d pdid=%d\""
                     " all_sects_writable\n",
                     Name, Params, Mem, pdid);
    }
    else if (strlen(Params)>0)
        n = snprintf(tmp_str, sizeof(tmp_str), "task \"%s\" \"%s\"\n", Name, Params);
    else
        n = snprintf(tmp_str, sizeof(tmp_str), "task \"%s\"\n", Name);

    if(n>sizeof(tmp_str))
    {
        LOG("ERROR: binary/param string too long.");
        return -1;
    }

    /* Finalize loader config script. */
    n = snprintf(pd_cfg, sizeof(pd_cfg),
                "%s"
                " priority 0x%x\n"
                " integrity_id \"%s\"\n"
                " hash_modules\n"
                " cap_handler \"ipcmon\"\n"
                " allow_ipc \"names\"\n"
                " allow_ipc \"LOADER\"\n"
                " allow_ipc \"DM_PHYS\"\n"
                " allow_ipc \"stdlogV05\"\n"
                " allow_ipc \"SIMPLE_TS\"\n",
                tmp_str, PDoms_Map[pdid]->Priority, ChildId64);

    if(n>sizeof(pd_cfg))
    {
        LOG("ERROR: Not enough space for loader config file.");
        return -1;
    }

    
    /* Find the file provider (usually called something like "BMODFS") */
    l4_threadid_t fp_id = L4_INVALID_ID;
    char *fp_name = PDBs_Map[PDoms_Map[pdid]->PDBId]->Params;
    if (!names_waitfor_name(fp_name, &fp_id, 3000))
    {
        LOG("ERROR: File provider %s not found!\n", fp_name);
        return -1;
    }

    /* Create a new dataspace where the config parameters 
     * (i.e. the string for the loader config file)
     * for the PD will be stored 
     * */
    l4_size_t ds_size = 0x1000;
    l4_addr_t ds_align = 0;
    l4_uint32_t ds_flags = 0;

    l4dm_dataspace_t ds = L4DM_INVALID_DATASPACE;

    int ds_create = l4dm_mem_open(dm_phys_id,
                                  ds_size,
                                  ds_align,
                                  ds_flags,
                                  "ds_bmsi",
                                  &ds);


    if(l4dm_dataspace_equal(ds,L4DM_INVALID_DATASPACE))
    {
        LOG("ERROR: Invalid dataspace after creation. Error %d\n", ds_create);
        return -1;
    }
    else
        LOGd(DBG_BMSI, "Valid dataspace created.\n");

    /* 
     * Attach the dataspace to the BMSI server
     */
    l4_offs_t ds_offset = 0;
    char* ds_addr = 0;

    int ds_attach = l4rm_attach(&ds,
                                ds_size,
                                ds_offset,
                                ds_flags,
                                (void**)&ds_addr);

    if(ds_attach<0)
    {
        LOG("ERROR: Failed to attach dataspace. Error %d\n", ds_attach);
        l4dm_close(&ds);
        return -1;
    }

    /* 
     * Write the config string for the PD at the dataspace 
     * located at ds_addr 
     */
    strcpy(ds_addr,pd_cfg);

    /* Detach from out address space. */
    l4rm_detach(ds_addr);
    
    /* Grant RW rights for the dataspace to the loader. */
    int res_grant = l4dm_share(&ds,
                               loader_id,
                               L4DM_RW);
    if (res_grant<0)
    {
        LOG("ERROR: Failed to share dataspace. Error %d\n", res_grant);
        l4dm_close(&ds);
        return -1;
    }

    /* Transfer ownership to loader. */
    int res_transfer = l4dm_transfer(&ds,
                                     loader_id);
    if (res_transfer<0)
    {
        LOG("ERROR: Failed to tranfer ownership of dataspace. Error %d\n", res_transfer);
        l4dm_close(&ds);
        return -1;
    }

                          
    /* Give the ds start address to the loader. */
    DICE_DECLARE_ENV(env);
    const char* task_name = "taskname";
    static char error_msg[1024];
    char* ptr = error_msg;
    
    static l4_taskid_t task_ids[l4loader_MAX_TASK_ID];

    int i=0;
    for(i=0; i<l4loader_MAX_TASK_ID; i++)
    {
        task_ids[i] = L4_INVALID_ID;
    }

    int res_open = l4loader_app_open_call(&loader_id,
                                          &ds,
                                          task_name,
                                          &fp_id,
                                          L4LOADER_STOP,
                                          task_ids,   
                                          &ptr,       //error_msg
                                          &env);
    if(res_open)
    {
        LOG("ERROR: Asking loader for task creation failed!");
        return -1;
    }

    /* Check that the new task has been created. */
    if(l4_is_invalid_id(task_ids[0]))
    {
        LOG("ERROR: Task creation did not succeed!");
        return -1;
    }
    else
    {
        LOGd(DBG_BMSI, "Valid task (%u.%u) created.\n",
             task_ids[0].id.task,
             task_ids[0].id.lthread);
    }
    
    /* Set the status of the PD. */
    PDoms_Map[pdid]->Status = DomainStatusAllocated;

    /* Register the l4_threadid_t of this PD. */ 
    PDoms_Map[pdid]->L4Id = task_ids[0];
    
    return 0;
}


int
bmsi_setStatus_component (CORBA_Object _dice_corba_obj,
                          int Self,
                          int Status,
                          CORBA_Server_Environment *_dice_corba_env)
{

    if(Self>=0 && Self<MAX_PDOMS && PDoms_Map[Self]!=NULL)
    {
        if(Status==DomainStatusRunning && PDoms_Map[Self]->Status==DomainStatusAllocated)
        {
            DICE_DECLARE_ENV(env);
            l4_taskid_t task_id = l4_get_taskid(PDoms_Map[Self]->L4Id);
            int res_cont = l4loader_app_cont_call(&loader_id,
                                                  &task_id,
                                                  &env);
            if(res_cont)
            {
                LOG("ERROR: Failed to start previously created task!");
                return -1;
            }
        }
        PDoms_Map[Self]->Status = Status;
        return 0;
    }

    LOG("ERROR: The PD(%d) was not found!", Self);

    return -1;
}

int
bmsi_destroy_component (CORBA_Object _dice_corba_obj,
                        int Self,
                        CORBA_Server_Environment *_dice_corba_env)
{

    if(Self>=0 && Self<MAX_PDOMS && PDoms_Map[Self]!=NULL)
    {

        /* Destroy the task. */
        l4_threadid_t tokill = PDoms_Map[Self]->L4Id;

        LOGd(DBG_BMSI, "Destroying task(%u.%u) - pdid(%d)",
             tokill.id.task,
             tokill.id.lthread,
             Self); 

        int res = l4ts_kill_task_recursive(tokill);

        /* Delete the entry in the list of PDs. */
        free(PDoms_Map[Self]->Params);
		free(PDoms_Map[Self]);
        PDoms_Map[Self] = NULL; 
        return res;
    }

    LOG("ERROR: The PD(%d) was not found!", Self);

    return -1;
}

int
bmsi_getStatus_component (CORBA_Object _dice_corba_obj,
                          int Self,
                          int *Status,
                          CORBA_Server_Environment *_dice_corba_env)
{

    if(Self>=0 && Self<MAX_PDOMS && PDoms_Map[Self]!=NULL)
    {
        *Status = PDoms_Map[Self]->Status;
        return 0;
    }

    LOG("ERROR: The PD(%d) was not found!", Self);

    return -1;

}

int
bmsi_setConnection_component (CORBA_Object _dice_corba_obj,
                              int Self,
                              int Dest,
                              int Allow,
                              CORBA_Server_Environment *_dice_corba_env)
{

    /* Look for the id of ipcmon.*/ 
    l4_threadid_t ipcmon_id = L4_INVALID_ID;
    if(!names_waitfor_name("ipcmon", &ipcmon_id, 3000))
    {
        LOG("ERROR: ipcmon not found!");
    }
    
    l4_threadid_t src = L4_INVALID_ID;
    l4_threadid_t dest = L4_INVALID_ID;

    int res_con = -1;

    if(Self>=0 && Self<MAX_PDOMS && PDoms_Map[Self]!=NULL)
    {
        src = PDoms_Map[Self]->L4Id;
        
        if(Dest>=0 && Dest<MAX_PDOMS && PDoms_Map[Dest]!=NULL)
        {
            dest = PDoms_Map[Dest]->L4Id; 

            /* Allow the communication Self->Dest. */
            if(Allow)
            {
                LOGd(DBG_BMSI, "Now allowing comm rights (%u.%u) -> (%u.%u).", 
                    src.id.task, src.id.lthread,
                    dest.id.task, dest.id.lthread);
                
                res_con = l4ipcmon_allow(ipcmon_id, src, dest);
            }

            /* Deny the communication Self->Dest. */
            else
            {
                LOGd(DBG_BMSI, "Now revoking comm rights (%u.%u) -> (%u.%u).", 
                    src.id.task, src.id.lthread,
                    dest.id.task, dest.id.lthread);
                
                res_con = l4ipcmon_deny(ipcmon_id, src, dest);
            }
        }
    }

    return res_con;
}



int
bmsi_setResources_component (CORBA_Object _dice_corba_obj,
                             int Self,
                             long MemoryMax,
                             long MemoryCurrent,
                             int Priority,
                             unsigned char NbrCpu,
                             CORBA_Server_Environment *_dice_corba_env)
{

    if(Self>=0 && Self<MAX_PDOMS && PDoms_Map[Self]!=NULL)
    {
        PDoms_Map[Self]->MemoryMax = MemoryMax;
        PDoms_Map[Self]->MemoryCurrent = MemoryCurrent;
        PDoms_Map[Self]->Priority = Priority;
        PDoms_Map[Self]->NbrCpu = NbrCpu;
        PDoms_Map[Self]->Status = DomainStatusAllocated;
        return 0;
    }

    LOG("ERROR: The PD(%d) was not found!", Self);

    return -1;
}

int
bmsi_getResources_component (CORBA_Object _dice_corba_obj,
                             int Self,
                             long *MemoryMax,
                             long *MemoryCurrent,
                             int *Priority,
                             unsigned char* NbrCpu,
                             CORBA_Server_Environment *_dice_corba_env)
{
    if(Self>=0 && Self<MAX_PDOMS && PDoms_Map[Self]!=NULL)
    {
        *MemoryMax = PDoms_Map[Self]->MemoryMax;
        *MemoryCurrent = PDoms_Map[Self]->MemoryCurrent;
        *Priority = PDoms_Map[Self]->Priority;
        *NbrCpu = PDoms_Map[Self]->NbrCpu;
        return 0;
    }

    LOG("ERROR: The PD(%d) was not found!", Self);

    return -1;

}



int
bmsi_createPD_component (CORBA_Object _dice_corba_obj,
                         int Self,
                         const char *Params,
                         int *PDomId,
                         CORBA_Server_Environment *_dice_corba_env)
{

    /* Find the L4Id of the parent. */
    l4_threadid_t *L4ParId = (l4_threadid_t*) _dice_corba_obj; 

    if(!(Self>=0 && Self<MAX_PDBS && PDBs_Map[Self]!=NULL))
    {
        LOG("ERROR: PDBuilder(%d) not found.", Self);
        return -1;
    }

    /* Register the created PD in PDom_Map. */ 
    int i=0;
    while(i<MAX_PDOMS)
    {
        if(!PDoms_Map[i])
        {
            /* Create a new PDEntry. */
            //IPDomain *ipdom = (IPDomain*)malloc(sizeof(IPDomain));
            PDoms_Map[i] = (IPDomain*)malloc(sizeof(IPDomain));
            PDoms_Map[i]->PDBId = Self;
            PDoms_Map[i]->PDomId = i;
            PDoms_Map[i]->Status = DomainStatusCreated;
            PDoms_Map[i]->Params = strdup(Params);
            PDoms_Map[i]->L4ParId = *L4ParId;
            //PDoms_Map[i] = ipdom;
            
            *PDomId = i;

            LOGd(DBG_BMSI, "Created PD(%d) for Self(%d) with Params(%s) from Parent(%u.%u)", 
                i, Self, Params, (L4ParId->id).task, (L4ParId->id).lthread); 
            return 0;
        }
        i++;
    }

    LOG("ERROR: Max number(%d) of PDs has already been reached!", 
        MAX_PDOMS);

    return -1;
}


int
bmsi_findPD_component (CORBA_Object _dice_corba_obj,
                         int Self,
                         int LocId,
                         int *PDomId,
                         CORBA_Server_Environment *_dice_corba_env)
{

    if(LocId>=0 && LocId<MAX_PDOMS && PDoms_Map[LocId]!=NULL)
    {
        *PDomId = PDoms_Map[LocId]->PDomId;
        return 0;
    }

    LOG("ERROR: PD with LocId(%d) not found!", 
        LocId);

    return -1;
}


int
bmsi_getVersion_component (CORBA_Object _dice_corba_obj,
                          int *version,  
                          CORBA_Server_Environment *_dice_corba_env)
{
    int v = ( (bmsi_major_version << 16) | bmsi_minor_version ) ;
    *version = v;

    return 0;

}

int
bmsi_getBuilder_component (CORBA_Object _dice_corba_obj,
                           const char *params,
                           int *pdbid,
                           CORBA_Server_Environment *_dice_corba_env)
{
    int i=0;
    while(i<MAX_PDBS)
    {
        if(!PDBs_Map[i])
        {
            /* Create a new IPDBuilder. */
            IPDBuilder *ipdb = (IPDBuilder*)malloc(sizeof(IPDBuilder));
            ipdb->Params = strdup(params);
            ipdb->PDBId = i;
            
            /* Register the new IPDBuilder in the PDBs_Map. */
            PDBs_Map[i] = ipdb;
            
            /* Set the output parameter. */
            *pdbid = i;

            LOGd(DBG_BMSI, "Created PDBuilder(%d) with Params(%s)", i, params); 

            return 0;
        }
        i++;
    }

    LOG("The max number(%d) of existing IPDBuilder has already been reached!", MAX_PDBS);

    return -1;
}

int
bmsi_destroyBuilder_component (CORBA_Object _dice_corba_obj,
                               int Self,
                               CORBA_Server_Environment *_dice_corba_env)
{

    if(Self>=0 && Self<MAX_PDBS && PDBs_Map[Self]!=NULL)
    {

        /* Delete the entry in the list of PDBs. */
     if(PDBs_Map[Self]->Params)
         free(PDBs_Map[Self]->Params);

     if(PDBs_Map[Self]->Data)
         free(PDBs_Map[Self]->Data);

     free(PDBs_Map[Self]);
     PDBs_Map[Self] = NULL; 
        return 0;
    }

    LOG("ERROR: The PDBuilder(%d) was not found!", Self);

    return -1;
}

/* 
 * DEBUG FUNCTIONS
 *
 */

int
bmsi_printPDBMap_component (CORBA_Object _dice_corba_obj,
                            CORBA_Server_Environment *_dice_corba_env)
{

    LOG("***** MAPSTART");

    int i=0;
    while(i<MAX_PDBS && PDBs_Map[i])
    {
        LOG("map[%d]: pdbid(%d) - size(%d) - data(%p)", i, 
            PDBs_Map[i]->PDBId, 
            PDBs_Map[i]->Size,
            PDBs_Map[i]->Data);
        i++;
    }

    LOG("***** MAPEND");


    return 0;
}

int
bmsi_printPDommap_component (CORBA_Object _dice_corba_obj,
                            CORBA_Server_Environment *_dice_corba_env)
{

    LOG("***** DOMMAPSTART");

    int i=0;
    while(i<MAX_PDOMS && PDoms_Map[i])
    {
        LOG("map[%d]: pdbid(%d)-pdomid(%d)-L4Id(%u.%u)-L4ParId(%u.%u)-MemMax(%ld)-MemCur(%ld)-Prio(%d)-NbrCpu(%d)", 
           i,
           PDoms_Map[i]->PDBId,
           PDoms_Map[i]->PDomId,
           (PDoms_Map[i]->L4Id).id.task,
           (PDoms_Map[i]->L4Id).id.lthread,
           (PDoms_Map[i]->L4ParId).id.task,
           (PDoms_Map[i]->L4ParId).id.lthread,
           PDoms_Map[i]->MemoryMax,
           PDoms_Map[i]->MemoryCurrent,
           PDoms_Map[i]->Priority,
           PDoms_Map[i]->NbrCpu);
        
           i++;
    }

    LOG("***** DOMMAPEND");


    return 0;
}



int main(int argc, char **argv)
{
    LOGd(DBG_BMSI, "BMSI server running.\n");
    
    /* Find the loader called "LOADER". */
    if (!names_waitfor_name("LOADER", &loader_id, 3000))
    {
        LOG("ERROR: LOADER not found!\n");
        return -1;
    }

    /* Find the dataspace manager called "DM_PHYS" */
    if (!names_waitfor_name("DM_PHYS", &dm_phys_id, 3000))
    {
        LOG("ERROR: Dataspace manager DM_PHYS not found!\n");
        return -1;
    }

    bmsi_server_loop(0);
  
    return 0;
}
