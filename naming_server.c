#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>

// File to store the Naming Server's IP and Port
#define NAMING_SERVER_INFO_FILE "naming_server_info.txt"

// Function to get the local IP address of the machine dynamically
char* get_local_ip() {
    static char IPbuffer[256];  // Use static to return the address
    char hostbuffer[256];
    struct hostent *host_entry;

    // Get the hostname
    if (gethostname(hostbuffer, sizeof(hostbuffer)) == -1) {
        perror("gethostname");
        exit(EXIT_FAILURE);
    }

    // Get the host information using the hostname
    host_entry = gethostbyname(hostbuffer);
    if (host_entry == NULL) {
        perror("gethostbyname");
        exit(EXIT_FAILURE);
    }

    // Convert the network address to a string (IPv4 only)
    strcpy(IPbuffer, inet_ntoa(*(struct in_addr *) host_entry->h_addr_list[0]));
    return IPbuffer;
}

// Function to write the Naming Server's IP and port to a file
void write_naming_server_info(const char* ip, int port) {
    FILE *file = fopen(NAMING_SERVER_INFO_FILE, "w");
    if (file == NULL) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    // Write IP and port to the file
    fprintf(file, "IP:%s\nPort:%d\n", ip, port);
    fclose(file);
    printf("Naming Server information written to file: %s\n", NAMING_SERVER_INFO_FILE);
}

// Function to initialize the Naming Server
void initialize_naming_server() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    
    // Get the local IP dynamically
    char *local_ip = get_local_ip();

    // Creating a socket file descriptor (TCP)
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attach socket to the port 9000
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;  // Bind to any available interface
    address.sin_port = htons(9000);  // Fixed port

    // Bind the socket to the IP address and the port
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Output the IP address and port number of the Naming Server
    printf("Naming Server initialized at IP: %s, Port: %d\n", local_ip, ntohs(address.sin_port));

    // Write the IP and port to the file
    write_naming_server_info(local_ip, ntohs(address.sin_port));

    // Start listening for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Accept incoming connections from clients or storage servers
    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Read data sent by the client/storage server
        read(new_socket, buffer, 1024);
        printf("Received registration: %s\n", buffer);

        // Send response back to the client/storage server
        send(new_socket, "Registration successful", strlen("Registration successful"), 0);
        printf("Response sent to client/storage server\n");

        // Close the connection with the client/storage server
        close(new_socket);
    }
}

int main(int argc, char const *argv[]) {
    initialize_naming_server();
    return 0;
}
