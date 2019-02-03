#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <assert.h>

#define BUFFER_SIZE 1024

struct msgbox_player
{
	long mtype;
	char buf[100];
} typedef msgbox_player;

void print_ip_udp(uint32_t ip)
{
    unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;   
    printf("%d.%d.%d.%d\n", bytes[0], bytes[1], bytes[2], bytes[3]);        
}

void* udp_player(void* arg)
{
	/* Create a datagram socket on which to receive. */
	in_addr_t mc_grp;
	in_addr_t mc_grp_old;
	int sd;
	int msqid = *((int*)arg);
	struct sockaddr_in localSock;
	struct ip_mreq group;
	int port;
	               
	msgbox_player mymsg = {0};
	int mytype = 1;
	int msg_bytes = msgrcv(msqid, &mymsg, sizeof(mymsg), mytype,0);
	char ip_string[16] = {0};
	if (msg_bytes > 0)
	{
		
		strcpy(ip_string, mymsg.buf);
		//printf("ip got: %16s\n",ip_string);
	}
	else
	{
		pthread_exit(0);
	}
	mytype = 2;
	
	msg_bytes = msgrcv(msqid, &mymsg, sizeof(mymsg), mytype,0);
	if (msg_bytes > 0)
	{
		
		sscanf(mymsg.buf, "%d", &port);
	}
	else
	{		
		pthread_exit(0);
	}
    mc_grp = inet_addr(ip_string);
    //printf("ip %x\n", mc_grp);
    mc_grp_old = mc_grp;
    
    #ifdef WITHSUDO
	sd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
	#endif
	#ifndef WITHSUDO
	sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	#endif
	if(sd < 0) 
	{
		perror("Opening datagram socket error");
		exit(1);
	} 

    //pthread_cleanup_push(&close, sd);

	/* Enable SO_REUSEADDR to allow multiple instances of this */
	/* application to receive copies of the multicast datagrams. */
	{
	int reuse = 1;
	if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) 
	{
		perror("Setting SO_REUSEADDR error");
		close(sd);
		exit(1);
	}

	}

	/* Bind to the proper port number with the IP address */
	/* specified as INADDR_ANY. */
	memset((char *) &localSock, 0, sizeof(localSock));
	localSock.sin_family = AF_INET;
	localSock.sin_port = htons(port);
	localSock.sin_addr.s_addr = htonl(INADDR_ANY);
	//printf("ip: %d\n", localSock.sin_addr.s_addr);


	if(bind(sd, (struct sockaddr*)&localSock, sizeof(localSock))) {
		perror("Binding datagram socket error");
		close(sd);
		exit(1);
	} 


	group.imr_multiaddr.s_addr = mc_grp_old;
	group.imr_interface.s_addr = inet_addr("0.0.0.0");

	if(setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0) {
		perror("Adding multicast group error");
		close(sd);
		exit(1);
	}

	/* Read from the socket. */
	int rec_bytes = 0;
	//printf("\n Station number %d playing\n",station_num);
	
   FILE* player;
   #ifndef VLC 
   player = popen("play -t mp3 -> /dev/null 2>&1", "w");
   #endif
   #ifdef VLC
   //player = popen("play -t mp3 alsa -> /dev/null", "w");
   player = popen("vlc - > /dev/null 2>&1", "w");
   #endif
   
   char databuf[2048] = {0};
   

	while(1)//while no change station input form user
	{
		int station;
		msgbox_player mymsg = {0};
		int mytype = 5;
		int msg_bytes = msgrcv(msqid, &mymsg, sizeof(mymsg), mytype, IPC_NOWAIT);
		if (msg_bytes > 0)
		{
			close(sd);
			pclose(player);
			
			//char *a = (char*)malloc(10);;
			//pthread_exit((void*)a);
			pthread_exit(0);
		}
		mytype = 3;
		msg_bytes = msgrcv(msqid, &mymsg, sizeof(mymsg), mytype, IPC_NOWAIT);
		if (msg_bytes > 0)
		{

			sscanf(mymsg.buf, "%d", &station);
			printf("The new station is %d\n", station);
			
			if(setsockopt(sd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&group, sizeof(group)) < 0) 
			{
				perror("Dropping multicast group error");
				close(sd);
				exit(1);
			}
			group.imr_multiaddr.s_addr = mc_grp + (station << 24);
			if(setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0) 
			{
				perror("Adding (after dropping) multicast group error");
				close(sd);
				exit(1);
			} 
		}		
		
		size_t size = sizeof(localSock);
		rec_bytes = recvfrom(sd, (char *)databuf, BUFFER_SIZE*2,  
                0, (struct sockaddr *) &localSock, (socklen_t *)&size);
        
		if(rec_bytes < 0) 
		{
			perror("Reading datagram message error");
			close(sd);
			exit(1);
		}
		else 
		{
			#ifdef WITHSUDO
			
			uint32_t dst_ip = *(uint32_t*)((databuf + 16));
			if ( dst_ip != group.imr_multiaddr.s_addr)
			{
				continue;
			}
			fwrite(databuf + 28, sizeof(char),rec_bytes - 28, player);//write a buffer of size numbyts into fp
			#endif
			//printf("received %d\n", rec_bytes);			
			#ifndef WITHSUDO
			//fwrite(databuf + 28, sizeof(char),rec_bytes - 28, player);//write a buffer of size numbyts into fp
			fwrite(databuf, sizeof(char),rec_bytes, player);//write a buffer of size numbyts into fp
			#endif
		}
	
	}
}

