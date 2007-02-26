#ifndef _MPG123_PROBE_
#define _MPG123_PROBE_

int probe_mp3 (const char *filename);
int get_mp3_info (const char *filename, double *time, int *channels,
		  long *samplerate, int *bitrate);
int get_mp3_taginfo (const char *filename, char info[128]);

#endif
