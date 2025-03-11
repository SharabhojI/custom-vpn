#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFLEN   512                       // Maximum length of buffer
#define PORT     9988                      // Fixed server port number
#define TUN_NAME "vpn0"                    // Name for TUN interface
#define MTU      1500                      // Standard MTU size

// Function to create and configure TUN interface
int tun_create(char *dev_name) {
    struct ifreq ifr;
    int fd;

    // Open the clone device
    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        perror("Opening /dev/net/tun failed");
        return fd;
    }

    // Set up interface request structure
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;  // TUN device, no packet info
    if (dev_name)
        strncpy(ifr.ifr_name, dev_name, IFNAMSIZ);

    // Create the interface
    if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) {
        perror("ioctl TUNSETIFF failed");
        close(fd);
        return -1;
    }

    return fd;
}

// Function to inspect and log IP packet details
void log_ip_packet(unsigned char *buffer, int length) {
	// Ensure packet meets min IP header length
	if (length < 20) {
		printf("IP packet length too short\n");
		return;
	}

	// Get IP version (right shift 4 bits and mask) 
	unsigned char version = (buffer[0] >> 4) &0xF;

	// Extract IP protocol version
	unsigned char ptcl = buffer[9];

	// Extract and print source and dest addresses along with version,
	// protocol, and packet length
	printf("IPv%d Packet: %d.%d.%d.%d -> %d.%d.%d.%d (Protocol: %d, Length: %d)\n",
		version,
		buffer[12], buffer[13], buffer[14], buffer[15], // Source IP address
		buffer[16], buffer[17], buffer[18], buffer[19],	// Dest IP address
		ptcl, length);
}

int main(void)
{
    // Declarations and definitions
    int listen_sock;                                    // Listen socket descriptor
    int sock;                                           // Socket descriptor
    int tun_fd;                                         // TUN interface file descriptor
    char buffer[BUFLEN];                                // Data buffer
    char tun_buffer[MTU];                               // Buffer for TUN packets
    char* ptr = buffer;                                 // Pointer to data buffer
    int len = 0;                                        // Number of bytes to send/receive
    int max_len = sizeof(buffer);                       // Maximum number of bytes to receive
    int n = 0;                                          // Number of bytes for each recv call
    int wait_size = 16;                                 // Size of waiting clients
    struct sockaddr_in server_address;                  // Data structure for server address
    struct sockaddr_in client_address;                  // Data structure for client address
    int client_address_len = sizeof(client_address);    // Length of client address

    // Create TUN interface
    printf("Creating TUN interface %s...\n", TUN_NAME);
    if ((tun_fd = tun_create(TUN_NAME)) < 0) {
        printf("Error creating TUN interface\n");
        exit(1);
    }
    printf("TUN interface created successfully\n");
    
    // Set up IP address and bring up interface
    char cmd[100];
    sprintf(cmd, "ip addr add 10.0.0.1/24 dev %s", TUN_NAME);
    system(cmd);
    sprintf(cmd, "ip link set %s up", TUN_NAME);
    system(cmd);

    // Populate socket address for the server
    printf("Populating server socket address..\n");
    memset(&server_address, 0, sizeof(server_address));   // Initialize server address data structure
    server_address.sin_family = AF_INET;                  // Populate family field - IPV4 protocol
    server_address.sin_port = htons(PORT);                // Set port number
    server_address.sin_addr.s_addr = INADDR_ANY;          // Set IP address to IPv4 value for localhost
    
    // Print server information (IP and port)
    char* ip = inet_ntoa(server_address.sin_addr);
    int port = ntohs(server_address.sin_port);
    printf("IP Address: %s Port: %d\n", ip, port);

    // Create a TCP socket; returns -1 on failure
    printf("Creating TCP socket..\n");
    if ((listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Error: Listen socket failed!\n");
        exit(1);
    }
    else
        printf("Socket successfully created..\n");

    // Bind the socket to the server address; returns -1 on failure
    printf("Binding socket to server address..\n");
    if ((bind(listen_sock, (struct sockaddr*)&server_address, sizeof(server_address))) == -1) {
        printf("Error: binding failed!\n");
        exit(1);
    }
    else
        printf("Socket successfully bound..\n");
    
    // Listen for connections...
    printf("Attempting to listen for client connections..\n");
    if (listen(listen_sock, wait_size) == -1) {
        printf("Error: listening failed!\n");
        exit(1);
    }    
    else
        printf("Server is listening for connections..\n");

    // Handle the connection (run forever)
    for(;;) {    
        // Accept connections from client...
        if ((sock = accept(listen_sock, (struct sockaddr*)&client_address, &client_address_len)) == -1) {
            printf("Error: accepting failed!\n");
            exit(1);
        }
        else
            printf("Server has accepted a client connection..\n");

        // Variables for select()
        fd_set readfds;
        struct timeval tv;
        int maxfd = (sock > tun_fd) ? sock : tun_fd;

        // Handle messages from this client until they disconnect
        while(1) {
            // Set up file descriptors for select()
            FD_ZERO(&readfds);
            FD_SET(tun_fd, &readfds);
            FD_SET(sock, &readfds);
            
            // Set timeout to 1 second
            tv.tv_sec = 1;
            tv.tv_usec = 0;
            
            // Wait for activity on any descriptor
            int activity = select(maxfd + 1, &readfds, NULL, NULL, &tv);
            
            if (activity < 0) {
                printf("Select error\n");
                break;
            }
            
            // Check for data from TUN interface
            if (FD_ISSET(tun_fd, &readfds)) {
                n = read(tun_fd, tun_buffer, MTU);
                if (n > 0) {
                    log_ip_packet((unsigned char*)tun_buffer, n);
                    printf("Received %d bytes from TUN\n", n);
                    send(sock, tun_buffer, n, 0);
                }
            }
            
            // Check for data from client socket
            if (FD_ISSET(sock, &readfds)) {
                n = recv(sock, buffer, BUFLEN, 0);
                
                if (n <= 0) {
                    if (n < 0) {
                        printf("Error receiving data\n");
                    } else {
                        printf("Client disconnected\n");
                    }
                    break;
                }
                
                log_ip_packet((unsigned char*)buffer, n);
                printf("Writing %d bytes to TUN\n", n);
                write(tun_fd, buffer, n);
            }
        }

        // Client disconnected, close their socket
        printf("Closing client socket..\n");
        close(sock);
    }

    // Close sockets and TUN interface
    printf("Closing server listen socket and TUN interface..\n");
    close(listen_sock);
    close(tun_fd);
    
    return 0;
}
