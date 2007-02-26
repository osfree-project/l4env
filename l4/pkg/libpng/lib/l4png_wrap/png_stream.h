#ifndef PNG_STREAM
#define PNG_STREAM struct public_png_stream
#endif

#include <stdlib.h>

#define size_t unsigned int

struct png_stream_methods;

struct public_png_stream {
	struct png_stream_methods *png_stream_meth;
};

struct png_stream_methods {
	size_t	 (*png_stream_read) (void *, size_t, size_t, PNG_STREAM *);	
};

struct png_stream_services {
        PNG_STREAM *(*create) (char *, int);
};
