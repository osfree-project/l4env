#if !defined(PETZE_POOLNAME)
#define PETZE_POOLNAME "anywhere"
#endif

#define malloc(size) petz_malloc(PETZE_POOLNAME, size)
#define free(addr) petz_free(PETZE_POOLNAME, addr)

void *petz_malloc(char *poolname, unsigned int size);
void  petz_free(char *poolname, void *addr);

