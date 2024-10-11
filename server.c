// Ethan Shanahan
// I pledge my honor that I have abided by the Stevens Honor System.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>

// Define constants
#define buf 1024
#define big_buf 2048
#define MAX_CONN 3
#define PMIN 1024
#define PMAX 49151
#define DEFAULT_PORT 25555
#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_FILE "questions.txt"

// Structure to hold a single entry in the questions file
struct Entry {
    char prompt[1024];
    char options[3][50];
    int answer_idx;
};

// Structure to hold player information
struct player {
    int fd;
    int score;
    char name[128];
};

// Function to close all connections
void close_all(int server, struct player* players){
    close(server);
    for(int i = 0; i < MAX_CONN; i++) if(players[i].fd != -1) close(players[i].fd);
}

// Function to handle error checking
void error_check(int e, char* error, int fd, struct player* players){
    if(e == -1){
        if(fd != -1 && players != NULL) close_all(fd, players);
        perror(error);
        exit(EXIT_FAILURE);
    }
}

// Function to display help message
void help_msg(char* program){
    printf("Usage: %s [-f question_file] [-i IP_address] [-p port_number] [-h]\n\n"
    "  -f question_file\tDefault to \"question.txt\";\n"
    "  -i IP_address\t\tDefault to \"127.0.0.1\";\n"
    "  -p port_number\tDefault to 25555;\n"
    "  -h\t\t\tDisplay this help info.\n", program);
}

// Function to display help message on error and exit
void exit_help(char* error_flag, char* program){
    fprintf(stderr, "Error: %s.\n", error_flag);
    help_msg(program);
    exit(EXIT_FAILURE);
}

// Function to send a message to all players
void send_all(char* string, struct player* players){
    for(int i = 0; i < MAX_CONN; i++){
        error_check(write(players[i].fd, string, strlen(string)), "write", -1, NULL);
    }
}

// Function to format question message
void make_question(char* question_buffer, struct Entry* questions, int question_num){
    sprintf(question_buffer, "Question: %d: %s\n1: %s\n2: %s\n3: %s\n", 
                    question_num+1, questions[question_num].prompt, 
                    questions[question_num].options[0], questions[question_num].options[1], questions[question_num].options[2]);
}

// Function to modify question message for a player
void mod_question_for_player(char* buffer, struct Entry* questions, int question_num){
    sprintf(buffer, "Question: %d: %s\nPress 1: %s\nPress 2: %s\nPress 3: %s\n", 
                    question_num+1, questions[question_num].prompt, 
                    questions[question_num].options[0], questions[question_num].options[1], questions[question_num].options[2]);
}

// Function to format answer message
void make_answer(char* buffer, struct Entry* questions, int question_num){
    sprintf(buffer, "The correct answer is: %s\n\n", questions[question_num].options[questions[question_num].answer_idx]);
}

