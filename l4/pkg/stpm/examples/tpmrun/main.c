/*
 * \brief   Simple L4Con console for getting in contact with a TPM.
 *          This tool can be used for creating, loading keys
 *          and much more ... simply press 'h' in the console for
 *          an overview.
 * \date    2007-01-11
 * \author  Alexander Boettcher <boettcher@os.inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2007 Alexander Boettcher <boettcher@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the STPM package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <stdlib.h> //strtol

#include <l4/log/l4log.h>
#include <l4/l4con/l4contxt.h>

#include <tcg/quote.h>
#include <tcg/rand.h>       //TPM_GetRandom
#include <tcg/owner.h>      //TPM_TakeOwnership 

#include "tpmrun.h"

static unsigned char   srk_auth [20]; //password of SRK 
static unsigned char owner_auth [20]; //password of owner
static unsigned char   anything [20]; //password of any keys

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
      printf("Key handle: %04lX\n", keys[i]);
}

static void show_help_info()
{
  printf("h ... this help info\n");
  printf("a ... set authentification of owner or SRK to be used\n");
  printf("c ... create a new RSA key for signing in tmp buffer\n");
  printf("e ... evict loaded key from TPM\n");
  printf("E ... evict loaded key from TPM, TCGA 1.2 \n");
  printf("k ... list loaded keys\n");
  printf("l ... load a key to TPM from tmp buffer\n");
  printf("L ... load a key to TPM from tmp buffer, TCGA 1.2\n");
  printf("o ... take ownership of TPM\n");
  printf("q ... quote of current pcrs with a loaded key\n");
  printf("r ... generate random numbers\n");
  printf("s ... selftest of TPM\n");
  printf("v ... version information of the TPM\n");
  printf("w ... clear owner of TPM\n");    
}

static void command_loop()
{
  keydata key;
  int argc = 4;
  char * argv [4];
  unsigned long foranything;
  int error;
  int c, i;
  int major, minor, version, rev;


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
        printf("Enter a new authentification for new key: ");
        contxt_ihb_read((char *)anything, sizeof(anything), NULL);
        sha1(anything, strlen((char *)anything), anything);

        printf("\nGenerating signing key as child of SRK ... ");

        error = createKey(&key, srk_auth, anything);
        
        if (error != 0)
          printf("failed (error=%d)\n", error);
        else
          printf("success. Key info stored in memory temporarily.\n");

        break;
      case 'e':
        printf("Key to be deleted (hex): 0x");
        contxt_ihb_read((char *)anything, sizeof(anything), NULL);
        printf(" ...");

        error = TPM_EvictKey(strtol((char *)anything, NULL, 16));

        if (error != 0)
          printf(" failed (error=%d)\n", error);
        else
          printf(" success.\n");

        break;
      case 'E':
        printf("Key to be deleted (hex): 0x");
        contxt_ihb_read((char *)anything, sizeof(anything), NULL);
        printf(" ...");

        error = TPM_FlushSpecific(strtol((char *)anything, NULL, 16), TPM_RT_KEY);

        if (error != 0)
          printf(" failed (error=%d)\n", error);
        else
          printf(" success.\n");

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
      case 'v':
        printf("Getting TPM version ...");
	error = TPM_GetCapability_Version(&major,&minor,&version,&rev);

        if (error)
          printf(" failed (error=%d)\n", error); 
	else
          printf(" success. TPM version: %d.%d.%d.%d\n", major, minor, version, rev);

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
      case 'q':
        argv[1] = "21ac06";
        argv[2] = "c";
        argv[3] = "nounce";

	quote_stdout(argc, (unsigned char **)argv);

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

  // Hash key auths (as required by TCG?!)

  command_loop();

  return 0;
}
