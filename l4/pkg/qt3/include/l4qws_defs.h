#ifndef __QT_DROPS_DEFS_H
#define __QT_DROPS_DEFS_H

/* These #defines are set via commandline when libqte_drops.a is
 * built. They do not appear in qconfig.h, but they are required 
 * so that the lib and any application make the same assumptions
 * about features and size/layout of data structures.
 * Because these #defines were missing in the test application,
 * memory corruption occured.
 */

#ifndef Q_OS_DROPS
#define Q_OS_DROPS
#endif

#ifndef QWS
#define QWS
#endif

#endif /* __QT_DROPS_DEFS_H */
