CC=gcc
CFLAGS= -Wall -g -lpthread
DEPS = tcp_server.h
OBJ = tcp_server.o

all: server clean

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

server: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
clean:
	@rm -f *.o core *.core 

