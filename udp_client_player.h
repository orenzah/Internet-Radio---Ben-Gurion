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



void udp_player(void* arg)
{
	/* Create a datagram socket on which to receive. */
	in_addr_t mc_grp;
	in_addr_t mc_grp_old;
	int sd;
	int msqid = *((int*)arg);
	struct sockaddr_in localSock;
	struct ip_mreq group;
	int station_num;
	int port;
	int cnt_bytes = 0;
    unsigned long i = 0;
    
    printf("msqid: %d\n", msqid);
    FILE *fp;
    
    //struct station *station = argv;
	msgbox_player mymsg = {0};
	int mytype = 1;
	int msg_bytes = msgrcv(msqid, &mymsg, sizeof(mymsg), mytype,0);
	char ip_string[16];
	if (msg_bytes > 0)
	{
		
		strcpy(ip_string, mymsg.buf);
		printf("ip got: %16s\n",ip_string);
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
    printf("ip %x\n", mc_grp);
    mc_grp_old = mc_grp;
    
	sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sd < 0) 
	{
		perror("Opening datagram socket error");
		exit(1);
	} 
	else
	{
		printf("Opening datagram socket....OK\n");
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
	else
		printf("Setting SO_REUSEADDR...OK\n");
	}

	/* Bind to the proper port number with the IP address */
	/* specified as INADDR_ANY. */
	memset((char *) &localSock, 0, sizeof(localSock));
	localSock.sin_family = AF_INET;
	localSock.sin_port = htons(port);
	localSock.sin_addr.s_addr = htonl(INADDR_ANY);
	printf("ip: %d\n", localSock.sin_addr.s_addr);


	if(bind(sd, (struct sockaddr*)&localSock, sizeof(localSock))) {
		perror("Binding datagram socket error");
		close(sd);
		exit(1);
	} 
	//localSock.sin_addr.s_addr = mc_grp;

	/* Join the multicast group 226.1.1.1 on the local 0.0.0.0 */
	/* interface. Note that this IP_ADD_MEMBERSHIP option must be */
	/* called for each local interface over which the multicast */
	/* datagrams are to be received. */

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
   player = popen("play -t mp3 -> /dev/null 2>&1", "w");
   char databuf[1024] = {0};
   while(1)//while no change station input form user
	{
		int station;
		msgbox_player mymsg = {0};
		int mytype = 5;
		int msg_bytes = msgrcv(msqid, &mymsg, sizeof(mymsg), mytype, IPC_NOWAIT);
		if (msg_bytes > 0)
		{
			close(sd);
			char *a = (char*)malloc(10);;
			pthread_exit((void*)a);
		}
		mytype = 3;
		msg_bytes = msgrcv(msqid, &mymsg, sizeof(mymsg), mytype, IPC_NOWAIT);
		if (msg_bytes > 0)
		{
			int reuse = 1;
			sscanf(mymsg.buf, "%d", &station);
			printf("the new station is %d\n", station);
			/*
			localSock.sin_addr.s_addr = mc_grp + (station << 24);
			int rtn = 10;
			rtn = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));
			
			assert(rtn == 0);
			if(bind(sd, (struct sockaddr*)&localSock, sizeof(localSock))) 
			{
				perror("Binding datagram socket error");
				close(sd);
				exit(1);
			} 
			printf("ip changed to %x\n", localSock.sin_addr.s_addr);
			*/
			if(setsockopt(sd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&group, sizeof(group)) < 0) 
			{
				perror("Adding multicast group error");
				close(sd);
				exit(1);
			}
			group.imr_multiaddr.s_addr = mc_grp + (station << 24);
			if(setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0) 
			{
				perror("Adding multicast group error");
				close(sd);
				exit(1);
			} 
		}		
		
		int size = sizeof(localSock);
		rec_bytes = recvfrom(sd, (char *)databuf, BUFFER_SIZE,  
                0, (struct sockaddr *) &localSock, 
                &size);
		if(rec_bytes < 0) 
		{
			perror("Reading datagram message error");
			close(sd);
			exit(1);
		}
		else 
		{
			//printf("received %d\n", rec_bytes);
			fwrite(databuf , sizeof(char),rec_bytes, player);//write a buffer of size numbyts into fp

		}
	
	}
}
