#if !defined(PETZE_POOLNAME)
#define PETZE_POOLNAME "anywhere"
#endif


/*
 * The following function are provided by libpetze.
 * Only use these functions when you manually report
 * data to the Petze server using custom poolnames.
 * For the standard reporting mechanism, this header
 * file is not needed at all.
 */

extern void *petz_malloc  (char *poolname, unsigned int size);
extern void *petz_calloc  (char *poolname, unsigned int nmemb, unsigned int size);
extern void *petz_realloc (char *poolname, void *addr, unsigned int size);
extern void  petz_free    (char *poolname, void *addr);
extern void *petz_cxx_new (char *poolname, unsigned int size);
extern void  petz_cxx_free(char *poolname, void *addr);

static inline void *malloc(unsigned int size) {
	return petz_malloc(PETZE_POOLNAME, size);
}

static inline void *calloc(unsigned int nmemb, unsigned int size) {
	return petz_calloc(PETZE_POOLNAME, nmemb, size);
}

static inline void *realloc(void *addr, unsigned int size) {
	return petz_realloc(PETZE_POOLNAME, addr, size);
}

static inline void *zalloc(unsigned int size) {
	return petz_malloc(PETZE_POOLNAME, size);
}

static inline void free(void *addr) {
	petz_free(PETZE_POOLNAME, addr);
}

