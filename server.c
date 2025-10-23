#include "ser_head.h"
#include "auth.h"

Client clients[MAX_CLIENTS] = {0};
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile bool server_running = true;
int server_sock;
/* ---------------------- utility functions------------------------- */
// Broadcast a message to all clients except the sender
void broadcast_message(const char *message, int sender_sock)
{
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].sock != 0 && clients[i].sock != sender_sock)
        {
            send(clients[i].sock, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Send the list of online users to a specific client
void send_online_users(int sock)
{
    char msg[BUFFER_SIZE] = "\n--- Online Users ---\n";
    int count = 0;

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].sock != 0 && strlen(clients[i].username) > 0)
        {
            count++;
            char line[128];
            snprintf(line, sizeof(line), "%d. %s\n", count, clients[i].username);
            strcat(msg, line);
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    char summary[64];
    snprintf(summary, sizeof(summary), "Total Online: %d\n", count);
    strcat(msg, summary);

    send(sock, msg, strlen(msg), 0);
}

// Check if a username is already logged in (multi-login prevention)
bool is_already_logged_in(const char *username)
{
    bool logged_in = false;
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].sock != 0 && strcmp(clients[i].username, username) == 0)
        {
            logged_in = true;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    return logged_in;
}

// Set the username for a given client socket
void set_client_username(int sock, const char *username)
{
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].sock == sock)
        {
            strncpy(clients[i].username, username, MAX_USERNAME - 1);
            clients[i].username[MAX_USERNAME - 1] = '\0';
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Remove a client and broadcast leave message
void remove_client(int sock)
{
    char username[MAX_USERNAME] = {0};
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    getpeername(sock, (struct sockaddr *)&addr, &addr_len);

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].sock == sock)
        {
            strncpy(username, clients[i].username, MAX_USERNAME);
            clients[i].sock = 0;
            clients[i].username[0] = '\0';
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    if (strlen(username) > 0)
    {
        char msg[BUFFER_SIZE];
        snprintf(msg, sizeof(msg), "-> %s left from %s:%d\n", username,
                 inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        broadcast_message(msg, -1);
        printf("%s", msg); // server log
    }
}
void remove_user_from_file(const char *username)// delete account from server database
{
    FILE *fp = fopen("data/users.txt", "r");
    FILE *temp = fopen("data/temp.txt", "w");
    char line[BUFFER_SIZE];

    if (!fp || !temp)
        return;

    while (fgets(line, sizeof(line), fp))
    {
        char line_copy[BUFFER_SIZE];
        strcpy(line_copy, line); // strtok modifies the string
        char *stored_user = strtok(line_copy, ":");

        if (stored_user && strcmp(username, stored_user) != 0)
        {
            fputs(line, temp); // copy other users
        }
    }

    fclose(fp);
    fclose(temp);

    remove("data/users.txt");
    rename("data/temp.txt", "data/users.txt");
}


/* ------------------------- intialize server and run ------------------------------ */
Status server_init()
{
    struct sockaddr_in server_addr;

    // Create TCP socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
        perror("socket");
        return e_failure;
    }

    // Setup server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Bind socket
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        return e_failure;
    }

    // Listen for connections
    if (listen(server_sock, MAX_CLIENTS) < 0)
    {
        perror("listen");
        return e_failure;
    }

    printf("Server started at %s:%d\n", SERVER_IP, SERVER_PORT);
    return e_success;
}

