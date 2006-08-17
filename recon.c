#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "data.h"

#define MAXCHAR 4096
#define PORT 1967
#define S3D_PORT 1968
#define ADDR_STR_LEN 16
#define PACKET_FIELDS 5

static int on = 1;

void *node_server (void *srv_addr)
{
	char recive_dgram[MAXCHAR];
	char addr_str[ADDR_STR_LEN];
	
	struct sockaddr_in server, client;
	int sock, n, packet_count,i;
	socklen_t len;
	
	sock = socket(PF_INET, SOCK_DGRAM,0 );

	memset( &server, 0, sizeof (server));
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	inet_pton(AF_INET, (char *)srv_addr,&server.sin_addr);

	if(server.sin_addr.s_addr == INADDR_NONE)
	{
		printf("invalid adress %s", (char *) srv_addr);
		exit(EXIT_FAILURE);
	}

	if(sock < 0)
	{
		printf("Cannot create socket => %s\n",strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) < 0)
	{
		printf("Cannot enable ip: %s\n", strerror(errno));
		close(sock);
		exit(EXIT_FAILURE);			
	}
	
	if(bind( sock, (struct sockaddr*)&server, sizeof(server)) < 0)
	{
		printf("Error by bind => %s\n",strerror(errno));
		close(sock);
		exit(EXIT_FAILURE);
	}

	while(1)
	{
		int orig;
		len = sizeof(client);
		n = recvfrom(sock, recive_dgram, sizeof(recive_dgram), 0, (struct sockaddr*) &client, &len);
		addr_to_string(client.sin_addr.s_addr, addr_str, sizeof(addr_str));
		
		packet_count = n / PACKET_FIELDS;
		for( i=0;i < packet_count; i++)
		{
			memmove(&orig,(unsigned int*)&recive_dgram[i*PACKET_FIELDS],4);
			handle_node(orig,client.sin_addr.s_addr,(unsigned char)recive_dgram[i*PACKET_FIELDS+4]);		
		}
		/*print_data();*/
	}
	close(sock);
	return NULL;
}

int main(int argc, char *argv[])
{
	pthread_t thread_node_server;
	pthread_t thread_node_cleaner;
	
	struct sockaddr_in adr_serv, adr_client;
	int n, len_inet, serv_socket, clnt_socket;
	char buffer[MAXCHAR];

	if(argc < 2)
	{
		fprintf(stderr,"Usage: recon <ip>\n");
		return(EXIT_FAILURE);
	}

	pthread_create(&thread_node_server, NULL, &node_server, argv[1]);
	pthread_create(&thread_node_cleaner, NULL, &node_cleaner, NULL);
	fprintf( stderr, "main: thread-id: %u\n",(unsigned int) pthread_self());
	fprintf( stderr, "main: run node_server thread-id: %u\n", (unsigned int)thread_node_server);
	fprintf( stderr, "main: run node_cleaner thread-id: %u\n", (unsigned int)thread_node_cleaner);
	
	serv_socket = socket(PF_INET, SOCK_STREAM, 0);
	memset(&adr_serv, 0, sizeof(adr_serv));
	adr_serv.sin_family = AF_INET;
	adr_serv.sin_port = htons(S3D_PORT);
	inet_pton(AF_INET, argv[1],&adr_serv.sin_addr);

	if(adr_serv.sin_addr.s_addr == INADDR_NONE)
	{
		printf("invalid adress %s", argv[1]);
		exit(EXIT_FAILURE);
	}

	if(serv_socket < 0)
	{
		printf("Cannot create socket => %s\n",strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	if(setsockopt(serv_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) < 0)
	{
		printf("Cannot enable ip: %s\n", strerror(errno));
		close(serv_socket);
		exit(EXIT_FAILURE);			
	}
	
	if(bind( serv_socket, (struct sockaddr*)&adr_serv, sizeof(adr_serv)) < 0)
	{
		printf("Error by bind => %s\n",strerror(errno));
		close(serv_socket);
		exit(EXIT_FAILURE);
	}
	
	if(listen(serv_socket,15) < 0)
	{
		printf("Error by listen => %s\n",strerror(errno));
		close(serv_socket);
		exit(EXIT_FAILURE);
	}
	printf("...accept connections on port %d\n", ntohs(adr_serv.sin_port));
	while(1)
	{
		len_inet = sizeof(adr_client);
		if((clnt_socket = accept(serv_socket, (struct sockaddr*)&adr_client,&len_inet)) == -1)
		{
			fprintf(stderr,"accept-error\n");
		}
	}
	return EXIT_SUCCESS;
}

