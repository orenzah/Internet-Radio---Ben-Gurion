

/**************************************************************/
/* This program uses the Select function to control sockets   */
/**************************************************************/
#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/ipc.h>
#include <sys/msg.h>
#include <netdb.h>
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/wait.h> 
#include <pthread.h> 
#include <time.h>
#include <unistd.h>
#include "udp_client_player.h"

#define MYPORT 3456    /* the port users will be connecting to */
#define BACKLOG 100    /* how many pending connections queue will hold */
#define BUFFER_SIZE 1024
#define MAX_SONGS 30
#define UPLOAD_INTERVAL 8000
clock_t start, end;
/* predefined enums*/
enum permitEnum {no, yes};
/* predefined structs */
struct song_node
{
	size_t	songSize;
	uint32_t nameLength;
	char* name;
	uint16_t station;
} typedef song_node;
struct node
{
	void*	pointer;
	struct node* next;
} typedef node;

struct mymsg {
	long type;
	char* text;
};
struct hello_msg
{
	uint8_t commandType;
	uint16_t reserved;
} typedef hello_msg;
struct asksong_msg
{
	uint8_t commandType;
	uint16_t stationNumber;
} typedef asksong_msg;
struct upsong_msg
{
	uint8_t		commandType;
	uint32_t	songSize;
	uint8_t		songNameSize;
	char		songName[100];
} typedef upsong_msg;
struct permit_msg
{
	uint8_t replyType;
	uint8_t permit_value;
} typedef permit_msg;
struct invalid_msg
{
	uint8_t replyType;
	uint8_t replySize;
	char*	text;
} typedef invalid_msg;
struct newstations_msg
{
	uint8_t replyType;
	uint16_t station_number;
} typedef newstations_msg;
struct welcome_msg
{
	uint8_t replyType;
	uint16_t numStations;
	uint32_t multicastGroup;
	uint16_t portNumber;
} typedef welcome_msg;
struct announce_msg
{
	uint8_t replyType;
	uint8_t songNameSize;
	char	text[100];
} typedef announce_msg;
/* Global variables */
int		msg_await = 0;
int		msg_waiting[5] = {0};

