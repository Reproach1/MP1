#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "interface.h"

#define PORT 6969

struct Room {
	char name[10];
	int sockfd;
	int members;
	int port;
};

struct Room rooms[256];
int active_rooms = 0;


void *process_command(void *s);
void run_room(int room_num);
void create_room(int port, char *name);


int main(int argc, char** argv) 
{
	int server_fd, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == 0) {
		perror("socket()");
		exit(EXIT_FAILURE);
	}
	
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
		perror("setsocketopt()");
		exit(EXIT_FAILURE);
	}
	
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);
	
	if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		perror("bind()");
		exit(EXIT_FAILURE);
	}
	
	while (1) {
		listen(server_fd, 10);
		
		new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
		if (new_socket < 0) {
			perror("accept()");
			exit(EXIT_FAILURE);
		}
		
		//printf("new connection\n");
		
		pthread_t thread_id;
		void *s = &new_socket;
		pthread_create(&thread_id, NULL, process_command, s);
		//pthread_join(thread_id, NULL);
	}
	
	close(server_fd);
	
    return 0;
}


void *process_command(void *s) {
	int sockfd = *(int *)s;
	char buffer[MAX_DATA], command[MAX_DATA], param[MAX_DATA];
	struct Reply reply;
	
	read(sockfd, buffer, MAX_DATA);
	
	printf("%s\n", buffer);
	
	int pos = -1;
	for (int i = 0; i < MAX_DATA; ++i) {
		if (buffer[i] == ' ') {
			pos = i+1;
		}
		else if (pos == -1) {
			command[i] = buffer[i];
		}
		else {
			param[i - pos] = buffer[i];
		}
	}
	
	
	if (strncmp(command, "CREATE", 6) == 0) {
		int exists = 0;
		printf("%d\n", active_rooms);
		for (int i = 0; i < active_rooms; ++i) {
			if (strncmp(rooms[i].name, param, 10) == 0) {
				reply.status = FAILURE_ALREADY_EXISTS;
				send(sockfd, (char *)&reply, sizeof(struct Reply), 0);
				exists = 1;
				break;
			}
		}
		
		if (exists == 0) {
			if (active_rooms == 0) {
				reply.port = PORT + 1;
			}
			else {
				reply.port = rooms[active_rooms - 1].port + 1;
			}
			
			reply.num_member = 0;
			reply.status = SUCCESS;
			send(sockfd, (char *)&reply, sizeof(struct Reply), 0);
			
			create_room(reply.port, param);
			
		}
	}
	else if (strncmp(command, "JOIN", 4) == 0) {
		int exists = 0;
		for (int i = 0; i < active_rooms; ++i) {
			if (strncmp(rooms[i].name, param, 10) == 0) {
				rooms[i].members++;
				
				reply.status = SUCCESS;
				reply.port = rooms[i].port;
				reply.num_member = rooms[i].members;
				
				exists = 1;
				break;
			}
		}
		
		if (exists == 0) {
			reply.status = FAILURE_NOT_EXISTS;
		}
		
		send(sockfd, (char *)&reply, sizeof(struct Reply), 0);
	}
	else if (strncmp(command, "DELETE", 6) == 0) {
		
	}
	else {
		
	}
	
	
	printf("Closing temp socket\n");
	close(sockfd);
	
	pthread_detach(pthread_self());
	
}

void create_room(int port, char *name) {
	int fd, message_sock;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == 0) {
		perror("socket()");
		exit(EXIT_FAILURE);
	}
	
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
		perror("setsocketopt()");
		exit(EXIT_FAILURE);
	}
	
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);
	
	if (bind(fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		perror("bind()");
		exit(EXIT_FAILURE);
	}
	
	printf("%d\n", fd);
	printf("%d\n", port);
	
	rooms[active_rooms].sockfd = fd;
	rooms[active_rooms].port = port;
	rooms[active_rooms].members = 0;
	strcpy(rooms[active_rooms].name, name);
	
	active_rooms++;
	
	char buffer[MAX_DATA];
	
	while (1) {
		if (listen(fd, 5) == -1) {
			perror("listen()");
			exit(EXIT_FAILURE);
		}
		
		message_sock = accept(fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
		if (message_sock < 0) {
			perror("accept()");
			exit(EXIT_FAILURE);
		}
			
		if (read(message_sock, buffer, sizeof(buffer)) < 0) {
			perror("read()");
			exit(EXIT_FAILURE);
		}
		
		display_message(buffer);
			
		if (send(message_sock, buffer, sizeof(buffer), 0) < 0) {
			perror("send()");
			exit(EXIT_FAILURE);
		}
		
		close(message_sock);
	}
	
	active_rooms--;
	
	close(fd);
}