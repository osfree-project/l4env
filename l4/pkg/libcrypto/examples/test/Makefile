PKGDIR		?= ../../
L4DIR		?= $(PKGDIR)/../..

TARGET		= crypto_test_ref crypto_test_linux crypto_test_oslo \
		  crypto_test_openssl


# the default relocation address. This may be superseded by a STATIC file.
DEFAULT_RELOC	= 0x01800000

# list your .c files here
SRC_C	= test.c

# if your server implements the server side of an idl defined in an idl-file
# of your package, list the idl file name(s) here (no path needed)
CLIENTIDL	=

# list additional library paths and libraries here
LIBS	= -lcrypto_pad -lcrypto_rsaref2 -lcrypto_modes
LIBS_crypto_test_ref     = -lcrypto_sha1_ref -lcrypto_aes_linux_586.o
LIBS_crypto_test_linux   = -lcrypto_sha1_linux -lcrypto_aes_linux.o
LIBS_crypto_test_oslo    = -lcrypto_sha1_oslo -lcrypto_aes_linux.o
LIBS_crypto_test_openssl = -lcrypto_sha1_openssl -lcrypto_aes_openssl

MODE	= l4env_minimal

include $(L4DIR)/mk/prog.mk
