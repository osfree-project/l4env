/*
 * \brief   BMSI internal data structures
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

#ifndef __BMSI_LOCAL_H
#define __BMSI_LOCAL_H

#include <l4/lyon/lyon.h>
#include <l4/bmsi/bmsi-oo.h>

/* Internal representation of a ProtectionDomain instance. */
typedef struct IPDomain_struct 
{
       
     int PDBId;
     int PDomId;
     int Nb;
     char *Name;
     char *Params;
     l4_threadid_t L4Id;
     l4_threadid_t L4ParId;
     lyon_id_t LyonId;
     lyon_id_t LyonParId;
     char Datastr[2048];
     long MemoryMax;
     long MemoryCurrent;
     int Priority;
     int NbrCpu;
     int Status;
     int size_old;
     char data_old[2048];
     l4_threadid_t l4id_old;
     int status_old;



} IPDomain;

/* Internal representation of a PDBuilder instance. */     
typedef struct IPDBuilder_struct 
{
       
     int PDBId;
     l4_threadid_t L4Id;
     char *Params;
     int Size;
     char *Data;
} IPDBuilder ;


#endif 
