

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
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/wait.h> 
#include <pthread.h> 
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <time.h>


#include "tcp_server.h"
#define MYPORT 3456    /* the port users will be connecting to */
#define BACKLOG 100    /* how many pending connections queue will hold */
#define BUFFER_SIZE 1024
#define MAX_SONGS 30




/* Global variables */
node*	head;
int msqid;
uint32_t mcast_g;
uint16_t mcast_p;
key_t msgbox_key = 0;
song_node song_arr[MAX_SONGS] = {0};
int	song_count = 0;
pthread_mutex_t fastmutex = PTHREAD_MUTEX_INITIALIZER;
key_t	msg_boxes[100]	= {0};
int		clients			= 1;		
client_node* clientsList = 0;

/* functions declarations */
void*				th_tcp_control(void **args);
int					get_msg_type(char * buffer, size_t size);
upsong_msg			get_upsong_details(char * buffer, size_t size);
int					get_asksong_station(char * buffer, size_t size);
void print_ip(uint32_t ip);
void create_songs();
void create_song_transmitter();

/*void signalStopHandler(int signo)
{
	if (signo != SIGSTP)
	{
		return;
	}
	
	client_node* temp;
	while (clientsList)
	{
		temp = clientsList;
		clientsList = clientsList->next;
		close(temp->fileDescriptor);
		free(temp);
	}
	{
		int i;
		for (i = 0; i < MAX_SONGS; i++)
		{
			if (song_arr[i].name == 0)
			{
				break;
			}
			free(song_arr[i].name);
			pthread_cancel(*(song_arr[i].thread_p));
			pthread_join(*(song_arr[i].thread_p),0);
			free((song_arr[i].thread_p));
		}
	}
	exit(0);
	
}
*/
void song_transmitter(void* arg)
{
	int station = *((int*)arg);
	free(arg);
	int sd;
	int i;
	struct ip_mreq group;
	FILE* inputfile = fopen(song_arr[station].name,"rb");
	printf("song file name: %s\n", song_arr[station].name);
	
	struct sockaddr_in multicastAddr;
	char mc_addr[16] = {0};	
	

	int n;
	
	sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	int reuse = 1;
	if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) 
	{
	    perror("Setting SO_REUSEADDR error\n\r");
	    close(sd);
	    exit(1);
	}
	/* Bind to the proper port number with the IP address */
	/* specified as INADDR_ANY. */
