CC=gcc
CFLAGS= -Wall -g -lpthread
DEPS = client.h udp_client_player.h
OBJ = client.o

all: client clean

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

client: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
clean:
	@rm -f *.o core *.core 

