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

#define BUFLEN  512                       // Maximum length of buffer
#define TUN_NAME "vpn0"                   // Name for our TUN interface
#define MTU     1500                      // Standard MTU size

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

int main (int argc, char* argv[ ])       // Two arguments needed: IP and port
{
    // Declarations and definitions
    int sock;                            // Socket descriptor
    int tun_fd;                          // TUN interface file descriptor
    int n = 0;                           // Number of bytes in each recv call
    struct sockaddr_in servAddr;         // Server socket address data structure
    char* servIP = argv[1];              // Server IP address from command line
    int servPort = atoi(argv[2]);        // Server port number from command line
    char buffer[BUFLEN + 1];             // Buffer for TCP data
    char tun_buffer[MTU];                // Buffer for TUN packets
    char* ptr = buffer;                  // Pointer to buffer
    int max_len = sizeof(buffer);        // Maximum number of bytes to send

    // Check for correct number of command line arguments
    if(argc != 3) {                                               
        printf("client [IP address] [Port]\n");                             
        exit (1);
    }                                                

    // Create TUN interface
    printf("Creating TUN interface %s...\n", TUN_NAME);
    if ((tun_fd = tun_create(TUN_NAME)) < 0) {
        printf("Error creating TUN interface\n");
        exit(1);
    }
    printf("TUN interface created successfully\n");

    // Populate socket address for the server
    printf("Populating server socket address..\n");
    memset (&servAddr, 0, sizeof(servAddr));         // Initialize data structure
    servAddr.sin_family = AF_INET;                   // This is an IPv4 address
    servAddr.sin_addr.s_addr = inet_addr(servIP);    // Server IP address
    servAddr.sin_port = htons(servPort);             // Server port number
    
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
    
    // Main communication loop
    while(1) {
        // Read from TUN interface
        n = read(tun_fd, tun_buffer, MTU);
        if (n > 0) {
            // Get packet from TUN and send to server
            printf("Received %d bytes from TUN\n", n);
            send(sock, tun_buffer, n, 0);
        }

        // Reset variables before receiving
        ptr = buffer;
        memset(buffer, 0, sizeof(buffer));

        // Receive response from the server..
        printf("Waiting for server response..\n");
        n = recv(sock, buffer, max_len, 0);
        if(n <= 0) {
            if(n < 0) {
                printf("Error receiving data\n");
            } else {
                printf("Server disconnected\n");
            }
            break;
        }

        // Write received data to TUN interface
        printf("Writing %d bytes to TUN\n", n);
        write(tun_fd, buffer, n);
    }

    // Close socket and TUN interface
    printf("Closing client connection and TUN interface..\n");
    close(sock);
    close(tun_fd);
    
    // Stop program
    printf("Stopping program..\n");
    exit (0);
}