/*
	memset((char *) &localSock, 0, sizeof(localSock));
	localSock.sin_family = AF_INET;
	localSock.sin_port = 0;
	localSock.sin_addr.s_addr = mc_grp;
	*/
	group.imr_multiaddr.s_addr = mcast_g + (station << 24);
	group.imr_interface.s_addr = inet_addr("0.0.0.0");
	
	int mutlicastttl = 255;
	
	
	if(setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&group, sizeof(group)) < 0) 
	{
		perror("Adding multicast group error");
		close(sd);
		exit(1);
	}

	if(setsockopt(sd, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&mutlicastttl, sizeof(mutlicastttl)) < 0) 
	{
		perror("Adding multicast group error");
		close(sd);
		exit(1);
	} 

	/* Construct local address structure */
	memset(&multicastAddr, 0, sizeof(multicastAddr));   /* Zero out structure */
	multicastAddr.sin_family = AF_INET;                 /* Internet address family */
	multicastAddr.sin_addr.s_addr = mcast_g + (station << 24);		/* Multicast IP address */
	multicastAddr.sin_port = htons(mcast_p);       /* Multicast port */
	printf("Group Address: %d\n", mcast_g + (station << 24));
	clock_t start, end;
	int bytes_streamed = 0;
	start = clock();
	while(1)
	{
		char songBuffer[BUFFER_SIZE] = {0};
		
		char c;

		c = fread(songBuffer, 1, 1024, inputfile);
		//printf("%d\n", c);
		if (feof(inputfile))
		{
			rewind(inputfile);
			
			c = fread(songBuffer, 1, 1024, inputfile);
			
		}	
			
		if(sendto(sd, songBuffer, 1024, 0, (struct sockaddr*)&multicastAddr, sizeof(multicastAddr)) == -1) 
		{
			perror("Writing datagram message error");
			close(sd);
			exit(1);
		}
		bytes_streamed += 1024;
		usleep(5000);
		//TODO read again at section 3.3
		
		
		if (bytes_streamed > 128*128) 
		{
			end = clock();
			double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
			int micro = (int)((1 - cpu_time_used)*1000*1000);
			//printf("Delay by: %f\n", (0.25 - cpu_time_used));
			usleep(micro);
			
			start = clock();
			bytes_streamed = 0;
			
		}		
		//maybe add sleep after full second passed
	}
	
}
void main(int argc, char* argv[])
{
	int 					sockfd, new_fd;  /* listen on sock_fd, new connection on new_fd */
	struct 	sockaddr_in 	my_addr;    /* my address information */
	struct 	sockaddr_in 	their_addr; /* connector's address information */
	int 					sin_size;
	
	struct 	timeval 		tv = {0};//The time wait for socket to be changed	*/
	fd_set 					readfds, writefds, exceptfds; /*File descriptors for read, write and exceptions */
	uint16_t tcp_port;
	if (argc < 5)
	{
		printf("error: not enough arguments\n");
		exit(1);
	}
	msgbox_key = ftok("/tmp/msgBox", 25);
	if ((msqid = msgget(15/*Warning key_t*/, IPC_CREAT | 0666 )) < 0) 
	{
		perror("msgget");
		exit(1);
	}	
	//signal(SIGSTP, signalStopHandler);

	sscanf(argv[1], "%d", &tcp_port);
	inet_pton(AF_INET, argv[2], &(mcast_g));
	sscanf(argv[3], "%d", &mcast_p);
	int i;
	for(i = 4; i < argc; i++)
	{
		FILE* songFile = fopen(argv[i], "r");
		song_node song = {0};
		int length = strlen(argv[i]);
		fseek(songFile, 0L, SEEK_END);
		size_t sz = ftell(songFile);
		song.songSize	= sz;
		song.nameLength = length;
		song.name = (char*)malloc(length);
		strcpy(song.name, argv[i]);
		song.station = song_count;
		pthread_t* songPlayer = (pthread_t*)malloc(sizeof(pthread_t));
		song.thread_p = songPlayer;
		song_arr[song_count] = song;
		song_count++;
		fclose(songFile);
		
		int* newStationPointer = (int*)malloc(sizeof(int));
		*newStationPointer = i-4;
		pthread_create(songPlayer, NULL, song_transmitter,newStationPointer/* &newStation*/);
	}
	//create_songs();
	
	
	
	
	head = (node*)malloc(sizeof(node));
	head->next = NULL;
	head->pointer = NULL;
	
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);

	

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	my_addr.sin_family = AF_INET;         /* host byte order */
	my_addr.sin_port = htons(tcp_port);     /* short, network byte order */
	my_addr.sin_addr.s_addr = INADDR_ANY; /* auto-fill with my IP */
	bzero(&(my_addr.sin_zero), 8);        /* zero the rest of the struct */
	printf("new\n");
	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) \
																  == -1) {
		perror("bind");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	while(1) 
	{  /* main accept() loop */
	FD_SET(sockfd, &readfds); /*Add sock_fd to the set of file descriptors to read from */
	tv.tv_sec = 30; 				/*Initiate time to wait for fd to change */
	if (select(sockfd + 1, &readfds, 0, 0, &tv) < 0) {
		   perror("select");
		   continue;
		}
		sin_size = sizeof(struct sockaddr_in);
		if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, \
													  &sin_size)) == -1) {
			perror("accept");
			continue;
		}
		pthread_t* thread_pt = (pthread_t*)malloc(sizeof(pthread_t));
		node* temp = (node*)malloc(sizeof(node));
		temp->pointer = thread_pt;
		temp->next = head;
		head = temp;
		int* pfd = (int*)malloc(sizeof(int));
		
		
		//*p_key = ftok("msgFile", 1024); // Create message boxex
		
		void* args[3];
		args[0] = 2; //how many args been passed
		(*pfd) 	= new_fd;
		args[1] = pfd;
		long* client_id = (long*)malloc(sizeof(long));
		*client_id = clients;
		clients++;
		args[2] = client_id;
		
		cascadeClient(new_fd, client_id, &clientsList);
		printf("server: got new connection with fd=%d\n", new_fd);
		pthread_create(thread_pt, NULL, th_tcp_control, args);

	
	}
}

