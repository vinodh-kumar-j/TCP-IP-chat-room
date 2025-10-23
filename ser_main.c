/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 *   Author         : VINODH KUMAR J
 *   Date           : 23 OCT 2025
 *   Title          : TCP/IP Chat Room
 *   Description    : Server–Client socket programming,chat room 
 *                    with minimal functionality using TCP/IP based sockets in a Linux based LAN environment
 *--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include "ser_head.h"

int main()
{
    if (server_init() != e_success) return -1;

    pthread_t server_thread;
    pthread_create(&server_thread, NULL, (void*(*)(void*))run_server, NULL);

    printf("Type 'stop' to shutdown server:\n");
    char cmd[32];
    while (1)
    {
        fgets(cmd, sizeof(cmd), stdin);
        if (strncmp(cmd, "stop", 4) == 0)
        {
            server_running = false;
            shutdown(server_sock, SHUT_RDWR);  // unblocks accept()
            break;
        }
    }

    pthread_join(server_thread, NULL);
    printf("Server exited.\n");
    return 0;
}
