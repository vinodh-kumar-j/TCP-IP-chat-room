#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5000
#define BUFFER_SIZE 512

void *recv_func(void *sock_ptr)
{
    int sock = *(int *)sock_ptr;
    char buffer[BUFFER_SIZE];
    int n;

    while ((n = recv(sock, buffer, sizeof(buffer), 0)) > 0)
    {
        buffer[n] = '\0';
        printf("\r%s\n", buffer);
        printf("[You] ");
        fflush(stdout);
    }

    printf("Disconnected from server.\n");
    pthread_exit(NULL);
}

int main()
{
    int sock;
    struct sockaddr_in serv_addr;
    pthread_t recv_thread;
    char msg[BUFFER_SIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0)
    {
        perror("connect");
        close(sock);
        return 1;
    }

    printf("Connected to chat room!\n");
    pthread_create(&recv_thread, NULL, recv_func, (void *)&sock);
    pthread_detach(recv_thread);

    while (1)
    {
        printf("[You] ");
        fflush(stdout);
        fgets(msg, sizeof(msg), stdin);
        msg[strcspn(msg, "\n")] = '\0'; // remove newline

        if (strcmp(msg, "exit") == 0)
        {
            send(sock, msg, strlen(msg) + 1, 0);
            break;
        }

        send(sock, msg, strlen(msg) + 1, 0);
    }

    close(sock);
    return 0;
}
