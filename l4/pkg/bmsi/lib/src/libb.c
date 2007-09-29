/*
 * \brief   BMSI builder interface implementation.
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

#include<stdio.h>
#include<stdlib.h>

#include<l4/names/libnames.h>
#include<l4/log/l4log.h>
#include<l4/env/errno.h>

#include<l4/bmsi/bmsi.h>
#include<l4/bmsi/bmsi-oo.h>

#include "bmsi-client.h"

/* Map used to register the IIFace instances. */ 
//IIFace *IIFaces_Map[MAX_IIFACES];
int *IIFaces_Map[MAX_IIFACES];


int
DBG_pdbmap(void);

int
DBG_pdommap(void);

int
DBG_iifacemap(int n);

/* *******************************************
 * Interface: BMSI
 * *******************************************/

int 
BMSI_getVersion(int *Version)
{
    
    /* Find the BMSI server */
    DICE_DECLARE_ENV(env);
    l4_threadid_t id_bmsi = get_bmsi_id();

    /* Ask the BMSI server for the BMSI version. */
    int res = bmsi_getVersion_call(&id_bmsi, Version, &env);

    return res;

}

int BMSI_getBuilder(BuilderInitializer Params, PDBuilder *Builder)
{

    /* Find the BMSI server */
    DICE_DECLARE_ENV(env);
    l4_threadid_t id_bmsi = get_bmsi_id();

    /* Cast the paramaters into simpler types for the BMSI server. */  
    char *BuilderParams = (char *)Params;
    int *PDBId = (int *)Builder;
    
    /* Ask the BMSI server for a PDBuilder. */
    int res = bmsi_getBuilder_call(&id_bmsi, BuilderParams, PDBId,  &env); 
    
    return res;
}


int BMSI_getIntegrityInterface(IntegrityInterface *IntInterface)
{

    /* Register the new IntegrityInterface. */ 
    int i=0;
    while(i<MAX_IIFACES)
    {
        if(!IIFaces_Map[i])
        {
            /* Create a new IntegrityInterface. */
            IIFaces_Map[i] = (int*)malloc(sizeof(int));
            *(IIFaces_Map[i]) = i;

            *IntInterface = (int*)(IIFaces_Map[i]);

            return 0;
        }
        i++;
    }
    
    return -1;
    
}

/* *******************************************
 * Interface: PDBuilder
 * *******************************************/

int
PDBuilder_createPD(PDBuilder Self, DomainInitializer Params, ProtectionDomain *PDomain)
{

    /* Find the BMSI server */
    DICE_DECLARE_ENV(env);
    l4_threadid_t id_bmsi = get_bmsi_id();
   
    /* Cast the paramaters into simpler types for the BMSI server. */  
    char *PDParams = (char *)Params;
    int *PDId = (int *)PDomain;
    
    /* Ask the BMSI server to create the PD. */
    int res = bmsi_createPD_call(&id_bmsi, (int)Self, PDParams, PDId, &env); 

    return res;
}

int
PDBuilder_findPD(PDBuilder Self, int64 LocalId, ProtectionDomain *PDomain)
{

    /* Find the BMSI server */
    DICE_DECLARE_ENV(env);
    l4_threadid_t id_bmsi = get_bmsi_id();
   
    /* Cast the paramaters into simpler types for the BMSI server. */  
    int LocId = (int)LocalId;
    int *PDId = (int *)PDomain;
    
    /* Ask the BMSI server to find the PD. */
    int res = bmsi_findPD_call(&id_bmsi, (int)Self, LocId, PDId, &env); 

    return res;
}

int
PDBuilder_free(PDBuilder Self)
{
    /* Find the BMSI server */
    DICE_DECLARE_ENV(env);
    l4_threadid_t id_bmsi = get_bmsi_id();

    /* Destroy the PDBuilder. */
    int res = bmsi_destroyBuilder_call(&id_bmsi, 
                                       (int)Self,
                                       &env); 

    return res;
}


/* *******************************************
 * Interface: ProtectionDomain
 * *******************************************/

int
ProtectionDomain_setResources(ProtectionDomain Self, Resources Res)
{
    /* Find the BMSI server */
    DICE_DECLARE_ENV(env);
    l4_threadid_t id_bmsi = get_bmsi_id();

    /* Ask the BMSI server to set the resources of the PD. */
    int res = bmsi_setResources_call(&id_bmsi, 
                                     (int)Self,
                                     Res.MemoryMax,
                                     Res.MemoryCurrent,
                                     Res.Priority,
                                     Res.NbrCpu,
                                     &env); 

    return res;
}

