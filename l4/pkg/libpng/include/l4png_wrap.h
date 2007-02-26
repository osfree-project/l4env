#define ARGB_BUF_TO_SMALL -2
#define ENOPNG -3;
#define EDAMAGEDPNG -4;

int png_get_width(void *png_data, int png_data_size);
int png_get_height(void *png_data, int png_data_size);

/** CONVERT PNG TO A ARGB-BUFFER (ALPHA, RED, GREEN, BLUE) **/
int png_convert_ARGB(void *png_data, void *argb_buf, int png_data_size, int argb_max_size);
int png_convert_RGB16bit(void *png_data, void *argb_buf, int png_data_size, int argb_max_size, int line_offset);
