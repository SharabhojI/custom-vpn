#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define	BUFLEN	512						// Maximum length of buffer
#define PORT	9988						// Fixed server port number

int main (void)
{
	// Declarations and definitions
	int listen_sock;					// Listen socket descriptor
	int sock;						// Socket descriptor
	char buffer[BUFLEN];					// Data buffer
	char* ptr = buffer;					// Pointer to data buffer
	int len = 0;						// Number of bytes to send/receive
	int max_len = sizeof(buffer);				// Maximum number of bytes to receive
	int n = 0;						// Number of bytes for each recv call
	int wait_size = 16;					// Size of waiting clients
	struct sockaddr_in server_address;			// Data structure for server address
	struct sockaddr_in client_address;			// Data structure for client address
	int client_address_len = sizeof(client_address);	// Length of client address

	// Populate socket address for the server
	printf("Populating server socket address..\n");
	memset (&server_address, 0, sizeof(server_address));	// Initialize server address data structure
	server_address.sin_family = AF_INET; 			// Populate family field - IPV4 protocol
	server_address.sin_port = htons(PORT);			// Set port number
	server_address.sin_addr.s_addr = INADDR_ANY;		// Set IP address to IPv4 value for loacalhost
	
	// Print server information (IP and port)
	char* ip = inet_ntoa(server_address.sin_addr);
	int port = ntohs(server_address.sin_port);
	printf("IP Address: %s Port: %d\n", ip, port);

	// Create a TCP socket; returns -1 on failure
	printf("Creating TCP socket..\n");
	if ((listen_sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error: Listen socket failed!\n");
		exit(1);
	}
	else
		printf("Socket successfully created..\n");

	// Bind the socket to the server address; returns -1 on failure
	printf("Binding socket to server address..\n");
	if ((bind(listen_sock, (struct sockaddr *)&server_address, sizeof(server_address))) == -1) {
		printf("Error: binding failed!\n");
		exit(1);
	}
	else
		printf("Socket successfully bound..\n");
	
	// Listen for connections...
	printf("Attempting to to listen for client connections..\n");
	if (listen(listen_sock, wait_size) == -1) {
		printf("Error: listening failed!\n");
		exit(1);
	}	
	else
		printf("Server is listening for connections..\n");

	// Handle the connection (run forever)
	for(;;) {	
		// Accept connections from client...
		if ((sock = accept(listen_sock, (struct sockaddr *)&client_address, &client_address_len)) == -1) {
			printf("Error: accepting failed!\n");
			exit(1);
		}
		else
			printf("Server has accepted a client connection..\n");

		// Reset variables for new connection
		ptr = buffer;
		len = 0;
		max_len = sizeof(buffer);
		memset(buffer, 0, sizeof(buffer));

		// Receive and format the data...
		printf("Receiving data from client..\n");
		while ((n = recv(sock, ptr, max_len, 0)) > 0) {
			// Print received data
			buffer[n] = 0;		// Null terminate
			printf("Server received: %s\n", buffer);

			ptr += n;			// Move pointer along buffer
			max_len -= n;			// Adjust maximum number of bytes to receive
			len += n;			// Update number of bytes received

			// Echo data back to the client...
			send(sock, buffer, len, 0);	// Echo received bytes
			close(sock);			// Close the socket
		}

		// Echo data back to the client...
		printf("Echoing data back to client: %s\n", buffer);
		send(sock, buffer, len, 0);		// Echo received bytes
		printf("Closing socket..\n");
		close(sock);				// Close the socket

	}		

	printf("Closing server listen socket..\n");
	close (listen_sock);				// Close descriptor referencing server socket
	
}
