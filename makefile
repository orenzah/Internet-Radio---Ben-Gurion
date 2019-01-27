CC = gcc
CCFLAGS = -Wall -g
DEPS = structs.h
LIBS ?= -lpthread
OBJS = tcp_server.c

tcp_server: $(OBJS)
	$(CC) $(CCFLAGS)  -o $@ $^ $(LIBS)
