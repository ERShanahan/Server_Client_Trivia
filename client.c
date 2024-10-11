#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/select.h>

// Define constants
#define buf 1024
#define PMIN 1024
#define PMAX 49151
#define DEFAULT_PORT 25555
#define DEFAULT_IP "127.0.0.1"

// Function to handle error checking and exit program on failure
void error_check(int e, char* error, int fd){
    if(e == -1){
        close(fd);
        perror(error);
        exit(EXIT_FAILURE);
    }
}

// Function to display help message
void help_msg(char* program_name){
    printf("Usage: %s [-i IP_address] [-p port_number] [-h]\n\n"
    "  -i IP_address\t\tDefault to \"127.0.0.1\";\n"
    "  -p port_number\tDefault to 25555;\n"
    "  -h\t\t\tDisplay this help info.\n", program_name);
}

// Function to display help message on error and exit
void exit_help(char* error_flag, char* program){
    fprintf(stderr, "Error: %s.\n", error_flag);
    help_msg(program);
    exit(EXIT_FAILURE);
}

// Function to set port number
int set_port(long* old_port, char* new_port){
    long p = atoi(new_port);
    if( p < PMIN || p > PMAX ){
        perror("port number");
        exit(EXIT_FAILURE);
    }

    *old_port = p;
    return 0;
}

// Main function
int main(int argc, char* argv[]) {

    // Initialize default IP address and port number
    char ip_addr[] = DEFAULT_IP;
    long port_number = DEFAULT_PORT;

    // Parse command line arguments
    int option;
    int argc_count = 1;
    const char* options = ":iph";
    while( (option = getopt(argc, argv, options)) != -1 ){
        switch(option){  
            case 'i':
                if(optind < argc){
                    strcpy(ip_addr, argv[optind]);
                    argc_count+=2;
                }else exit_help("i flag", argv[0]);
                break;
            case 'p':
                if(optind < argc){
                    set_port(&port_number, argv[optind]);
                    argc_count+=2;
                }else exit_help("p flag", argv[0]);
                break; 
            case 'h':
                help_msg(argv[0]);
                exit(EXIT_FAILURE);
            case '?':  
                printf("Error: Unknown option '-%c' received.\n", optopt); 
                exit(EXIT_FAILURE);
        }
    }

    // Check for additional arguments
    for(int i = argc_count; i < argc; i++){
        printf("Error: Additional argument passed '%s'.\n", argv[i]);
        help_msg(argv[0]);
        exit(EXIT_FAILURE);
    }

    int server_fd;
    struct sockaddr_in server_addr;
    socklen_t addr_size = sizeof(server_addr);

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    error_check(server_fd, "socket", server_fd);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(25555); // Default port number
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Default IP address

    // Connect to server
    error_check(connect(server_fd, (struct sockaddr *) &server_addr, addr_size), "connect", server_fd);

    // Set up file descriptors for select
    fd_set active_fds;
    int max_fd = server_fd;

    char buffer[buf];

    // Prompt user to enter name
    printf("Please type your name: "); fflush(stdout);
    scanf("%s", buffer);
    error_check(write(server_fd, buffer, strlen(buffer)), "write", server_fd);

    // Main loop to read and send messages
    while(1){

        // Clear file descriptors
        FD_ZERO(&active_fds);
        FD_SET(server_fd, &active_fds);
        FD_SET(STDIN_FILENO, &active_fds);

        // Wait for activity on file descriptors
        error_check(select(max_fd + 1, &active_fds, NULL, NULL, NULL), "select", server_fd);

        // Check if there's data to read from server
        if(FD_ISSET(server_fd, &active_fds)){
            int recvbytes = read(server_fd, buffer, buf);
            error_check(recvbytes, "read", server_fd);
            if (recvbytes == 0) break;
            else {
                buffer[recvbytes] = 0;
                printf("%s", buffer); fflush(stdout);
            }
        }else {
            // Read user input from stdin
            int recvbytes = read(STDIN_FILENO, buffer, buf);
            error_check(recvbytes, "read", server_fd);
            buffer[recvbytes] = '\0';
            // Send user input to server
            error_check(write(server_fd, buffer, strlen(buffer)), "write", server_fd);
        }
    }
    // Close socket and exit
    close(server_fd);
    return 0;
}
