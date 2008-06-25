/* (c) 2008 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef L4_VTPMEMU_INCLUDE_XYZ
  #define L4_VTPMEMU_INCLUDE_XYZ
  #include <l4/sys/types.h>

  void vtpmemu_event_loop(void *data);
  char * vtpm_get_name(void);

  int
  _seal_TPM(unsigned char * outdata, unsigned int * outdatalen,
            unsigned char * indata, int indatalen);

  int
  _unseal_TPM(unsigned int sealedbloblen,
              unsigned char * sealedblob,
              unsigned int * unsealedlen,
              unsigned char ** unsealedblob);

  struct slocal {
    l4_taskid_t shutdown_on_exit;
    char * vtpmname;
  };
#endif
