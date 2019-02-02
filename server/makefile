CC=gcc
CFLAGS= -Wall -g -lpthread
DEPS = tcp_server.h
OBJ = tcp_server.o
GRN=\033[0;32m
NC=\033[0m # No Color
all: server clean

%.o: %.c $(DEPS)
	@$(CC) -c -o $@ $< $(CFLAGS)

server: $(OBJ)
	@echo -e "${GRN}Compiling the server side${NC}"
	@$(CC) -o $@ $^ $(CFLAGS)
	@echo -e "${GRN}Great Success! ${NC}"
clean:
	@rm -f *.o core *.core 