void *th_tcp_control(void **args)
{
	long mytype	= *((int*)args[2]);
	int client_fd = *((int*)args[1]);
	char buffer[BUFFER_SIZE] = {0};
	
	
	size_t struct_size;
	char* buf2snd;

	printf("New client thread created, controlling socket %d\n\r", client_fd);
	struct timeval timeout = {0};
	timeout.tv_usec = 1000000;
	setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
	ssize_t numBytesRcvd = recv(client_fd, buffer, BUFFER_SIZE, 0);
	buffer[numBytesRcvd] = '\0';
	if(get_msg_type(buffer, numBytesRcvd) != 0)
	{

		//prepare InvalidCommand
		invalid_msg inv_msg = {0};
		inv_msg.replyType = 3;
		strcpy(inv_msg.text ,"Shtok Tzair"); 
		inv_msg.replySize = strlen(inv_msg.text);
		
		size_t buf_size = inv_msg.replySize + 2;
		buf2snd = (char*)malloc(buf_size);
		printf("%d\n", buf_size);
		memcpy(buf2snd, &inv_msg, buf_size);
		if (send(client_fd, buf2snd, buf_size, 0) == -1)
		{
			perror("send invalid_command");
		}
		printf("error: ");
		printf("rude client has been connected without saying Hello\n");		
		free(buf2snd);
	}
	else
	{
		struct welcome_msg msg	= {0};
		struct_size =  9;//sizeof(struct welcome_msg);
		printf("size of welcome: %d\n", struct_size);
		msg.replyType			= 0;
		msg.numStations			= htons(song_count);
		msg.multicastGroup		= htonl(mcast_g);
		msg.portNumber			= htons(mcast_p);
		
		//struct_size = 1+2+4+2;
		
		buf2snd = (char*)malloc(struct_size);
		memcpy(buf2snd, &(msg.replyType), 1);
		memcpy((uint16_t*)(buf2snd + 1), &(msg.numStations), 2);
		memcpy((uint32_t*)(buf2snd + 3), &(msg.multicastGroup), 4);
		memcpy((uint16_t*)(buf2snd + 7), &(msg.portNumber), 2);
		//memcpy(buf2snd, &msg, struct_size);
		
		send(client_fd, buf2snd, struct_size, 0);
		free(buf2snd);
	}
	while(1)
	{
		msgbox mymsg = {0};
		
		int msg_bytes = msgrcv(msqid, &mymsg, sizeof(mymsg), mytype, IPC_NOWAIT);
		int newstation = 0;
		if(msg_bytes > 0)
		{
			//printf("%d ", strlen(mymsg.text));
			//printf("%10s\n",mymsg.text);
			//printf("strcmp: %d\n", strcmp(mymsg.text, "newstatio"));
			newstation = (strcmp(mymsg.text, "newstatio") == 0)*1;
		}
		
		if (newstation)
		{
			printf("newstation arrived\n");
			newstations_msg temp_msg = {0};
			temp_msg.replyType = 4;
			temp_msg.station_number = htons(clients);
			size_t size_send = 3;//sizeof(temp_msg);
			char msgBuf[100] = {0};
			memcpy(msgBuf, &(temp_msg.replyType), 1);
			memcpy((uint16_t*)(msgBuf + 1), &(temp_msg.replyType), 2);
			//memcpy(msgBuf, &temp_msg, size_send);
			send(client_fd, msgBuf, size_send, 0);
			newstation = 0;			
		}
		//TODO finish msg box
		ssize_t numBytesRcvd = recv(client_fd, buffer, BUFFER_SIZE, 0);
		if (numBytesRcvd == 0) 
		{
			//close connection

			printf("closing socket and thread\n");
			client_node* temp = clientsList;
			if ((temp->prev) == NULL)
			{
				clientsList = temp->next;
			}
			else
			{
				while (temp && (temp->clientId != mytype))
				{
					temp = temp->next;
				}
				(temp->prev)->next = temp->next;
			}
			free(temp);
			close(client_fd);
			pthread_exit(0);
		}
		buffer[numBytesRcvd] = '\0';
		if(numBytesRcvd == 0)
			continue;
		switch(get_msg_type(buffer, numBytesRcvd))
		{
			case 1: /*	Client AskSong */
				/*TODO: Server tell client Announce*/
				{
					announce_msg msg = {0};
					uint16_t station = get_asksong_station(buffer, numBytesRcvd);
					struct_size =  sizeof(struct announce_msg);
					if(song_arr[station].name == NULL)
					{
						msg.songNameSize	= 0;
						//msg.text			= NULL;
					}
					else
					{
						strcpy(msg.text, song_arr[station].name);
						msg.songNameSize = song_arr[station].nameLength;
					}
					char str[] = "AskSong from my client!\n";
					printf("%s", str);
					size_t buf_size = sizeof(announce_msg) - 100 + strlen(msg.text);
					msg.replyType = 1;
					buf2snd = (char*)malloc(buf_size);
					memcpy(buf2snd, &(msg.replyType), 1);
					memcpy(buf2snd, &(msg.songNameSize), 1);
					memcpy(buf2snd, &(msg.text),strlen(msg.text));
					send(client_fd, buf2snd, struct_size, 0);
					free(buf2snd);
				}
				break;
			case 2: /*	Client UpSong */				
				/*TODO: Server tell client PermitSong*/
				{
					struct permit_msg msg = {0};
					enum permitEnum per;
					msg.replyType = 2;
					printf("upsong req\n");
					int status;
					if((status = pthread_mutex_trylock(&fastmutex)))
					{
						if(status == EBUSY)
						{
							per = no;
							msg.permit_value = (uint8_t)per;
							printf("permit: No.\n");
						}
						else
						{
							perror("mutex try lock");
							exit(1); /* usage of exit is neccesary*/
						}
					}
					else
					{
						per = yes;
						msg.permit_value = (uint8_t)per;
						printf("permit: Yes.\n");
					}
					if (per != yes)
					{
						
					}
					
					/*TODO: check mutex, then answer permit*/
					buf2snd = (char*)malloc(2);
					memcpy(buf2snd, &msg, 2);
					send(client_fd, buf2snd, 2, 0);
					free(buf2snd);
					printf("after reply\n");
					if (msg.permit_value)
					{
						upsong_msg theSong = get_upsong_details(buffer,numBytesRcvd);
						song_node song = {0};
						char songBuffer[BUFFER_SIZE] = {0};
						int len = 0, remain_data = theSong.songSize;
						char* songNameText = (char*)malloc(theSong.songNameSize*sizeof(char));
						strcpy(songNameText, theSong.songName);
						FILE* newsong = fopen(songNameText, "wb");
						
						song.songSize = theSong.songSize;
						song.nameLength = theSong.songNameSize;
						song.name = songNameText;
						song.station = song_count;
						song_arr[song_count++] = song;
						printf("start downloading\n");
						struct 	timeval tv = {0};		/*The time wait for socket to be changed	*/
						tv.tv_usec = 1000000;
						
						setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
						while ((len = recv(client_fd, songBuffer, BUFFER_SIZE, 0)) > 0)
						{
							if(len == 0)
							{
								break;
							}
							if(len < 0)
							{
								perror("recv");
								break;
							}
							if (len >= remain_data)
							{
								printf("remain data: %d\n", remain_data);
								fwrite(songBuffer, sizeof(char), remain_data, newsong);
								break;
							}
							else
							{
								if(fwrite(songBuffer, sizeof(char), len, newsong) != len)
								{
									perror("fwrite");
								}
							}
							remain_data -= len;
							//fprintf(stdout, "Receive %d bytes and we hope : %d bytes\n", len, remain_data);
								
						}
						fclose(newsong);
						printf("song closed\n");						
						if(pthread_mutex_unlock(&fastmutex))
						{
							perror("mutex try lock");
							exit(1); /* usage of exit is neccesary*/
							//TODO fashion exit
						}
						pthread_t* songPlayer = (pthread_t*)malloc(sizeof(pthread_t));
						int newStation = song.station;
						pthread_create(songPlayer, NULL, song_transmitter, &newStation);
						/* Recv the upload*/
						init_newstations_procedure();
					}
				}
				break;
			case 3:
				break;
			default:
				{
					invalid_msg msg = {0};
					char inv_buf[140] = {0};
					msg.replyType = 3;
					strcpy(msg.text, "Invalid Command has been asserted");
					msg.replySize = strlen(msg.text);
					size_t buf_size = msg.replySize + 2;
					memcpy(inv_buf, &msg, buf_size);
					if (send(client_fd, inv_buf, buf_size, 0) == -1)
					{
						perror("send invalid_command");
					}					
				}
				break;
		}
		printf("Received %ld bytes from fd: %d\n", numBytesRcvd, client_fd);		
	}
}
void send_newstation(int fd)
{
	
}
void init_newstations_procedure(void)
{
	msgbox mymsg = {0};
	client_node* temp = clientsList;
	strcpy(mymsg.text, "newstatio");
	while (temp)
	{
		mymsg.mtype = temp->clientId;
		msgsnd(msqid, &mymsg, sizeof(mymsg), 0);
		temp = temp->next;
	}
}
int get_msg_type(char * buffer, size_t size)
{
	uint8_t	type;
	memcpy(&type, buffer, sizeof(uint8_t));
	int i;
	return type;
}

