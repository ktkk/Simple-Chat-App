#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdbool.h>
#include <signal.h>
#include <limits.h>

#define PORT 9000
#define MAX_CLIENTS 5
#define MESSAGE_SIZE 2000
#define CLIENT_NAME_SIZE 32

typedef struct {
    int fd;
    char name[CLIENT_NAME_SIZE];
} Clients_s;

fd_set server_socket, tmp_socket;

Clients_s clients[MAX_CLIENTS+10] = {0}; // To avoid having to loop over the entire array,
                                          // each client's info will be stored at the index == the client's fd
                                          // which requires the array be slightly bigger 
                    
static volatile sig_atomic_t run = 1;

int server_fd;
int clients_size = 0;
int first_client = INT_MAX;
int last_client  = INT_MIN;

char close_client_message[] = "Client Exiting. CLOSE!";


static void intHandler(sig_atomic_t value);


void send_to_everyone_except(int sender, char message[]) {
    for ( int i = first_client; i <= last_client; i++ ) 
        if ( i != sender )
            send( i, message, strlen(message), 0 );
}


void handle_new_connection(struct sockaddr_in server_address) {
    int new_client = 0;
    int listener = listen(server_fd, 5);
    int addrlen = sizeof(server_address);
        
    char max_clients_message[] = "Maximum clients reached. Disconnecting\n";

    if (listener < 0) {
        perror("error in listen\n");
        exit(EXIT_FAILURE);
	} else if ( listener == 0 ) {
		new_client = accept( server_fd, (struct sockaddr *)&server_address, &addrlen );
	} 

    if ( clients_size == MAX_CLIENTS ) {
        send( new_client, max_clients_message, sizeof(max_clients_message), 0 );
        close(new_client);
        return;
    }

	if (new_client < 0) {
		perror("error in accept\n");
		exit(EXIT_FAILURE);
	} else if ( new_client > 0 ) {
		char name[CLIENT_NAME_SIZE] = {0};
		recv( new_client, name, sizeof(name), 0 );

        if ( strcmp(name, close_client_message) == 0 ) {
            close(new_client);
            printf("new client tried to join and disconnected\n");
            return;
        }
		

        FD_SET( new_client, &server_socket );

        clients[new_client].fd = new_client;
        strcpy( clients[new_client].name, name ); 

        if ( new_client < first_client ) first_client = new_client;
        if ( new_client > last_client  ) last_client  = new_client;

        clients_size++;

		printf("Client %s has joined\n", clients[new_client].name);
	
	}
}

void remove_client(int leaver) {

    char message[MESSAGE_SIZE] = {0};
    char has_left[] = " has left\n";

    strncat( message, clients[leaver].name, strlen(clients[leaver].name) );
    strncat( message, has_left            , strlen(has_left)             );

    send_to_everyone_except(leaver, message);
    printf("%s", message);

	FD_CLR(leaver, &server_socket);
	close(leaver);

	clients[leaver].fd = 0;
	memset( clients[leaver].name, 0, sizeof(clients[leaver].name) );

	clients_size--;
}


void run_server(struct sockaddr_in server_address) {
    struct timeval tv;
    int valread, message_size;
    
    while(run) {
        tmp_socket = server_socket;
        tv.tv_sec  = 1;
        
        char buffer[MESSAGE_SIZE]  = {0};
        char message[MESSAGE_SIZE] = {0};

        int n = 20; 
        if ( select(n, &tmp_socket, NULL, NULL, &tv) < 0 ) {
	        printf("error in select\n");
	        exit(EXIT_FAILURE);
        }

	    for ( int fd = 0; fd < n; fd++ ) {
	        if ( FD_ISSET(fd, &tmp_socket) ) {

	            if ( fd == server_fd ) {
                    handle_new_connection(server_address);
                    continue;
                }

	            valread = read( fd , buffer, sizeof(buffer) );
	            if (valread > 0) {
	                char name[CLIENT_NAME_SIZE] = {0};
                    strcpy(name, clients[fd].name);

		            if ( strcmp(buffer, close_client_message) == 0 ) {
                        remove_client(fd);
                        break;
		            }

				    printf("%s: %s\n",name, buffer);
		                
		            strncat( message, name  , strlen(name)   );
		            strncat( message, ": "  , 3              );
		            strncat( message, buffer, strlen(buffer) );
		
                    send_to_everyone_except(fd, message);
	            }
	            
	        }
	    }
    }

}


int main() {
    struct sockaddr_in server_address;
    struct sigaction act;

    act.sa_handler = intHandler;

	
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == 0)	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons( PORT );
	
	if ( bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0 ) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

    FD_ZERO(&server_socket);
    FD_SET(server_fd, &server_socket);

    run_server(server_address);

    sigaction(SIGINT, &act, NULL);

	return 0;
}


static void intHandler(sig_atomic_t value) {
    (void)value;
    run = 0;    
    close(server_fd);
}
