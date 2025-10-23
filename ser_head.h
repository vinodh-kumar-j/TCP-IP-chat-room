#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_CLIENTS 10
#define MAX_USERNAME 32
#define MAX_PASSWORD 32
#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5000

typedef enum
{
    e_success,
    e_failure
} Status;

typedef struct {
    int sock;
    char username[MAX_USERNAME];
    struct sockaddr_in addr;  // store IP/port for join/leave messages
} Client;

extern Client clients[MAX_CLIENTS];                         // current session array to keep the count of online users 
extern volatile bool server_running;                        // flag for forced stop of server
extern int server_sock; 

// Function declarations
void broadcast_message(const char *msg, int sender_sock);   // broadcast the msg to all online users 
bool is_already_logged_in(const char *username);            // to check and avoid multiple log in at same time
void set_client_username(int sock, const char *username);   // server set a name to the client and stores in an current session array 
void remove_client(int sock);                               // remove client from online user or current session array
void remove_user_from_file(const char *username);           // delete the account from server database
void send_online_users(int sock);                           // broadcast number of users online
//void handle_chat_menu(int sock, const char *username);
void handle_single_chat(int sock, const char *sender);      // handler/helper function for single chat 
void handle_group_chat(int sock, const char *sender);       // handler/helper function for group chat
void *client_handler(void *arg);                            // handler for chat menu and navigation
Status server_init(void);                                   // three way hand shake and initializiton
Status run_server(void);                                    // adds client to server multithreading 

#endif