int 					sockfd, new_fd;  /* listen on sock_fd, new connection on new_fd */
node*	head;
pthread_t* udp_player_th;
volatile uint16_t stations_cnt = 0;
uint32_t mcast_g;
uint16_t mcast_p;
song_node song_arr[MAX_SONGS] = {0};
pthread_mutex_t fastmutex = PTHREAD_MUTEX_INITIALIZER;
key_t	msg_boxes[100]	= {0};
int		clients			= 0;	
int msqid;	
key_t msgbox_key;
/* functions declarations */
void read_stdin();
void read_socket(int fd);
int get_msg_type(char * buffer, size_t size);
void print_ip(uint32_t ip);
void got_announce(char* buffer);
void got_welcome(char* buffer);
void send_upsong(char* filename);
void upload_song(char* filename);
void got_newstations(char* buffer);
void got_invalidCommand(char* buffer);
void ip_to_str(char* str, uint32_t ip);
void main(int argc, char* argv[])
{
	
	
	struct 	sockaddr_in 	server_addr; /* connector's address information */
	int 					sin_size;
	struct 	timeval 		tv = {0};		/*The time wait for socket to be changed	*/
	fd_set 					readfds, writefds, exceptfds; /*File descriptors for read, write and exceptions */
	struct hostent *he;
	int port;
	int numbytes;
	tv.tv_usec = 300000;
	
	key_t msg_key = ftok("msgBox", 10);
	if ((msqid = msgget(msg_key/*Warning key_t*/, IPC_CREAT | 0666 )) < 0) 
	{
		perror("msgget");
		exit(1);
	}		
	if(argc != 3)
	{
		fprintf(stderr, "usage: expected 2 arguments\n");
	}
	if((he = gethostbyname(argv[1])) == NULL)
	{
		herror("gethostbyname");
		exit(1);
	}
	if( sscanf(argv[2],"%d", &port) == NULL)
	{
		perror("port");
		exit(1);
	}
	
	/*
	head = (node*)malloc(sizeof(node));
	head->next = NULL;
	head->pointer = NULL;
	*/
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));


	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	server_addr.sin_family = AF_INET;         /* host byte order */
	server_addr.sin_port = htons(port);     /* short, network byte order */
	server_addr.sin_addr = *((struct in_addr *)he->h_addr); /* auto-fill with my IP */
	bzero(&(server_addr.sin_zero), 8);        /* zero the rest of the struct */
	
	
	if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) \
																  == -1) {
		perror("connect");
		exit(1);
	}
	
	char buffer[sizeof(hello_msg)] = {0};
	hello_msg msg = {0};
	msg.commandType = 0;
	memcpy(buffer, &msg, sizeof(msg));
	if (send(sockfd, buffer, sizeof(buffer),0) == -1)
	{
		perror("send");
		exit(1);
	}

	
	char buf[1024] = {0};
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
	if ((numbytes = recv(sockfd, buf,BUFFER_SIZE,0)) == -1)
	{
		if (errno == EAGAIN)
		{
			printf("timeout: server is not responding to Hello Message\n");
			close(sockfd);
			exit(1);
		}
		else
		{
			perror("recv");
			exit(1);
		}
	}
	if (numbytes > 0)
	{
		int type = -1;
		//printf("got reply: \r");
		type = get_msg_type(buf, numbytes);
		if (type == 0)
		{
			// Got willkommen message
			// Now establish
			got_welcome(buf);
			msgbox_player udp_msg_ip = {0};
			msgbox_player udp_msg_port = {0};
			udp_msg_ip.mtype = 1;
			udp_msg_port.mtype = 2;
			sprintf(udp_msg_port.buf,"%d", mcast_p);
			ip_to_str(udp_msg_ip.buf, mcast_g);
			
			udp_player_th = (pthread_t*)malloc(sizeof(pthread_t));
			
			msgsnd(msqid, &udp_msg_ip, sizeof(udp_msg_ip), 0);
			msgsnd(msqid, &udp_msg_port, sizeof(udp_msg_port), 0);
			printf("client  %d\n", msqid);
			/*
			int rc;
			pthread_attr_t attr;
			rc = pthread_attr_init(&attr);
			rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
			* */
			pthread_create(udp_player_th,0, udp_player, &msqid);
		}
		else
		{
			// Wrong message accepted
			// bye
			printf("error: wrong message from server\n\r");
			close(sockfd);
			exit(1);
		}
		
	}
	
	printf("Welcome to the Internet Radio Client\n");
	printf("Connection has been established with: ");
	printf("%s\n", argv[1]);
	printf("Mutlicast group address:\t");
	print_ip(mcast_g);
	printf("Mutlicast group port:\t\t%04d\n", mcast_p);
	printf("Server has %d stations\n", stations_cnt);
	printf("Usage: \n");
	printf("Ask Song:\nasksong #\n");
	printf("Up Song:\nupsong filename\n");
	printf("Change Station:\nstation #\n");
	printf("Exit:\nquit 0\n\r");
	printf("> \r");
	int retval = 0;
	while(1)
	{
		FD_ZERO(&readfds);
		FD_SET(0, &readfds);
		FD_SET(sockfd, &readfds);
		if (msg_await)
		{
			end = clock();
			double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
			if (cpu_time_used > 0.3)
			{
				close(sockfd);
				exit(1);
			}			
		}
		retval = select(sockfd + 1, &readfds, NULL, NULL, &tv);
		if (retval == -1)
		{
			perror("select socket");
			exit(1);
		}
		else if (retval > 0)
		{
			if (FD_ISSET(0, &readfds))
			{
				read_stdin();
				
			}
			if (FD_ISSET(sockfd, &readfds))
			{
				read_socket(sockfd);
			}
			
		}

	}

}
void read_stdin()
{
	char user_cmd[100] = {0};
	int  user_arg = -1;
	scanf("%s", &user_cmd);
	
	int type = get_cmd_type(user_cmd);
	switch(type)
	{
		case 1:
			//asksong
			scanf("%d", &user_arg);
			send_asksong(user_arg);
			break;
		case 2:
			{
				char filename[100] = {0};
				scanf("%s", &filename);
				send_upsong(filename);
			}
			break;
		case 3:
			{
				int chg_stat = 0;
				scanf("%d", &chg_stat);
				msgbox_player udp_msg = {0};
				udp_msg.mtype = 3;
				sprintf(udp_msg.buf, "%d", chg_stat);			
				msgsnd(msqid, &udp_msg, sizeof(udp_msg), 0);
				// TODO change station
			}
			break;
		case 4:
			//TODO quit
			close(sockfd);
			
			msgbox_player udp_msg = {0};
			udp_msg.mtype = 5;
			msgsnd(msqid, &udp_msg, sizeof(udp_msg), 0);
			//void *j;
			//pthread_join(*udp_player_th, (void**)&j);
			pthread_join(*udp_player_th, 0);
			
			free(udp_player_th);
			
			exit(0);
			break;
		default:
			break;												
	}
	

}
void read_socket(int fd)
{
	size_t numbytes = 0;
	char buffer[BUFFER_SIZE] = {0};
	if ((numbytes = recv(fd, buffer,BUFFER_SIZE,0)) == -1)
	{
		perror("recv");
		exit(1);
	}
	if (numbytes > 0)
	{
		int type = -1;
		//printf("got reply: \r");
		type = get_msg_type(buffer, numbytes);
		msg_waiting[type] = 0;
		switch(type)
		{
			case 1:
			got_announce(buffer);
				break;
			case 2:
				break;
			case 3:
				got_invalidCommand(buffer);
				break;
			case 4:
				//printf("newstations\n");
				got_newstations(buffer);
				break;
			default:
				break;
		}
		
	}
}
int get_msg_type(char * buffer, size_t num)
{
	uint8_t	type;
	memcpy(&type, buffer, sizeof(uint8_t));
	return type;
}
int get_cmd_type(char* text)
{
	uint8_t	type = 0;
	type += (strcmp("asksong", text) == 0)	*1;
	type += (strcmp("upsong", text) == 0)	*2;
	type += (strcmp("station", text) == 0)	*3;
	type += (strcmp("quit", text) == 0)		*4;

	return type;
}
void send_asksong(int arg)
{
	
	asksong_msg msg = {0};
	msg.commandType = 1;
	msg.stationNumber = arg;
	char buffer[sizeof(msg)] = {0};
	memcpy(buffer, &msg, sizeof(msg));
	memcpy(buffer + 1, &(msg.stationNumber), 2);
	msg.stationNumber = ntohs(msg.stationNumber);
	if (send(sockfd, buffer, sizeof(buffer),0) == -1)
	{
		perror("send");
		exit(1);
	}
	start = clock();
	msg_waiting[1] = 1;
	
}