int
ProtectionDomain_getResources(ProtectionDomain Self, Resources *Res)
{

    /* Find the BMSI server */
    DICE_DECLARE_ENV(env);
    l4_threadid_t id_bmsi = get_bmsi_id();
    
    /* Set the memory allocation functions (needed by Dice). */
    env.malloc = (dice_malloc_func) malloc;
    env.free   = (dice_free_func) free;

    /* Ask the BMSI server for the resources of the PD. */
    int res = bmsi_getResources_call(&id_bmsi, 
                                     (int)Self,
                                     &Res->MemoryMax,
                                     &Res->MemoryCurrent,
                                     &Res->Priority,
                                     &Res->NbrCpu,
                                     &env); 

    return res;
}

int 
ProtectionDomain_build(ProtectionDomain Self, PDDescriptor Image)
{
    /* Find the BMSI server */
    DICE_DECLARE_ENV(env);
    l4_threadid_t id_bmsi = get_bmsi_id();

    /* Retrieve the L4 parameters from the Image. */
    L4PDDescriptor *Img = (L4PDDescriptor*)Image;
    char *Img_name = Img->Name;
    int Img_mem = Img->Memory;
    char Img_l4lx = Img->L4Lx;
    char *Img_params = Img->Params;

    int res = bmsi_build_call(&id_bmsi, 
                              (int)Self,
                              Img_l4lx,
                              Img_name,
                              Img_mem,
                              Img_params,
                              &env); 

    return res;
} 

int
ProtectionDomain_getStatus(ProtectionDomain Self, DomainStatus *Status)
{

    /* Find the BMSI server */
    DICE_DECLARE_ENV(env);
    l4_threadid_t id_bmsi = get_bmsi_id();

    /* Ask the BMSI server for the status of the PD. */
    int res = bmsi_getStatus_call(&id_bmsi, 
                                  (int)Self,
                                  (int*)Status,
                                  &env); 
 
    return res;
}

static int
ProtectionDomain_setStatus(ProtectionDomain Self, DomainStatus Status)
{
    /* Find the BMSI server */
    DICE_DECLARE_ENV(env);
    l4_threadid_t id_bmsi = get_bmsi_id();

    /* Ask the BMSI server to set the status for the PD. */
    int res = bmsi_setStatus_call(&id_bmsi, 
                                  (int)Self,
                                  (int)Status,
                                  &env); 

    return res;
}

int
ProtectionDomain_run(ProtectionDomain Self)
{
    return ProtectionDomain_setStatus(Self, DomainStatusRunning);
}

int
ProtectionDomain_pause(ProtectionDomain Self)
{
    return ProtectionDomain_setStatus(Self, DomainStatusPaused);
}

int 
ProtectionDomain_destroy(ProtectionDomain Self)
{
    /* Find the BMSI server */
    DICE_DECLARE_ENV(env);
    l4_threadid_t id_bmsi = get_bmsi_id();

    /* Destroy the PD. */
    int res = bmsi_destroy_call(&id_bmsi, 
                                (int)Self,
                                &env); 

    return res;
} 

int
ProtectionDomain_allowConnection(ProtectionDomain Self, ProtectionDomain Dest, boolean Allow)
{

    /* Find the BMSI server */
    DICE_DECLARE_ENV(env);
    l4_threadid_t id_bmsi = get_bmsi_id();
    
    /* Ask the BMSI server to change the communication rights. */
    int res = bmsi_setConnection_call(&id_bmsi, 
                                      (int)Self,
                                      (int)Dest,
                                      (int)Allow,
                                      &env); 
 
    return res;
}

int
ProtectionDomain_getLocalId(ProtectionDomain Self, int64* LocalId)
{

    *LocalId = (int)Self;
    return 0;
}

int
ProtectionDomain_free(ProtectionDomain Self)
{


    return 0;
}

/* *******************************************
 * Debugging Functions
 * *******************************************/

int
DBG_pdbmap(void)
{
    /* Find the BMSI server */
    DICE_DECLARE_ENV(env);
    l4_threadid_t id_bmsi = get_bmsi_id();

    /* Ask the BMSI server to print the PDBMap. */
    int res = bmsi_printPDBMap_call(&id_bmsi, &env);

    return res;
}

int
DBG_pdommap(void)
{
    /* Find the BMSI server */
    DICE_DECLARE_ENV(env);
    l4_threadid_t id_bmsi = get_bmsi_id();

    /* Ask the BMSI server to print the PDommap. */
    int res = bmsi_printPDommap_call(&id_bmsi, &env);

    return res;
}

int
DBG_iifacemap(int n)
{

    /* Print the IIFaces_Map. */
    LOG("***** IIFACES_MAP START [%d]", n);

    int i=0;
    while(i<MAX_IIFACES)
    {
        if(IIFaces_Map[i]!=NULL)
        {

            LOG("map[%d]: iiface(%d), ref(%p) at(%p)", 
                i,
                *(IIFaces_Map[i]),
                IIFaces_Map[i],
                &(IIFaces_Map[i]));
 
        }
        i++;
    }

    LOG("***** IIFACES_MAP END [%d]", n);

    return 0;
}
