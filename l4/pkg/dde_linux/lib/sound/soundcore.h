#define TYPE_MAX	3	/**< maximum number of device types */
# define TYPE_DSP	0
# define TYPE_MIXER	1
# define TYPE_MIDI	2	/* unused */
#define NUM_MAX		4	/**< maximum number of devices per type */

struct file_operations* soundcore_req_fops(int type, int num);
void soundcore_rel_fops(int type, int num);
