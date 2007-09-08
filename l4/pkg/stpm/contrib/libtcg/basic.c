/*
 * \brief   Basic TCG functions.
 * \author  Bernhard Kauer <kauer@tudos.org>
 */
/*
 * Copyright (C) 2006  Bernhard Kauer <kauer@tudos.org>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the STPM package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */       


#include <tcg/tpm.h>
#include <tcg/basic.h>

/**
 * Get a trival property from the tpm.
 */
#define TPM_TRANSMIT_FUNC_GET_PROP(name,prop) \
        TPM_TRANSMIT_FUNC(name, (unsigned long *value), , \
			  if (TPM_EXTRACT_LONG(0)!=4) \
			  return -2; \
			  *value=TPM_EXTRACT_LONG(4); , \
			  "L L L", \
			  TPM_CAP_PROPERTY, \
			  TPM_SUBCAP, \
			  prop);

/**
 * Reset the TPM.
 */
TPM_TRANSMIT_FUNC(Reset, (void), , , "");

/**
 * Full self test.
 */
TPM_TRANSMIT_FUNC(SelfTestFull, (void), , , "");

/**
 * Get the version of the tpm.
 */
TPM_TRANSMIT_FUNC(GetCapability_Version,
		  (int *major, int *minor, int *version,int *rev),
		  ,
		  if (TPM_EXTRACT_LONG(0)!=4)
  		      return -2;
		  *major = (int)(buffer[TCG_DATA_OFFSET+4]);
		  *minor = (int)(buffer[TCG_DATA_OFFSET+5]);
		  *version = (int)(buffer[TCG_DATA_OFFSET+6]);
		  *rev = (int)(buffer[TCG_DATA_OFFSET+7]);
		  ,
		  "L L",
		  TPM_CAP_VERSION,
		  TPM_NO_SUBCAP);


/**
 * Get the number of the available key slots.
 */			  			  
TPM_TRANSMIT_FUNC_GET_PROP(GetCapability_Slots,TPM_CAP_PROP_SLOTS);

/**
 * Get the number of the pcrs.
 */			  			  
TPM_TRANSMIT_FUNC_GET_PROP(GetCapability_Pcrs,TPM_CAP_PROP_PCR);

/**
 * Get all loaded keys.
 */
TPM_TRANSMIT_FUNC(GetCapability_Key_Handle,
		  (unsigned short *num, unsigned long keys[]),
		  ,
                  {
			  int i;
			  *num=TPM_EXTRACT_SHORT(4);
			  for(i=0;i<*num;i++)
				  keys[i]=TPM_EXTRACT_LONG(6+4*i); 
		  },
		  "L L",
		  TPM_CAP_KEY_HANDLE,
		  TPM_NO_SUBCAP);

TPM_TRANSMIT_FUNC(Startup, (unsigned short value), , , "S",value);
TPM_TRANSMIT_FUNC(SaveState, (void), , , "");
TPM_TRANSMIT_FUNC(ForceClear, (void), , , "");
TPM_TRANSMIT_FUNC(PhysicalPresence, (void), , , "o 08", 0);
TPM_TRANSMIT_FUNC(PhysicalEnable, (void), , , "L",0x6F);
