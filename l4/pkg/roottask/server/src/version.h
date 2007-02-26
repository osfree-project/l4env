#ifndef VERSION_H
#define VERSION_H

#define VERSION_L4_V2	         0
#define VERSION_L4_IBM           1
#define VERSION_FIASCO           2
#define VERSION_L4_KA_HAZELNUT   3
#define VERSION_L4_KA_PISTACHIO  4

#define VERSION_ABI_V2           0
#define VERSION_ABI_X0           1
#define VERSION_ABI_V4           2

#define KERNEL_NAMES ((char*[4]) { "L4/GMD", \
                                   "LN/IBM", \
                                   "Fiasco", \
				   "L4Ka::Hazelnut", \
                                   "L4Ka::Pistachio" })

#endif