void send_upsong(char* filename)
{
	FILE* songFile = fopen(filename, "rb");
	fseek(songFile, 0L, SEEK_END);
	size_t sz = ftell(songFile);
	fclose(songFile);
	
	upsong_msg msg = {0};
	msg.commandType = 2;
	strcpy(msg.songName, filename);
	msg.songNameSize = strlen(filename);
	msg.songSize = htonl(sz);
	char buffer[sizeof(msg)] = {0};
	memcpy(buffer, &(msg.commandType), 1);
	memcpy(buffer + 1, &(msg.songSize), 4);
	memcpy(buffer + 5, &(msg.songNameSize), 1);
	strcpy(buffer + 6, filename);
	printf("Sending upsong %s\n", filename);
	//memcpy(buffer + 6, &(msg.replyType), 1);
	if (send(sockfd, buffer, sizeof(buffer),0) == -1)
	{
		perror("send");
		exit(1);
	}
	start = clock();
	struct 	timeval tv = {0};		/*The time wait for socket to be changed	*/
	tv.tv_usec = 300000;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
	size_t numbytes = 0;
	char temp_buffer[BUFFER_SIZE] = {0};
	if ((numbytes = recv(sockfd, temp_buffer,BUFFER_SIZE,0)) == -1)
	{
		if (errno == EAGAIN)
		{
			printf("timeout reached\n");
		}
		
		perror("recv");
		exit(1);
	}
	int type = -1;
	type = get_msg_type(temp_buffer, numbytes);
	if (type != 2)
	{
		//TODO fashion exit
		close(sockfd);
		exit(1);
	}
	else
	{
		uint8_t permit = *(temp_buffer+1);
		if (permit == 1)
		{
			upload_song(filename);
		}
		else
		{
			printf("the server answered 'Yesh li Haver'.\nbitch...\n");
		}
		
	}
	msg_waiting[2] = 1;
	
}
void upload_song(char* filename)
{
	FILE* songFile = fopen(filename, "r");
	
	fseek(songFile, 0L, SEEK_END);
	size_t sz = ftell(songFile);
	fseek(songFile, 0L, SEEK_SET);
	//printf("the first char: %d\n", fgetc(songFile));
	fseek(songFile, 0L, SEEK_SET);
	printf("start uploading\n");
	clearerr(songFile);
	size_t bytes_transmit = 0;
	struct 	timeval tv = {0};		/*The time wait for socket to be changed	*/
	tv.tv_usec = 300000;
	setsockopt(sockfd, SOL_SOCKET,SO_SNDTIMEO, (char*)&tv, sizeof(tv));
	printf("00.00%");
	while(feof(songFile) == 0)
	{
		char songBuffer[1024] = {0};
		size_t bytes = 0;
		bytes = fread(songBuffer, sizeof(char), 1024, songFile);
		if (send(sockfd, songBuffer, bytes,0) == -1)
		{
			if ( errno == EAGAIN)
			{
				printf("\b\b\b\b\btimeout reached\n");
			}
			perror("send");
			exit(1);
		}
		bytes_transmit += bytes;
		double percent = 100*((double)bytes_transmit / sz);
		printf("\b\b\b\b\b%5.2lf%", percent);
		usleep(UPLOAD_INTERVAL);
		
	}
	printf("\b\b\b\b\b");
	printf("done uploading %d bytes\n", bytes_transmit);
	fclose(songFile);
	
}
void got_welcome(char* buffer)
{
	
	struct welcome_msg msg = {0};
	memcpy(&(msg.replyType),		buffer,		1);
	memcpy(&(msg.numStations), 		buffer + 1,	2);
	memcpy(&(msg.multicastGroup),	buffer + 3,	4);
	memcpy(&(msg.portNumber),		buffer + 7,	2);
	
	
	stations_cnt	= ntohs(msg.numStations);
	mcast_g			= ntohl(msg.multicastGroup);
	mcast_p			= ntohs(msg.portNumber);	
}
void got_announce(char* buffer)
{
	announce_msg msg = {0};
	memcpy(&msg, buffer,	sizeof(struct announce_msg));
	printf("The song is: %s\n", msg.text);
}
void got_newstations(char* buffer)
{
	newstations_msg msg = {0};
	memcpy(&(msg.replyType), buffer,	1);
	memcpy(&(msg.station_number), buffer + 1,	2);
	msg.station_number = ntohs(msg.station_number);
	printf("server announced on the new station %d\n\r", msg.station_number);
	//printf("got new stations\n");
	//TODO something with the new song
}
int got_permit(char* buffer)
{
	permit_msg msg = {0};
	memcpy(&msg, buffer,	sizeof(struct permit_msg));
	return msg.permit_value;
}
void got_invalidCommand(char* buffer)
{
	invalid_msg msg = {0};
	memcpy(&msg, buffer,	sizeof(struct invalid_msg));
	//TODO take care of the invalid command, exit or something
}
void print_ip(uint32_t ip)
{
    unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;   
    printf("%d.%d.%d.%d\n", bytes[0], bytes[1], bytes[2], bytes[3]);        
}
void ip_to_str(char* str, uint32_t ip)
{
    unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;   
    sprintf(str, "%d.%d.%d.%d\n", bytes[0], bytes[1], bytes[2], bytes[3]);        
    //printf("ip_to_str:%d.%d.%d.%d\n", bytes[0], bytes[1], bytes[2], bytes[3]);   
}
