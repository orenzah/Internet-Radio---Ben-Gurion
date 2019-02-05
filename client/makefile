CC=gcc
CFLAGS= -Wall -g -lpthread
DEF=
DEPS =  udp_client_player.h
OBJ = client.o
GRN = \033[0;32m
NC=\033[0m # No Color
all: client clean

%.o: %.c $(DEPS)
	@$(CC) -c -o $@ $< $(CFLAGS) $(DEF)

client: $(OBJ)
	@echo "${GRN}Compiling the client side${NC}"\
	@$(CC) -o $@ $^ $(CFLAGS) $(DEF)
	@echo "${GRN}Great Success! ${NC}"

	@$(CC) -o $@ $^ $(CFLAGS)
	@echo  "${GRN}Great Success! ${NC}"

clean:
	@rm -f *.o core *.core 

