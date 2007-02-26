INTERFACE [arm]:

void kdb_ke(const char *msg) asm ("kern_kdebug_entry") 
__attribute__((long_call));

