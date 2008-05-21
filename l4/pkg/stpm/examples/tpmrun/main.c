/*
 * \brief   Simple L4Con console for getting in contact with a TPM.
 *          This tool can be used for creating, loading keys
 *          and much more ... simply press 'h' in the console for
 *          an overview.
 * \date    2007-11-01
 * \author  Alexander Boettcher <boettcher@os.inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2007 - 2008
 * Alexander Boettcher <boettcher@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the STPM package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <stdlib.h> //strtol
#include <stdio.h>

#include <l4/log/l4log.h>
#include <l4/l4con/l4contxt.h>

#include <tcg/quote.h>
#include <tcg/rand.h>       //TPM_GetRandom
#include <tcg/owner.h>      //TPM_TakeOwnership 
#include <tcg/pcrs.h>       //TPM_Extend 

#include "tpmrun.h"
#include "encap.h"

static unsigned char srk_auth   [20];   //password of SRK 
static unsigned char owner_auth [20];   //password of owner
static unsigned char anything   [20];   //password of any keys
static unsigned char anything2  [20];   //password of any keys
static unsigned char quote      [1024]; //temp storage for quotes
static unsigned int  quotelen;
static pubkeydata    pubkey;            //temp storage for a pubkey
static unsigned char pcrcomposite [1024];

static const char * fp = "BMODFS";
static const char * file = "keydata.hex";

//#define _LOG_OUTPUT
#ifdef _LOG_OUTPUT
static char log [1024];
#endif

static void show_loaded_keys()
{
  unsigned short max;
  unsigned long keys[256];
  int i;

  if (TPM_GetCapability_Key_Handle(&max, keys)) {
    printf("Couldn't get capability key handle\n");
    return;
  }

  if (max==0)
    printf("No keys are loaded\n");
  else
    for(i=0; i<max; i++)
      printf("Key handle: 0x%04lx\n", keys[i]);
}

static void show_help_info()
{
  printf("h ... this help info\n");
  printf("------- key creation  -------\n");
  printf("c ... create a new RSA key for signing in tmp buffer\n");
  printf("k ... list loaded keys\n");
  printf("------- key loading   -------\n");
  printf("f ... load a key from a file\n");
  printf("l ... load a key to TPM from tmp buffer\n");
  printf("L ... load a key to TPM from tmp buffer, TCGA 1.2\n");
  printf("------- key eviction  -------\n");
  printf("z ... dump key in tmp buffer to LOG\n");
  printf("d ... evict loaded key from TPM\n");
  printf("D ... evict loaded key from TPM, TCGA 1.2 \n");
  printf("------- general       -------\n");
  printf("i ... general information about TPM\n");
  printf("p ... print public key of loaded key\n");
  printf("e ... extend a PCR register\n");
  printf("q ... quote of current pcrs with a loaded key\n");
  printf("r ... generate random numbers\n");
  printf("s ... selftest of TPM\n");
  printf("------- ownership     -------\n");
  printf("a ... set authentication of owner or SRK to be used\n");
  printf("o ... take ownership of TPM\n");
  printf("w ... clear owner of TPM\n");    
  printf("------- miscellaneous -------\n");
  printf("x ... connect to another vTPM\n");    
}

static void show_quote()
{
  int i,j;
  unsigned long value_count;
  unsigned long select_count;
  #ifdef _LOG_OUTPUT
  char * log2;
  #endif

  select_count = ntohs(*(unsigned short *)&pcrcomposite[0]);
  value_count = ntohl(*(unsigned long *)&pcrcomposite[2 + select_count]) / 20;   

  printf("\npcrcomposite select count %d, pcrs %lu:\n",
         ntohs(*(unsigned short *)&pcrcomposite[0]) * 8,
          value_count);

  #ifdef _LOG_OUTPUT
  log2 = log;  
  #endif
  for(i=0; i < value_count; i++)
  {
    printf("%8s%02d: ", "PCR-", i);
    for(j=0;j<20;j++)
    {
      printf("%02x", pcrcomposite[2 + select_count + 4 + i * 20 + j]);
      #ifdef _LOG_OUTPUT
      sprintf(log2, "%02x", pcrcomposite[2 + select_count + 4 + i * 20 + j]);
      log2 += 2;
      #endif
    }
    printf("\n");
  }
  #ifdef _LOG_OUTPUT
  *log2 = 0;
  LOG("%s\n", log);
  #endif
  printf("signature (%d Bytes):\n", quotelen);

  #ifdef _LOG_OUTPUT
  log2 = log;  
  #endif
  for(i=0; i < (quotelen>>4); i++)
  {
    printf("    ");
    for(j=0;j<16;j++)
    {
      printf("%02x", quote[(i<<4)+j]);      
      #ifdef _LOG_OUTPUT
      sprintf(log2, "%02x", quote[(i<<4)+j]);      
      log2 += 2;
      #endif
    }
    printf("\n");
  }
  printf("signature end\n");
  #ifdef _LOG_OUTPUT
  LOG("%s\n", log);
  #endif

}

static unsigned long tpm_check()
{
	int error;
  int major, minor, version, rev;
  unsigned long maxpcrs = 16;
  unsigned long rpcrs;

  printf("Detecting version of TPM ...");
  error = TPM_GetCapability_Version(&major, &minor, &version, &rev);

  if (error)
    printf(" failed (error=%d)\n", error);
  else
  {
    printf(" found version: %d.%d.%d.%d ", major, minor, version, rev);

    if (major == 1 && minor == 1)
      if (version == 0 && rev == 0)
      {
        printf("... TPM specification 1.2\n");
        maxpcrs = 24;
      } else {
        printf("... TPM specification 1.1\n");
      }
    else
      printf("... unknown TPM specification version\n");

  }

  printf("Detecting number of PCRs ...");

  error = TPM_GetCapability_Pcrs(&rpcrs);

  if (error)
    printf(" failed (error=%d). Assume %lu PCR registers.\n", error, maxpcrs);
  else
  {
    maxpcrs = rpcrs;
    printf(" success. %lu PCR registers reported by TPM.\n", maxpcrs);
  }

  return maxpcrs;
}

static void command_loop()
{
  keydata key;
  unsigned long foranything, keyhandle;
  int error;
  int c, i;
  int major, minor, version, rev;
  unsigned long maxpcrs; 
  #ifdef _LOG_OUTPUT
  char * log2;
  #endif

  // find out what kind of TPM we have
  maxpcrs = tpm_check();

  printf("\nWelcome ... press 'h' for a list of supported commands\n");

  while(1)
  {
    printf("\n<cmd>: ");
    do
    {
      c = getchar();
      if (c == 0xd)
        printf("\n<cmd>: ");
    } while (c < 0x20);

    printf("%c\n", c);
    switch (c)
    {
      case 'a':
        printf("o ... change owner auth\n");
        printf("s ... change SRK auth\n");

        do
        {
          c = getchar();
        } while (c < 0x20 && c != 0xd);

        switch(c)
        {
          case 'o':
            memset(owner_auth, 0, sizeof(srk_auth));

            printf("\nowner authentication to be used: ");
            contxt_ihb_read((char *)owner_auth, sizeof(owner_auth), NULL);
            
            sha1(owner_auth, strlen((char *)owner_auth), owner_auth);
            printf("\n");

            break;
          case 's':
            memset(srk_auth, 0, sizeof(srk_auth));

            printf("\nSRK authentication to be used: ");
            contxt_ihb_read((char *)srk_auth, sizeof(srk_auth), NULL);

            sha1(srk_auth, strlen((char *)srk_auth), srk_auth);
            printf("\n");

            break;
        }

        break;
      case 'c':
        memset(anything, 0, sizeof(anything));
        printf("Enter a authentication to be used for new key: ");
        contxt_ihb_read((char *)anything, sizeof(anything), NULL);
        sha1(anything, strlen((char *)anything), anything);

        printf("\nGenerating signing key as child of SRK ... ");

        error = createKey(&key, srk_auth, anything);
       
        if (error != 0)
          printf("failed (error=%d)\n", error);
        else
          printf("success. Key info stored in memory temporarily.\n");

        break;
      case 'z':
        dumpkey(&key);
        printf("success.\n");
        break;
      case 'd':
        memset(anything, 0, sizeof(anything));
        printf("Key to be deleted (hex): 0x");
        contxt_ihb_read((char *)anything, sizeof(anything), NULL);
        printf(" ...");

        error = TPM_EvictKey(strtol((char *)anything, NULL, 16));

        if (error != 0)
          printf(" failed (error=%d)\n", error);
        else
          printf(" success.\n");

        break;
      case 'D':
        memset(anything, 0, sizeof(anything));
        printf("Key to be deleted (hex): 0x");
        contxt_ihb_read((char *)anything, sizeof(anything), NULL);
        printf(" ...");

        error = TPM_FlushSpecific(strtol((char *)anything, NULL, 16), TPM_RT_KEY);

        if (error != 0)
          printf(" failed (error=%d)\n", error);
        else
          printf(" success.\n");

        break;
      case 'e':
        printf("PCR index to be extended: ");
        memset(anything, 0, sizeof(anything));
        contxt_ihb_read((char *)anything, sizeof(anything), NULL);
        i = strtol((char *)anything, NULL, 10);

        printf("\nExtend with : ");
        memset(anything2, 0, sizeof(anything2));
        contxt_ihb_read((char *)anything2, sizeof(anything2), NULL);
 
        sha1(anything2, strlen((char *)anything2), anything2);

        error = TPM_Extend(i, anything2);

        if (error != 0)
          printf(" failed (error=%d)\n", error);
        else
          printf(" success.\n");

        break;
      case 'f':
        printf("Loading key from fileprovider %s, file %s ...", fp, file);

        error = loadkeyfile(fp, file, &key);

        if (error != 0)
          printf(" failed (error=%d)\n", error);
        else
          printf(" success. Key stored in temporary memory.\n");
        
        break;
      case 'l':
        printf("Loading key stored in memory to TPM ...");

        error = TPM_LoadKey(0x40000000, srk_auth, &key, &foranything);
        
        if (error != 0)
          printf(" failed (error=%d)\n", error);
        else
          printf(" success. New keyhandle is: 0x%0lx\n", foranything);
        
        break;
      case 'L':
        printf("Loading key (1.2) stored in memory to TPM ...");

        error = TPM_LoadKey2(0x40000000, srk_auth, &key, &foranything);
        
        if (error != 0)
          printf(" failed (error=%d)\n", error);
        else
          printf(" success. New keyhandle is: 0x%0lx\n", foranything);

        break;  
      case 'h':
        show_help_info();

        break;
      case 'i':
        printf("p ... number of PCRs\n");
        printf("t ... timeout values of function classes\n");
        printf("v ... version of TPM\n");

        do
        {
          c = getchar();
        } while (c < 0x20 && c != 0xd);

        switch(c)
        {
          case 'v':
            printf("Getting version ...");
            error = TPM_GetCapability_Version(&major,&minor,&version,&rev);

            if (error)
              printf(" failed (error=%d)\n", error); 
            else
              printf(" success. TPM version: %d.%d.%d.%d\n",
                     major, minor, version, rev);

            break;
          case 't':
            printf("Getting timout values ...");
            
            unsigned long timeout_a, timeout_b, timeout_c, timeout_d;

            error = TPM_GetCapability_Timeouts(&timeout_a, &timeout_b, &timeout_c, &timeout_d);

            if (error)
              printf(" failed (error=%d)\n", error); 
            else
              printf(" success. Timeout of class A: %lums, B: %lums, C: %lums, D: %lums\n",
                     timeout_a, timeout_b, timeout_c, timeout_d);
            break;
          case 'p':
            printf("Detecting number of PCRs ...");

            error = TPM_GetCapability_Pcrs(&foranything);

            if (error)
              printf(" failed (error=%d). Assume %lu PCR registers.\n", error, foranything);
            else
              printf(" success. %lu PCR registers.\n", foranything);
            break;
        }
        break;
      case 'k':
        show_loaded_keys();

        break;
      case 'o':
        printf("Take ownership ...");
        if (strlen((char *)srk_auth) == 0 || strlen((char *)owner_auth) == 0)
        {
          printf(" failed. Please specify a SRK and/or owner authentitication. (use 'a')\n");
          break;
        }
        error = TPM_TakeOwnership((unsigned char *) owner_auth,
                                  (unsigned char *) srk_auth, &key);
        if (error)
          printf(" failed (error=%d)\n", error);
        else
          printf(" success.\n");

        break;
      case 'p':
        memset(anything, 0, sizeof(anything));
        
        printf("Public key of key handle (hex): 0x");
        contxt_ihb_read((char *)anything, sizeof(anything), NULL);
        keyhandle = strtol((char *)anything, NULL, 16);

        printf("\nAuthentication/password of key 0x%08lx: ", keyhandle);
        contxt_ihb_read((char *)anything, sizeof(anything), NULL);
        sha1(anything, strlen((char *)anything), anything);

        error = TPM_GetPubKey(keyhandle, anything, &pubkey);

        if (error)
          printf(" failed (error=%d)\n", error);
        else
        {
          printf(" success.\n");

          if (pubkey.keylength > 256)
            printf("Problem with public key data structure. Key length is greater than 256 (%lu)\n", pubkey.keylength);
          else
          {
            #ifdef _LOG_OUTPUT
            log2 = log;
            #endif
            printf("exponent: ");
            for (i=0; i < pubkey.expsize; i++)
            {
              printf("%02x", pubkey.exponent[i]);
              #ifdef _LOG_OUTPUT
              sprintf(log2, "%02x", pubkey.exponent[i]);
              log2 += 2;
              #endif
              if ((i + 1) % 30 == 0)
                printf("\n        ");
            }
            #ifdef _LOG_OUTPUT
            *log2 = 0;
            LOG("%s\n", log);
            #endif
            printf("\n");

            printf("modulus: ");
            #ifdef _LOG_OUTPUT
            log2 = log;
            #endif
            for (i=0; i < pubkey.keylength; i++)
            {
              printf("%02x", pubkey.modulus[i]);
              #ifdef _LOG_OUTPUT
              sprintf(log2, "%02x", pubkey.modulus[i]);
              log2 += 2;
              #endif
              if ((i + 1) % 30 == 0)
                printf("\n        ");
            }
            printf("\n");
            #ifdef _LOG_OUTPUT
            *log2 = 0;
            LOG("%s\n", log);
            #endif
          }
        }
      
        break;
      case 'q':
        memset(anything, 0, sizeof(anything));
        memset(anything2, 0, sizeof(anything2));

        printf("Key to be used for signing (hex): 0x");
        contxt_ihb_read((char *)anything, sizeof(anything), NULL);
        keyhandle = strtol((char *)anything, NULL, 16);

        printf("\nAuthentication/password of key 0x%08lx: ", keyhandle);
        contxt_ihb_read((char *)anything, sizeof(anything), NULL);
        sha1(anything, strlen((char *)anything), anything);

        //TODO
        anything2[0] = 'n';
        anything2[1] = 'o';
        anything2[2] = 'n';
        anything2[3] = 'c';
        anything2[4] = 'e';
        anything2[5] = 0;
        sha1(anything2, strlen((char *)anything2), anything2);

        printf("\nStart quoting ... ");

        error = quotePCRs(keyhandle, anything, anything2, quote, &quotelen,
                          pcrcomposite, sizeof(pcrcomposite), maxpcrs);

        if (error)
          printf(" failed (error=%d)\n", error);
        else
        {
          printf(" success.");
          show_quote();
        }

        break;
      case 'r':
        printf("Generating random numbers by TPM ...");
        error = TPM_GetRandom(16, &foranything, anything);

        if (error != 0)
          printf(" failed (error=%d)\n", error);
        else
        {
          printf(" success. %lu numbers: ", foranything);
          for (i=0; i< foranything; i++)
            printf("%02x ", anything[i]);

          printf("\n");
        }
        break;
      case 's':
        printf("Performing TPM self test ...");

        error = TPM_SelfTestFull();

        if (error)
          printf(" failed (error=%d)\n", error);
        else
          printf(" success.\n");

        break;
      case 'w':
        printf("Clear ownership ...");

        error = TPM_OwnerClear((unsigned char *) owner_auth);

        if (error)
          printf(" failed (error=%d)\n", error);
        else
          printf(" success.\n");

        break;
      case 'x':
        memset(anything, 0, sizeof(anything));

        printf("Name of (v)TPM to be used: ");
        contxt_ihb_read((char *)anything, sizeof(anything), NULL);
        printf("\n\nCheck for availability ... ");

        error = stpm_check_server((char *)anything, 1);

        if (error)
          printf(" failed (error=%d)\n", error);
        else
				{
          printf(" success. Try to detect TPM version of '%s' ... \n\n", anything);
					maxpcrs = tpm_check();
				}

        break;
      default:
        printf("unknown command\n");
    }

  }
}

int main(int argc, const char * argv []) {
  int error;

  if ((error = contxt_init(4096, 1000)))
    {
      LOG("Error %d opening contxt lib -- terminating", error);
      return error;
    }

  command_loop();

  return 0;
}
