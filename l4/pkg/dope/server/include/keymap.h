
#define KEYMAP_SWITCH_LSHIFT 	0x01
#define KEYMAP_SWITCH_RSHIFT 	0x02
#define KEYMAP_SWITCH_LCONTROL 	0x04
#define KEYMAP_SWITCH_RCONTROL	0x08
#define KEYMAP_SWITCH_ALT		0x10
#define KEYMAP_SWITCH_ALTGR		0x11


struct keymap_services {
	char	(*get_ascii) (long keycode,long switches);
};