upsong_msg get_upsong_details(char * buffer, size_t size)
{
	struct upsong_msg msg =	{0};
	memcpy(&(msg.replyType), buffer, 1);
	memcpy(&(msg.songSize), buffer, 4);
	msg.songSize = ntohl(msg.songSize);
	memcpy(&(msg.songNameSize), buffer, 1);
	memcpy(&(msg.songName), buffer, msg.songNameSize);
	return msg;
}

int	get_asksong_station(char * buffer, size_t size)
{
	struct asksong_msg msg =	{0};
	memcpy(&(msg.replyType), buffer, 1);
	memcpy(&(msg.station_number), buffer + 1, 2);
	msg.station_number = ntohs(msg.station_number);
	return msg.station_number;
}

void create_songs()
{
	DIR *dirp;
    struct dirent *dp;
    dirp = opendir(".");
    if (!dirp) 
    {
        perror("opendir()");
        exit(1);
    }
	printf("looking for songs\n");
	
	while ((dp = readdir(dirp))) 
	{
		int length = strlen(dp->d_name);
		if(dp->d_name[length-1] != '3' ||
		dp->d_name[length-2] != 'p' ||
		dp->d_name[length-3] != 'm' ||
		dp->d_name[length-4] != '.' )
		{
			continue;
		}
		
		printf("Song: %s\n", dp->d_name);
		song_node song = {0};
		/*
		 *
 *  	size_t	songSize;
		uint32_t nameLength;
		char* name;
		uint16_t station;
		* */
		FILE* songFile = fopen(dp->d_name, "r");
		fseek(songFile, 0L, SEEK_END);
		size_t sz = ftell(songFile);
		song.songSize	= sz;
		song.nameLength = length;
		song.name = (char*)malloc(length);
		strcpy(song.name, dp->d_name);
		song.station = song_count;
		song_arr[song_count] = song;
		song_count++;
		fclose(songFile);
	}
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
