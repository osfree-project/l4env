/*
 * \brief   BMSI command-line client.
 * \date    2007-09-27
 * \author  Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 * \author  Christelle Braun <cbraun@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007 Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 * Copyright (C) 2007 Christelle Braun <cbraun@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the BMSI package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/* general includes */
#include <stdlib.h>
#include <errno.h>
#include <string.h>

/* L4-specific includes */
#include <l4/bmsi/bmsi-oo.h>
#include <l4/bmsi/bmsi.h>

/*
 * **************************************************************************
 */

#define DBG_TEST 1

/*
 * **************************************************************************
 */

static int parse_number(const char *str, long long *number_out) {

    *number_out = strtoll(str, NULL, 0);
    return (errno == 0) ? 0 : -1;
}

/*
 * **************************************************************************
 */

static int init_builder(char *builder_tag, PDBuilder* builder_out,
                        int *version_out) {

    BuilderInitializer builder_params = builder_tag;
    int res;

    /* Get the BMSI version. */
    res = BMSI_getVersion(version_out);
    LOGd(DBG_TEST, "BMSI_getVersion(): %d (version is %d)", res, *version_out);
    if (res != 0)
        return res;

    /* Create a PDBuilder instance ('Builder'). */
    res = BMSI_getBuilder(builder_params, builder_out);
    LOGd(DBG_TEST, "BMSI_getBuilder(): %d", res);

    return res;
}


static int start_new_pd(PDBuilder builder, char *bin_name,
                        char *bin_args, int is_linux, int mem_size,
                        char *pd_tag, int64 *local_id_out) {

    /* Create a PD instance. */
    DomainInitializer pd_params = pd_tag;
    ProtectionDomain  pd;
    Resources         resources;
    L4PDDescriptor    l4_pd_desc;
    int res;
    
    res = PDBuilder_createPD(builder, pd_params, &pd);
    LOGd(DBG_TEST, "PDBuilder_createPD(): %d", res);
    if (res != 0)
        return res;

    /* Set Resources for PD1. */
    resources.MemoryMax     = mem_size;
    resources.MemoryCurrent = mem_size;
    resources.Priority      = 1;
    resources.NbrCpu        = 1;

    res = ProtectionDomain_setResources(pd, resources);
    LOGd(DBG_TEST, "ProtectionDomain_setResources(): %d\n"
         "MemMax=%ld, MemCur=%ld, Prio=%d, NbCpus=%d",
         res, resources.MemoryMax, resources.MemoryCurrent,
         resources.Priority, resources.NbrCpu);
    if (res != 0)
        return res;

    /* Build PD1. */
    l4_pd_desc.Name   = bin_name;
    l4_pd_desc.Memory = mem_size;
    l4_pd_desc.L4Lx   = is_linux;
    l4_pd_desc.Params = bin_args;

    res = ProtectionDomain_build(pd, &l4_pd_desc);
    LOGd(DBG_TEST, "ProtectionDomain_build(): %d", res);
    if (res != 0)
        return res;

    /* Get LocalID of PD2. */
    res = ProtectionDomain_getLocalId(pd, local_id_out);
    LOGd(DBG_TEST, "ProtectionDomain_getLocalId: %d (local id is %lld)",
         res, *local_id_out);

    return res;
}


static int change_pd_status(PDBuilder builder, int64 local_id,
                            DomainStatus status) {

    PDDescriptor pd;
    int res;

    res = PDBuilder_findPD(builder, local_id, &pd);
    LOGd(DBG_TEST, "PDBuilder_findPD(): %d", res);
    if (res != 0)
        return res;

    switch (status) {
        case DomainStatusRunning:
            res = ProtectionDomain_run(pd);
            LOGd(DBG_TEST, "ProtectionDomain_run(): %d", res);
            break;
        case DomainStatusPaused:
            res = ProtectionDomain_pause(pd);
            LOGd(DBG_TEST, "ProtectionDomain_pause(): %d", res);
        default:
            LOG("Unsupported status %d", status);
            res = -1;
    }

    return res;
}


