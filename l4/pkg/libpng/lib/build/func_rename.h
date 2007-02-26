#define FILE PNG_STREAM 

#define fread(buf, size, nmemb, file) \
        l4libpng_fread(buf, size, nmemb, file)

#define pow(x, y) \
	ipow(x, y)
