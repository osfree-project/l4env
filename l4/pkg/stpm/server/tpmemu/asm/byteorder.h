#ifndef __dumy_byteorder
#define __dummy_byteorder

#include <arpa/inet.h>

#define __cpu_to_be64s(x) __cpu_to_be64(x)
#define __be64_to_cpus(x) __be64_cpu_cpu(x) 
#define __cpu_to_be32s(x) htonl(x)
#define __be32_to_cpus(x) ntohl(x)
#define __cpu_to_be16s(x) htons(x)
#define __be16_to_cpus(x) ntohs(x)
#define __cpu_to_be16(x) htons(x)
#define __be16_to_cpu(x) ntohs(x)
#define __cpu_to_be64(x) (((unsigned long long)htonl(x)) << 32) & htonl((x) >> 32)
#define __be64_to_cpu(x) (((unsigned long long)ntohl(x)) << 32) & ntohl((x) >> 32)
#define __cpu_to_be32(x) htonl(x)
#define __be32_to_cpu(x) ntohl(x)

#endif
