/* (c) 2008 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef L4_VTPMEMU_INCLUDE_XYZ
  #define L4_VTPMEMU_INCLUDE_XYZ
  void vtpmemu_event_loop(void *data);
  char * vtpm_get_name(void);

  struct slocal {
    l4_taskid_t shutdown_on_exit;
    char * vtpmname;
  };
#endif
