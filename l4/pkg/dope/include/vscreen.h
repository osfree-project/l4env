/* handling shared memory buffer of the widget */
extern void *vscr_get_fb(char *smb_ident);

/* handling vscreen server */
extern void *vscr_get_server_id(char *vscr_ident);
extern void  vscr_release_server_id(void *vscr_server_id);
extern void  vscr_server_waitsync(void *vscr_server_id);
