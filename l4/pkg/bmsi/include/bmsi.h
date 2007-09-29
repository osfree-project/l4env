/*
 * \brief   L4-specific types for BMSI library.
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

#ifndef __BMSI_H_
#define __BMSI_H_

#include <l4/sys/types.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>

#define DBG_BMSI 0
#define DBG_TEST 1 

/* Max number of PDBuilder instances. */
#define MAX_PDBS 200

/* Max number of ProtectionDomain instances. */
#define MAX_PDOMS  200

/* Max number of IntegrityInterface instances. */
#define MAX_IIFACES 200

/* Max size of the data image. */
#define MAX_IMAGE_DATA_LENGTH 100

/* Max size of the command line string. */
#define MAX_CMDLINE_LENGTH 500

/* Current version of the BMSI. */
static const unsigned int bmsi_major_version = 0;
static const unsigned int bmsi_minor_version = 1;

/* Internal representation of an IntegrityInterface. */
#if 0
typedef struct 
{
    int IIFId;
} IIFace;
#endif

typedef int IIFace;


/* Internal representation of a PDDescriptor. */
typedef struct 
{
    char *Name;
    int Memory;
    char L4Lx;
    char *Params;
} L4PDDescriptor;

/* Internal representation of a ProtectionDomain. */
typedef int L4PD;

/* Internal representation of a PDBuilder. */
typedef int L4PDBuilder;

/* Internal representation of a DomainInitializer. */
typedef char *L4DomainInitializer;

/* Internal representation of a BuilderInitializer. */
typedef char *L4BuilderInitializer;

#if 0
typedef struct BuilderInitializer_struct
{
    int Size;
    char *Data;
} BuilderInitializer_t;

typedef struct DomainInitializer_struct
{
    int Nb;
    char *Name;
} DomainInitializer_t;
#endif 


/* Inline function for finding the l4_threadid of the BMSI server. */
static inline l4_threadid_t get_bmsi_id(void)
{
    /* Find the bmsi server */
    l4_threadid_t id_bmsi = L4_INVALID_ID;
    if(!names_waitfor_name("bmsi.main",&id_bmsi,3000))
    {
        LOG("ERROR: bmsi.main not found!\n");
    }
    
    return id_bmsi;
}


#endif 