// Function to read questions from file
int read_file(struct Entry* arr, char* filename){
    FILE* fstream;
    if( ( fstream = fopen(filename, "r") ) == NULL ){
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    char* delim_string = " \n";
    char* answer;

    char* line = NULL;
    size_t line_count;
    int num_entries = 0;
    while ( getline( &line, &line_count, fstream) != -1){
        if( line[0] == '\n'){ num_entries++; continue; }
        strcpy(arr[num_entries].prompt, line);
        if( getline( &line, &line_count, fstream) != -1 ){
            strcpy(arr[num_entries].options[0], strtok(line, delim_string));
            for(int i = 1; i < 3; i++){
                strcpy(arr[num_entries].options[i], strtok(NULL, delim_string));
            }
        }else{
            perror("reading from file");
            exit(EXIT_FAILURE);
        }

        if( getline( &line, &line_count, fstream) != -1 ){ answer = strtok(line, delim_string); }
        else{
            perror("reading from file");
            exit(EXIT_FAILURE);
        }
        for(int i = 0; i < 3; i++){
            if( strcmp(answer, arr[num_entries].options[i]) == 0 ){ arr[num_entries].answer_idx = i; break; }
        }
    }
    free(line);
    fclose(fstream);
    return num_entries;
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

// Function to prepare file descriptors for select
int prep_fds(fd_set* active_fds, int server_fd, struct player* players){
    FD_SET(server_fd, active_fds);
    int max_fd = server_fd;
    for(int i = 0; i < MAX_CONN; i++ ){
        if(players[i].fd!=-1){
            FD_SET(players[i].fd, active_fds);
            if (players[i].fd > max_fd) max_fd = players[i].fd;
        }
    }
    return max_fd;
}

// Main function
int main(int argc, char** argv){

    // Initialize default values
    char filename[] = DEFAULT_FILE;
    char ip_addr[] = DEFAULT_IP;
    long port_number = DEFAULT_PORT;

    // Parse command line arguments
    int option;
    int argc_count = 1;
    const char* options = ":fiph";
    while( (option = getopt(argc, argv, options)) != -1 ){
        switch(option){  
            case 'f':
                if(optind < argc){
                    strcpy(filename, argv[optind]);
                    argc_count+=2;
                }else exit_help("f flag", argv[0]);
                break;
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

    // Read questions from file
    struct Entry questions[50];
    int num_entries = read_file(questions, filename);

    // Initialize player structures
    struct player players[MAX_CONN];
    for(int i = 0; i < MAX_CONN; i++ ){
        players[i].fd = -1;
        players[i].score = 0;
    }

    // Initialize server and client variables
    int server_fd;
    int client_fd;
    struct sockaddr_in server_addr;
    struct sockaddr_in in_addr;
    socklen_t addr_size = sizeof(in_addr);
    fd_set active_fds;
    int nconn = 0;
    
    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    error_check(server_fd, "socket", -1, NULL);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number);
    server_addr.sin_addr.s_addr = inet_addr(ip_addr);
    
    // Bind socket
    error_check(bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)), "bind", server_fd, players);

    // Listen for connections
    error_check(listen(server_fd, MAX_CONN), "listen", server_fd, players);
    printf("Welcome to 392 Trivia!\n");

    // Variables for receiving messages
    int recvbytes;
    char buffer[buf];

    // Accept connections until max number of players reached
    while(nconn < MAX_CONN){

        // Prepare file descriptors for select
        int max_fd = prep_fds(&active_fds, server_fd, players);

        // Wait for activity on sockets
        error_check(select(max_fd + 1, &active_fds, NULL, NULL, NULL), "select", server_fd, players);
        
        // Check if new connection request received
        if (FD_ISSET(server_fd, &active_fds)) {

            // Accept new connection
            client_fd = accept(server_fd, (struct sockaddr*) &in_addr, &addr_size);
            error_check(client_fd, "accept", server_fd, players);

            // Add player to list
            players[nconn].fd = client_fd;
            printf("New connection detected!\n");

            // Read player name
            recvbytes = read(client_fd, buffer, buf);
            error_check(recvbytes, "read", server_fd, players);
            if (recvbytes > 0){
                
                buffer[recvbytes] = '\0';
                printf("Hi, %s!\n", buffer);
                strcpy(players[nconn].name, buffer);

                nconn++;

            }else if (recvbytes == 0){
                printf("Connection lost!\n");
                close_all(server_fd, players);
                exit(EXIT_FAILURE);
            }

            // Start game when max connections reached
            if(nconn == MAX_CONN){
                printf("Max Connection Reached!\n");
                printf("The game starts now!\n");
            }
        }
    }
        
    char q_buf[big_buf], a_buf[big_buf];
    int q_count = 0;
    int asked = 0;
    int game_over = 0;

    // Main game loop
    while(!game_over){
        
        if(!asked){

            // Prepare question message
            make_question(q_buf, questions, q_count);
            printf("%s", q_buf); fflush(stdout);
            mod_question_for_player(q_buf, questions, q_count);

            // Prepare answer message
            make_answer(a_buf, questions, q_count);
            printf("%s", a_buf); fflush(stdout);

            // Send question to all players
            send_all(q_buf, players);

            asked = 1;
        }

        // Prepare file descriptors for select
        int max_fd = prep_fds(&active_fds, server_fd, players);

        // Wait for activity on sockets
        error_check(select(max_fd + 1, &active_fds, NULL, NULL, NULL), "select", server_fd, players);

        // Check for new connection attempts
        if (FD_ISSET(server_fd, &active_fds)) {
            client_fd = accept(server_fd, (struct sockaddr*) &in_addr, &addr_size);
            error_check(client_fd, "accept", server_fd, players);
            printf("New Connection Attempted!\n");
            printf("Max Connection Reached!\n");
            close(client_fd);
        }

        // Check for player answers
        for(int i = 0; i < MAX_CONN; i++){
            if(players[i].fd != -1 && FD_ISSET(players[i].fd, &active_fds)){
                recvbytes = read(players[i].fd, buffer, buf);
                error_check(recvbytes, "read", server_fd, players);
                if (recvbytes > 0){
                
                    buffer[recvbytes] = '\0';
                    int answer = atoi(buffer);

                    // Check answer and update score
                    if( (answer - 1) == questions[q_count].answer_idx) {players[i].score++;}
                    else {players[i].score--;}

                    // Send correct answer to all players
                    send_all(a_buf, players);

                    // Check if game over
                    if(q_count == num_entries){
                        game_over = 1;
                    }
                    q_count++;
                    asked = 0;

                }else if (recvbytes == 0){
                    printf("Connection lost!\n");
                    close_all(server_fd, players);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    // Determine winner and send message to all players
    strcpy(buffer, "Congrats\0");
    int max_score = players[0].score;
    for (int i = 1; i < MAX_CONN; i++){
        if (players[i].score > max_score)
            max_score = players[i].score;
    }
    for (int i = 0; i < MAX_CONN; i++){
        if (players[i].score == max_score){
            strcat(buffer, ", ");
            strcat(buffer, players[i].name);
        }
    }

    strcat(buffer, "!\n\0");
    printf("%s", buffer);
    send_all(buffer, players);

    // Close all connections and exit
    close_all(server_fd, players);
    exit(EXIT_SUCCESS);
}
