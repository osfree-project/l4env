/*
 * \brief   png stream module
 * \date    2003-12-30
 * \author  Jens Syckor <js712688@inf.tu-dresden.de>
 *
 * This file implements an attached png stream. 
 */

struct private_png_stream;
#define PNG_STREAM struct private_png_stream

#define MIN(a, b) ((a)<(b)?(a):(b))

#include <string.h>
#include "png_stream.h"

size_t l4libpng_fread (void *buf, size_t size, size_t nmemb, PNG_STREAM *stream);

PNG_STREAM {

	/* png_stream methods */
	struct png_stream_methods *png_stm;
	
	char *content;	
	int offset;
	int size;
};

size_t l4libpng_fread (void *buf, size_t size, size_t nmemb, PNG_STREAM *stream) {
	long count;
	int ret;
	char *byte_buf;

	byte_buf = (char *) buf;
	count = size * nmemb;

	if (count <= 0) return 0;

	ret = MIN(count, stream->size - stream->offset);

	memcpy(byte_buf,stream->content+stream->offset,ret);

	stream->offset+=ret;

	return ret;
}

static struct png_stream_methods png_stream_meth = {
	l4libpng_fread,
};

static PNG_STREAM *create(char *addr, int size) {
	PNG_STREAM *new = (PNG_STREAM *) malloc(sizeof(PNG_STREAM));
	new->png_stm = &png_stream_meth;
	new->content = addr;
	new->size = size;
	new->offset=0;
	return new;
}

struct png_stream_services png_stream_serv = {
	create,
};