Status run_server()
{
    struct sockaddr_in client_addr;
    socklen_t addr_size = sizeof(client_addr);
    pthread_t tid;

    while (server_running)
    {
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size);
        if (client_sock < 0)
        {
            if (!server_running)
                break; // server is stopping
            perror("accept");
            continue;
        }

        // Log client join
        char join_msg[BUFFER_SIZE];
        snprintf(join_msg, sizeof(join_msg), "Server: Client joined from %s:%d\n",
                 inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        printf("%s", join_msg);
        broadcast_message(join_msg, -1); // server broadcast

        // Add client to clients array
        pthread_mutex_lock(&clients_mutex);
        int i;
        for (i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i].sock == 0)
            {
                clients[i].sock = client_sock;
                strcpy(clients[i].username, ""); // empty at first
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        if (i == MAX_CLIENTS)
        {
            printf("Max clients reached. Connection rejected.\n");
            send(client_sock, "Server full. Try again later.\n", 30, 0);
            close(client_sock);
        }
        else
        {
            // Create client handler thread
            pthread_create(&tid, NULL, client_handler, (void *)&clients[i]);
            pthread_detach(tid);
        }
    }

    // --- Server stopping: notify all clients ---
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].sock != 0)
        {
            send(clients[i].sock, "Server is shutting down.\n", 25, 0);
            close(clients[i].sock);
            clients[i].sock = 0;
            clients[i].username[0] = '\0';
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    // Close server socket
    close(server_sock);
    printf("Server has been stopped.\n");
    return e_success;
}

