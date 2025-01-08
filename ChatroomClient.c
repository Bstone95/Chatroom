#ifndef unix
#define WIN32
#include <windows.h>
#include <winsock.h>
#else
#define closesocket close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#endif

#include <stdio.h>
#include <string.h>

#define PROTOPORT 5193 /* default protocol port number */
extern int errno;
char localhost[] = "localhost"; /* default host name */
/*------------------------------------------------------------------------
 * * Program: client
 * *
 * * Purpose: allocate a socket, connect to a server, and print all output
 * * Syntax: client [ host [port] ]
 * *
 * * host - name of a computer on which server is executing
 * * port - protocol port number server is using
 * *
 * * Note: Both arguments are optional. If no host name is specified,
 * * the client uses "localhost"; if no protocol port is
 * * specified, the client uses the default given by PROTOPORT.
 * *
 * *------------------------------------------------------------------------
 * */

void chat_loop(int sd) {
    char buffer[1024];
    fd_set readfds;
    int n;

    printf("Connected to the chatroom. Type messages to send.\n");
    printf("Type 'EXIT' to leave the chatroom.\n");

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(0, &readfds);    // Standard input
        FD_SET(sd, &readfds);   // Server socket

        if (select(sd + 1, &readfds, NULL, NULL, NULL) < 0) {
            fprintf(stderr, "Error: select() failed.\n");
            break;
        }

        // Check if input is available from the user
        if (FD_ISSET(0, &readfds)) {
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline

            if (strcmp(buffer, "EXIT") == 0) {
                printf("Exiting chatroom...\n");
                break;
            }

            send(sd, buffer, strlen(buffer), 0);
        }

        // Check if input is available from the server
        if (FD_ISSET(sd, &readfds)) {
            n = recv(sd, buffer, sizeof(buffer) - 1, 0);
            if (n <= 0) {
                printf("Disconnected from the server.\n");
                break;
            }

            buffer[n] = '\0';
            printf("Server: %s\n", buffer);
        }
    }
}

int main(int argc, char *argv[]) {
    struct hostent *ptrh; /* pointer to a host table entry */
    struct protoent *ptrp; /* pointer to a protocol table entry */
    struct sockaddr_in sad; /* structure to hold an IP address */
    int sd; /* socket descriptor */
    int port; /* protocol port number */
    char *host; /* pointer to host name */
    int n; /* number of characters read */
    char buf[1000]; /* buffer for data from the server */
    char chatroom_name[256]; /* Chatroom name */

#ifdef WIN32
    WSADATA wsaData;
    WSAStartup(0x0101, &wsaData);
#endif

    memset((char *)&sad, 0, sizeof(sad)); /* clear sockaddr structure */
    sad.sin_family = AF_INET; /* set family to Internet */

    /* Check command-line argument for protocol port and extract */
    /* port number if one is specified. Otherwise, use the default */
    /* port value given by constant PROTOPORT */
    if (argc > 2) { /* if protocol port specified */
        port = atoi(argv[2]); /* if protocol port specified */
    } else {
        port = PROTOPORT; /* use default port number */
    }
    if (port > 0) { /* test for legal value */
        sad.sin_port = htons((u_short)port);
    } else { /* print error message and exit */
        fprintf(stderr, "Invalid port number.\n");
        exit(1);
    }

    /* Check host argument and assign host name */
    if (argc > 1) {
        host = argv[1]; /* if host argument specified */
    } else {
        host = localhost;
    }

    /* Convert host name to equivalent IP address and copy to sad */
    ptrh = gethostbyname(host);
    if (((char *)ptrh) == NULL) {
        fprintf(stderr, "Invalid host: %s\n", host);
        exit(1);
    }
    memcpy(&sad.sin_addr, ptrh->h_addr, ptrh->h_length);

    /* Map TCP transport protocol name to protocol number */
    ptrp = getprotobyname("tcp");
    if ( ((int)(ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "cannot map \"tcp\" to protocol number");
		exit(1);
	}

    /* Create a socket */
    sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
    if (sd < 0) {
        fprintf(stderr, "Socket creation failed.\n");
        exit(1);
    }

    /* Connect the socket to the specified server */
    if (connect(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
        fprintf(stderr, "Connection failed.\n");
        exit(1);
    }

    /* Request chatroom name from user */
    printf("Enter chatroom name to join: ");
    fgets(chatroom_name, sizeof(chatroom_name), stdin);
    chatroom_name[strcspn(chatroom_name, "\n")] = '\0'; // Remove newline

    /* Send chatroom name to the server */
    send(sd, chatroom_name, strlen(chatroom_name), 0);

    /* Enter chat loop */
    chat_loop(sd);

    /* Close the socket */
    closesocket(sd);
#ifdef WIN32
    WSACleanup();
#endif
    return 0;
}