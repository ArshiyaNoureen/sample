#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>

// File to read Naming Server's IP and Port
#define NAMING_SERVER_INFO_FILE "naming_server_info.txt"

// Function to get the local IP address dynamically
char* get_local_ip() {
    char hostbuffer[256];
    char *IPbuffer;
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
    IPbuffer = inet_ntoa(*(struct in_addr *) host_entry->h_addr_list[0]);
    if (IPbuffer == NULL) {
        perror("inet_ntoa");
        exit(EXIT_FAILURE);
    }

    return IPbuffer;
}

// Function to read the Naming Server's IP and Port from the file
void read_naming_server_info(char* ip, int* port) {
    FILE *file = fopen(NAMING_SERVER_INFO_FILE, "r");
    if (file == NULL) {
        perror("Failed to open Naming Server info file");
        exit(EXIT_FAILURE);
    }

    // Read IP and Port from the file
    fscanf(file, "IP:%s\nPort:%d\n", ip, port);
    fclose(file);
    printf("Read Naming Server information: IP = %s, Port = %d\n", ip, *port);
}

// Function to initialize the client connection server (for interacting with clients)
int initialize_client_server(int *client_port) {
    int client_server_fd;
    struct sockaddr_in address;
    int opt = 1;

    // Create a socket for client-server communication (TCP)
    if ((client_server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attach socket to the port 0 (dynamic port assignment)
    if (setsockopt(client_server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(0);  // Let OS assign the port dynamically

    // Bind the socket to a dynamic port for client communication
    if (bind(client_server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Get the dynamically assigned port number for client-server communication
    socklen_t len = sizeof(address);
    if (getsockname(client_server_fd, (struct sockaddr*)&address, &len) == -1) {
        perror("getsockname");
        exit(EXIT_FAILURE);
    }

    *client_port = ntohs(address.sin_port);  // Store the dynamic client port

    return client_server_fd;
}

// Function to register SS_1 with the Naming Server
void register_with_naming_server(char* nm_ip, int nm_port, char* ss_ip, int nm_ss_port, int client_port, char* paths) {
    struct sockaddr_in nm_address;
    int sock = 0;
    char message[1024];

    // Create socket for communication with Naming Server (TCP)
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error\n");
        return;
    }

    nm_address.sin_family = AF_INET;
    nm_address.sin_port = htons(nm_port);

    // Convert the Naming Server's IP address from text to binary form
    if (inet_pton(AF_INET, nm_ip, &nm_address.sin_addr) <= 0) {
        printf("Invalid address or Address not supported\n");
        return;
    }

    // Connect to the Naming Server
    if (connect(sock, (struct sockaddr *)&nm_address, sizeof(nm_address)) < 0) {
        printf("Connection to Naming Server failed\n");
        return;
    }

    // Prepare the registration message (SS IP, NM Port, Client Port, Paths)
    snprintf(message, sizeof(message), "REGISTER SS IP:%s NM_Port:%d Client_Port:%d Paths:%s", ss_ip, nm_ss_port, client_port, paths);

    // Send the registration message to the Naming Server
    send(sock, message, strlen(message), 0);
    printf("Registration message sent to Naming Server: %s\n", message);

    // Close the socket
    close(sock);
}

int main() {
    char nm_ip[256];  // Buffer to store Naming Server's IP
    int nm_port;      // Variable to store Naming Server's port
    int client_port;  // Variable to store client-server port
    int nm_ss_port = 8000;  // Port number for NM_SS communication
    char *ss_ip = get_local_ip();  // Get local IP of Storage Server
    char *paths = "/A/B,/A/C";     // Storage paths

    // Initialize client-server socket and get the dynamic port number
    int client_server_fd = initialize_client_server(&client_port);

    // Read Naming Server's IP and Port from file
    read_naming_server_info(nm_ip, &nm_port);

    // Register the Storage Server with the Naming Server
    register_with_naming_server(nm_ip, nm_port, ss_ip, nm_ss_port, client_port, paths);

    // Rest of the client-server logic

    return 0;
}