static int destroy_pd(PDBuilder builder, int64 local_id) {

    PDDescriptor pd;
    int res;

    res = PDBuilder_findPD(builder, local_id, &pd);
    LOGd(DBG_TEST, "PDBuilder_findPD(): %d", res);
    if (res != 0)
        return res;

    res = ProtectionDomain_destroy(pd);
    LOGd(DBG_TEST, "ProtectionDomain_destroy(): %d", res);

    return res;
}


static int probe_pds(PDBuilder builder, int num_pds_to_probe,
                     int64 local_ids_out[]) {

    PDDescriptor pd;
    int i, res, found = 0;

    for (i = 0; i < num_pds_to_probe; i++) {
        res = PDBuilder_findPD(builder, local_ids_out[found], &pd);
        LOGd(DBG_TEST, "PDBuilder_findPD(): %d", res);
        if (res == 0)
            found++;
        else if (0 /* res != BMSI_ErrorNotFound */)
            return res;
    }

    /* terminate list */
    local_ids_out[found] = -1;

    return 0;
}

/*
 * **************************************************************************
 */

/* Print information about the PDs currently registered at the BMSI */
static int print_all_pds(PDBuilder builder) {

    int64 local_ids[32];
    int i, res;

    res = probe_pds(builder, 32, local_ids);
    LOGd(DBG_TEST, "probe_pds(): %d", res);
    if (res != 0)
        return res;

    for (i = 0; local_ids[i] >= 0; i++)
        LOG("found PD %lld", local_ids[i]);

    return 0;
}


static void show_help_and_exit(PDBuilder builder) {

    LOG("Invalid or no command specified!");
    LOG("Usage: bmsic --start|--start-linux <binary_name> <binary_arg_string> <mem_size> <tag>");
    LOG("   or: bmsic --pause <pd_id>");
    LOG("   or: bmsic --run <pd_id>");
    LOG("   or: bmsic --destroy <pd_id>");
    LOG("   or: bmsic --probe-ids");

    if (builder)
        PDBuilder_free(builder);

    exit(1);
}

/*
 * **************************************************************************
 */

int main(int argc, char** argv) {

    PDBuilder builder = NULL;
    char *cmd = argv[1];
    int64 local_id, mem_size;
    int version, res;

    if (argc < 2)
        show_help_and_exit(builder);

    res = init_builder("COPYFS:BMODFS", &builder, &version);
    LOG("init_builder(): %d", res);
    if (res != 0)
        return 1;

    LOG("BMSI version %d", version);

    if (strcmp(cmd, "--start") == 0 || strcmp(cmd, "--start-linux") == 0) {
        if (argc < 6)
            show_help_and_exit(builder);
        if (parse_number(argv[4], &mem_size) != 0)
            show_help_and_exit(builder);
        res = start_new_pd(builder, argv[2], argv[3],
                           strcmp(cmd, "--start-linux") == 0,
                           mem_size, argv[5], &local_id);
        if (res == 0)
            res = change_pd_status(builder, local_id, DomainStatusRunning);
        LOGd(res == 0, "Started new PD %lld", local_id);

    } else if (strcmp(cmd, "--run") == 0) {
        if (argc < 3)
            show_help_and_exit(builder);
        if (parse_number(argv[2], &local_id) != 0)
            show_help_and_exit(builder);
        res = change_pd_status(builder, local_id, DomainStatusRunning);

    } else if (strcmp(cmd, "--pause") == 0) {
        if (argc < 3)
            show_help_and_exit(builder);
        if (parse_number(argv[2], &local_id) != 0)
            show_help_and_exit(builder);
        res = change_pd_status(builder, local_id, DomainStatusPaused);

    } else if (strcmp(cmd, "--destroy") == 0) {
        if (argc < 3)
            show_help_and_exit(builder);
        if (parse_number(argv[2], &local_id) != 0)
            show_help_and_exit(builder);
        res = destroy_pd(builder, local_id);

    } else if (strcmp(cmd, "--probe-pds") == 0) {
        res = print_all_pds(builder);
    }

    return (res == 0) ? 0 : 1;
}
 