/* -------------------------------------- chat handler ------------------------------ */
void *client_handler(void *arg)
{
    Client *client = (Client *)arg;
    int sock = client->sock;
    char buffer[BUFFER_SIZE];
    int n;

    char username[MAX_USERNAME] = {0};

    // --- Authentication Loop ---
    while (1)
    {
        char *menu = "1. Login\n2. Register\n3. Exit\nChoice: ";
        send(sock, menu, strlen(menu), 0);

        n = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0)
            break;

        buffer[n] = '\0';
        buffer[strcspn(buffer, "\r\n")] = 0;

        if (strcmp(buffer, "1") == 0) // Login
        {
            send(sock, "Username: ", 10, 0);
            n = recv(sock, username, sizeof(username) - 1, 0);
            if (n <= 0)
                continue;
            username[n] = '\0';
            username[strcspn(username, "\r\n")] = 0;

            char password[MAX_PASSWORD];
            send(sock, "Password: ", 10, 0);
            n = recv(sock, password, sizeof(password) - 1, 0);
            if (n <= 0)
                continue;
            password[n] = '\0';
            password[strcspn(password, "\r\n")] = 0;

            if (is_already_logged_in(username))
            {
                send(sock, "User already logged in.\n", 25, 0);
                continue;
            }

            if (login_user(username, password))
            {
                set_client_username(sock, username);

                char msg[BUFFER_SIZE];
                snprintf(msg, sizeof(msg), "Login successful. Welcome %s!\n", username);
                send(sock, msg, strlen(msg), 0);

                // Notify all users about join with IP/port
                struct sockaddr_in addr;
                socklen_t addr_len = sizeof(addr);
                getpeername(sock, (struct sockaddr *)&addr, &addr_len);

                char join_msg[BUFFER_SIZE];
                snprintf(join_msg, sizeof(join_msg), "-> %s joined from %s:%d\n", username,
                         inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
                broadcast_message(join_msg, -1); // send to all clients
                printf("%s", join_msg);          // server console log

                // Show online users
                send_online_users(sock);

                // --- Chat Menu Loop ---
                while (1)
                {
                    char *chat_menu = "\n1. Single Chat\n2. Group Chat\n3. Online Users\n4. Logout to Main Menu\n5. Delete Account & Exit\nChoice: ";
                    send(sock, chat_menu, strlen(chat_menu), 0);

                    n = recv(sock, buffer, sizeof(buffer) - 1, 0);
                    if (n <= 0)
                        break;
                    buffer[n] = '\0';
                    buffer[strcspn(buffer, "\r\n")] = 0;

                    if (strcmp(buffer, "1") == 0) // Single Chat
                    {
                        handle_single_chat(sock, username);
                    }
                    else if (strcmp(buffer, "2") == 0) // Group Chat
                    {
                        handle_group_chat(sock, username);
                    }
                    else if (strcmp(buffer, "3") == 0) // Show online users
                    {
                        send_online_users(sock);
                    }
                    else if (strcmp(buffer, "4") == 0) // Logout to main menu
                    {
                        set_client_username(sock, "");
                        break;
                    }
                    else if (strcmp(buffer, "5") == 0)
                    {                                    // Delete Account & Exit
                        remove_user_from_file(username); 
                        set_client_username(sock, ""); 
                        remove_client(sock);             
                        send(sock, "Your account has been deleted. Goodbye!\n", 44, 0);
                        close(sock);
                        pthread_exit(NULL); // terminate thread
                    }
                    else
                    {
                        send(sock, "Invalid choice.\n", 16, 0);
                    }
                }
            }
            else
            {
                send(sock, "Invalid username or password.\n", 31, 0);
            }
        }
        else if (strcmp(buffer, "2") == 0) // Register
        {
            send(sock, "Choose username: ", 17, 0);
            n = recv(sock, username, sizeof(username) - 1, 0);
            if (n <= 0)
                continue;
            username[n] = '\0';
            username[strcspn(username, "\r\n")] = 0;

            char password[MAX_PASSWORD];
            send(sock, "Choose password: ", 17, 0);
            n = recv(sock, password, sizeof(password) - 1, 0);
            if (n <= 0)
                continue;
            password[n] = '\0';
            password[strcspn(password, "\r\n")] = 0;

            if (register_user(username, password))
                send(sock, "Registration successful. Please login.\n", 40, 0);
            else
                send(sock, "Username already exists.\n", 25, 0);
        }
        else if (strcmp(buffer, "3") == 0) // Exit
        {
            remove_client(sock);
            close(sock);
            pthread_exit(NULL);
        }
        else
        {
            send(sock, "Invalid choice.\n", 16, 0);
        }
    }

    remove_client(sock);
    close(sock);
    pthread_exit(NULL);
}
/* ----------------------------- single and group chat helper===================== */
void handle_single_chat(int sock, const char *sender)
{
    char target[MAX_USERNAME];
    char message[BUFFER_SIZE];
    int n;

    send(sock, "Enter recipient username: ", 26, 0);
    n = recv(sock, target, sizeof(target) - 1, 0);
    if (n <= 0)
        return;
    target[n] = '\0';
    target[strcspn(target, "\r\n")] = 0;

    send(sock, "Message (type '<-' to return to menu): ", 40, 0);

    while ((n = recv(sock, message, sizeof(message) - 1, 0)) > 0)
    {
        message[n] = '\0';
        message[strcspn(message, "\r\n")] = 0;

        if (strcmp(message, "<-") == 0)
            break;

        pthread_mutex_lock(&clients_mutex);
        int found = 0;
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i].sock != 0 && strcmp(clients[i].username, target) == 0)
            {
                char full_msg[BUFFER_SIZE];
                int max_len = sizeof(full_msg) - strlen(sender) - 7; // "[Private] " + ": \n\0"
                if (max_len < 0)
                    max_len = 0;

                snprintf(full_msg, sizeof(full_msg), "[Private] %s: %.*s\n", sender, max_len, message);

                // Send the message to the target client
                if (send(clients[i].sock, full_msg, strlen(full_msg), 0) < 0)
                    perror("send");

                found = 1;
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        if (!found)
            send(sock, "User not found or offline.\n", 27, 0);
    }
}
void handle_group_chat(int sock, const char *sender)
{
    char message[BUFFER_SIZE];
    int n;

    send(sock, "Enter group message (type '<-' to return to menu): ", 50, 0);

    while ((n = recv(sock, message, sizeof(message) - 1, 0)) > 0)
    {
        message[n] = '\0';
        message[strcspn(message, "\r\n")] = 0;

        if (strcmp(message, "<-") == 0)
            break;

        char full_msg[BUFFER_SIZE];
        int max_len = sizeof(full_msg) - strlen(sender) - 4; // "[Group] " + ": \n\0" = 4
        if (max_len < 0)
            max_len = 0;

        snprintf(full_msg, sizeof(full_msg), "[Group] %s: %.*s\n", sender, max_len, message);

        // Broadcast message to all clients except sender
        broadcast_message(full_msg, sock);
    }
}
