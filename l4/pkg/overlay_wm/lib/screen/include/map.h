/*** MAP SHARED MEMORY BLOCK INTO LOCAL ADDRESS SPACE ***/
extern void *ovl_screen_get_framebuffer(char *smb_ident);

/*** UNMAP FRAMEBUFFER FROM LOCAL ADDRESS SPACE ***/
extern void ovl_screen_release_framebuffer(void *addr);
