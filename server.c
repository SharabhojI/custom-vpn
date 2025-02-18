#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
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

        // Handle messages from this client until they disconnect
        while(1) {
            // Reset variables for new message
            ptr = buffer;
            len = 0;
            max_len = sizeof(buffer);
            memset(buffer, 0, sizeof(buffer));

            // Read from TUN interface
            n = read(tun_fd, tun_buffer, MTU);
            if (n > 0) {
                // Got packet from TUN, send to client
                printf("Received %d bytes from TUN\n", n);
                send(sock, tun_buffer, n, 0);
            }

            // Receive data from client...
            printf("Receiving data from client..\n");
            n = recv(sock, ptr, max_len, 0);
            
            // Check for client disconnect or error
            if (n <= 0) {
                if (n < 0) {
                    printf("Error receiving data\n");
                } else {
                    printf("Client disconnected\n");
                }
                break;
            }

            // Write received data to TUN interface
            printf("Writing %d bytes to TUN\n", n);
            write(tun_fd, buffer, n);
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
