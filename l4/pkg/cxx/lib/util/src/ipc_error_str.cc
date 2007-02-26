/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the cxx package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

namespace L4 {

  char *ipc_error_str[] = 
    { "ok",
      "timeout",
      "phase canceled",
      "mapping failed",
      "send page fault timeout",
      "receive page fault timeout",
      "aborted",
      "message cut" 
    };
};
