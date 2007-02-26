OBJECTS = utils.o accept_test.o close_shutdown_test.o io_test_tcp.o listen_test.o socket_test.o bind_test.o io_test.o io_test_udp.o socket.o
CFLAGS  = -Wall -g
CC      = gcc
SOURCES = main.c prim.c
SHELL   = /bin/bash


socket: $(OBJECTS) utils.h
	$(CC) -lpthread -o $@ $(OBJECTS)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:  
	$(RM) $(OBJECTS) *% socket *~
