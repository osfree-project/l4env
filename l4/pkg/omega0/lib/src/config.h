#ifndef __OMEGA0_LIB_CONFIG_H
#define __OMEGA0_LIB_CONFIG_H


/* define the following if you want to measure the time from sending an
   IPC at server until receiving the IPC at client. The omega0-server must
   send the lowest 32Bit of the TSC in dw1, which can be ensured by setting
   the corresponding compiler-switch when compiling the server. The the lib
   compares this 32Bit with the current value and prints the result.
   
   Note: Printing is VERY slow, use this feature only for measuring exactly
   this time and not in normal use (measuring the time is not so expensive,
   needs a few cycles.)
   
   Note2: When enabling this feature, you have to link a printf
   implementation to your binary.
*/
//#define OMEGA0_LIB_MEASURE_CALL

#endif

