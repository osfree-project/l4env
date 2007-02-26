#include <l4/util/parse_cmd.h>
#include <l4/util/spin.h>
#include <l4/util/util.h>
#include <string.h>

int main(int argc, const char**argv){
    int x, y, delay, len;
    const char*text;

    if(parse_cmdline(&argc, &argv,
		     'x', "x", "x coordinate",
		     PARSE_CMD_INT, 0, &x,
		     'y', "y", "y coordinate",
		     PARSE_CMD_INT, 0, &y,
		     'd', "delay", "delay between updates (milliseconds)",
		     PARSE_CMD_INT, 1000, &delay,
		     't', "text", "text before the wheel",
		     PARSE_CMD_STRING, "", &text,
		     0)) return 1;

    len = strlen(text);
    while(1){
	l4_spin_n_text_vga(x, y, len, text);
	if(delay) l4_sleep(delay);
    }
}
