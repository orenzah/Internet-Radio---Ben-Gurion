

/**************************************************************/
/* This program uses the Select function to control sockets   */
/**************************************************************/
    #include <stdio.h> 
    #include <stdlib.h> 
    #include <errno.h> 
    #include <string.h> 
    #include <sys/types.h> 
    #include <netinet/in.h> 
    #include <sys/socket.h> 
    #include <sys/wait.h> 

    #define MYPORT 3456    /* the port users will be connecting to */

    #define BACKLOG 100    /* how many pending connections queue will hold */

    main()
    {
        int 					sockfd, new_fd;  /* listen on sock_fd, new connection on new_fd */
        struct 	sockaddr_in 	my_addr;    /* my address information */
        struct 	sockaddr_in 	their_addr; /* connector's address information */
        int 					sin_size;
		struct 	timeval 		tv;		/*The time wait for socket to be changed	*/
        fd_set 					readfds, writefds, exceptfds; /*File descriptors for read, write and exceptions */

		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_ZERO(&exceptfds);
	

        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("socket");
            exit(1);
        }

        my_addr.sin_family = AF_INET;         /* host byte order */
        my_addr.sin_port = htons(MYPORT);     /* short, network byte order */
        my_addr.sin_addr.s_addr = INADDR_ANY; /* auto-fill with my IP */
        bzero(&(my_addr.sin_zero), 8);        /* zero the rest of the struct */

        if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) \
                                                                      == -1) {
            perror("bind");
            exit(1);
        }

        if (listen(sockfd, BACKLOG) == -1) {
            perror("listen");
            exit(1);
        }

        while(1) {  /* main accept() loop */
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
            printf("server: got connection from %s\n", \
                                               inet_ntoa(their_addr.sin_addr));
            if (send(new_fd, "Hello, world!\n", 14, 0) == -1)
                 perror("send");


            close(new_fd);

        }
    }

