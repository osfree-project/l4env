PKGDIR	?= ../..
L4DIR	?= $(PKGDIR)/../..

# insert the binary name of the server to test
TEST_SERVER	= crypto_test_linux

# insert the binary name of the application testing the server
TEST_CLIENT	=

EXPECTED_OUT	= $(SRC_DIR)/expected.txt

TIMEOUT		= 10

include $(L4DIR)/mk/runux.mk
