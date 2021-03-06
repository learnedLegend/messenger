#ifndef _NET_TAG
#define _NET_TAG "NETWORK_UTILS"
#include <stdio.h>
#include <stdlib.h>
#if defined(__linux__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)

#if defined(__linux__) && defined(kernel_version_2_4)
#include <sys/sendfile.h>
#endif
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#else
#error Unknown architecture
#endif

#include <clog/clog.h>

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 256
#endif

#ifndef CON_MAX_ATTEMPTS
#define CON_MAX_ATTEMPTS 5
#endif

/*
 * Write data to a given socket
 * @param sockfd The socket descriptor to write to
 * @param msg The message to be written to the socket
 */
ssize_t write_data(int sockfd, char *msg, int msg_size){
	ssize_t sent_bytes = 0;
	int i = -1;
	while(++i < msg_size && msg[i] != '\0'){
		sent_bytes = write (sockfd, &msg[i], 1);
		if (sent_bytes < 0){
			clog_e(_NET_TAG, "sending failed");
			return -1;
		}
	}
	clog_i(_NET_TAG, "sent data: %s", msg);
	return sent_bytes;
}

/*
 * Write a file to a socket
 * Automatically chooses the optimized way at compile time
 * @param sockfd The socket descriptor to write to
 * @param file_path The file which has to written to the socket
 */
int send_file(int sockfd, const char *file){
#if defined(__linux__) || defined(__APPLE__)
#if defined(__kernel_version_2_4)
	//sendfile(..,..,...);
#else
	//write_data(..,..,..);
#endif
#endif
	//code to send file to socket
	return -1;
}

/** Read data from the given socket
 * @param sockfd The socket descriptor to read form
 */
int read_data (int sockfd, char *buffer, int buffer_size){
	ssize_t read_bytes = 0;
	int i = -1;
	while(++i < buffer_size && buffer[i] != '\0'){
	read_bytes = read (sockfd, &buffer[i], 1);
	if (read_bytes < 0){
		clog_e(_NET_TAG, "read failed");
		return -1;
	}
	}
	clog_i(_NET_TAG, "data received: %s", buffer);
	return 0;
}

/** Read data from the given socket
 * @param sockfd The socket descriptor to read form
 */
FILE *read_file(int sockfd){
#if defined(__linux__) || defined(__APPLE__)
#if defined(__kernel_version_2_4)
	//recvfile(..,..,...);
#else
	//read(..,..,..);
#endif
#endif
	return NULL;
}

int disconnect_server(int sockfd){
	if(close(sockfd) == -1){
		clog_e(_NET_TAG, "Disconnection Unsuccesful");
        return -1;
	}
	else clog_i(_NET_TAG, "Disconnection Successful");
    return 0;
}

int connect_server (const char * hostname, int port){
	struct sockaddr_in serv_addr;
	struct hostent *server;
	//checking whether port is between 0 and 65536
	if (port < 0 || port > 65535){
		clog_e (_NET_TAG, "invalid port number, port number should be between 0 and 65536");
		return -1;
	}
	//Create socket
	int sockfd = socket(AF_INET , SOCK_STREAM , 0);
	if (sockfd == -1){
		clog_e(_NET_TAG, "Could not create socket");
		return -1;
	}
	clog_i(_NET_TAG, "Socket created");
	if((server = gethostbyname(hostname))==NULL){
		clog_e(_NET_TAG, "no such host found");
		return -1;
	}
	memset((char *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	memcpy((char*)&serv_addr.sin_addr.s_addr, (char*)server->h_addr, server->h_length);
	serv_addr.sin_port = htons( port );
	int i = 0;
	while (connect (sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1){
		if(i++ > CON_MAX_ATTEMPTS){
			//guess other hostnames for the user
			close(sockfd);
			clog_e(_NET_TAG, "cannot establish connection to %s on port %d", hostname, port);
			return -1;
		}
	}
	clog_i(_NET_TAG, "connection established successfully to %s on port %d", hostname, port);
	return sockfd;
}

#ifndef SERV_BACKLOG
#define SERV_BACKLOG 10
#endif


//TODO support multiple server options
/** Starts the server with the standard IPv4 and TCP stack
 * @param port Port number for the server to start
 * @return Socket descriptor of the started server
 */
int start_server(int port){
	static int cont;
	static int servfd;

	struct sockaddr_in server, client;
    	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( port );
	socklen_t cli_size = sizeof(struct sockaddr_in);

	if(cont == port){
		clog_i(_NET_TAG, "Connection accepted");
		return accept(servfd, (struct sockaddr *)&client, &cli_size);
	}
	if(cont == 0)
		cont = port;
	//Create socket
	servfd = socket(PF_INET , SOCK_STREAM , 0);
	if (servfd == -1){
		clog_e(_NET_TAG, "could not create socket");
		return -1;
	}
	//Bind
	if( bind(servfd,(struct sockaddr *)&server , sizeof(server)) < 0){
		clog_e(_NET_TAG, "bind failed");
		return -1;
	}
	//Listen
	listen(servfd , SERV_BACKLOG);
	//Accept and incoming connection
	clog_i(_NET_TAG, "Waiting for incoming connections...");
	//accept connection from an incoming client
	int clifd = accept(servfd, (struct sockaddr *)&client, &cli_size);
	if (clifd < 0){
		clog_i(_NET_TAG, "Accept failed");
		return -1;
	}
	clog_i(_NET_TAG, "Connection accepted");
	return clifd;
}
#endif
