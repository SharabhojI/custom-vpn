#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main (int argc, char* argv[ ])		// Three arguments to be checked later
{
	// Declarations and definitions
	int sock;				// Socket descriptor
	int n = 0;				// Number of bytes in each recv call
	struct sockaddr_in servAddr;		// Server socket address data structure
	char* servIP = argv[1];			// Server IP address from command line
	int servPort = atoi(argv[2]);		// Server port number from command line
	char* message = argv[3];		// Message specified on the command line
	int len = 0;				// Length of string to be echoed
	char buffer[512 + 1];			// Buffer
	char* ptr = buffer;			// Pointer to buffer
	int max_len = sizeof(buffer);		// Maximum number of bytes to send

	// Check for correct number of command line arguments
	if(argc != 4) {                                               
		printf("client [IP address] [Port] [Message]\n");                             
		exit (1);
	}                                                

	// Populate socket address for the server
	printf("Populating server socket address..\n");
	memset (&servAddr, 0, sizeof(servAddr));	// Initialize data structure
	servAddr.sin_family = AF_INET; 			// This is an IPv4 address
	servAddr.sin_addr.s_addr = inet_addr(servIP); 	// Server IP address
	servAddr.sin_port = htons(servPort);		// Server port number
	
	// Create a TCP socket stream
	printf("Creating TCP socket stream..\n");
	if ((sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error: socket creation failed!\n");
		exit (1);
	}
	else
 		printf("Socket successfully created..\n"); 

	// Connect to the server
	printf("Connecting to the server..\n");
	if ((connect (sock, (struct sockaddr*)&servAddr, sizeof(servAddr))) == -1) {
		printf("Error: connection to the server failed!\n");
		exit (1);
	}
	else
        	printf("Connected to the server..\n"); 
	
	// Send data to the server...
	printf("Sending message to server: %s\n", message);
	send(sock, message, strlen(message), 0);

	// Reset variables before receiving
	ptr = buffer;
	len = 0;
	max_len = sizeof(buffer);
	memset(buffer, 0, sizeof(buffer));

	// Receive data back from the server..
	printf("Waiting for server to echo back message..\n");
	while ((n = recv(sock, ptr, max_len, 0)) > 0) {		// Loop while receiving data... 
		ptr += n;					// Move pointer along the buffer
		max_len -= n;					// Adjust maximum number of bytes
		len += n;					// Update length of string received

		// print data...
		buffer[len] = '\0'; 				// Null terminate
		printf("Echoed string received: %s\n", buffer);	// Print buffer
	}

	// Close socket
	printf("Closing client connection..\n");
	close (sock);
	
	// Stop program
	printf("Stopping program..\n");
	exit (0);
	
}
