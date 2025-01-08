#ifndef unix
#define WIN32
#include <windows.h>
#include <winsock.h>
#else
#define closesocket close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#endif

#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define PROTOPORT 5193
#define QLEN 6
#define MAX_CLIENTS 100
#define MAX_CHATROOMS 50

typedef struct {
    char chatroom_name[256];
    int clients[MAX_CLIENTS];
    int client_count;
    pthread_mutex_t lock;
} ChatRoom;

ChatRoom chatrooms[MAX_CHATROOMS];
int chatroom_count = 0;
pthread_mutex_t chatroom_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Function to find or create a chatroom */
ChatRoom *find_chatroom(const char *chatroom_name) {
    pthread_mutex_lock(&chatroom_mutex);

    // Search for existing chatroom
    for (int i = 0; i < chatroom_count; i++) {
        if (strcmp(chatrooms[i].chatroom_name, chatroom_name) == 0) {
            pthread_mutex_unlock(&chatroom_mutex);
            return &chatrooms[i];
        }
    }

    // Create a new chatroom
    if (chatroom_count >= MAX_CHATROOMS) {
        printf("Error: Maximum chatrooms reached.\n");
        pthread_mutex_unlock(&chatroom_mutex);
        return NULL;
    }

    ChatRoom *new_chatroom = &chatrooms[chatroom_count++];
    strcpy(new_chatroom->chatroom_name, chatroom_name);
    new_chatroom->client_count = 0;
    pthread_mutex_init(&new_chatroom->lock, NULL);

    pthread_mutex_unlock(&chatroom_mutex);
    return new_chatroom;
}

/* Function to handle messages from a client */
void process_client(int client_socket, ChatRoom *chatroom) {
    char buffer[1024];
    int n;

    while ((n = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[n] = '\0';
        printf("Message from client in chatroom %s: %s\n", chatroom->chatroom_name, buffer);

        // Broadcast message to all clients in the chatroom
        pthread_mutex_lock(&chatroom->lock);
        for (int i = 0; i < chatroom->client_count; i++) {
            if (chatroom->clients[i] != client_socket) {
                send(chatroom->clients[i], buffer, n, 0);
            }
        }
        pthread_mutex_unlock(&chatroom->lock);
    }

    // Remove client from chatroom on disconnection
    pthread_mutex_lock(&chatroom->lock);
    for (int i = 0; i < chatroom->client_count; i++) {
        if (chatroom->clients[i] == client_socket) {
            chatroom->clients[i] = chatroom->clients[--chatroom->client_count];
            break;
        }
    }
    pthread_mutex_unlock(&chatroom->lock);

    closesocket(client_socket);
    printf("Client disconnected from chatroom %s\n", chatroom->chatroom_name);
}

/* Worker thread for handling a client */
void *worker(void *arg) {
    int client_socket = *(int *)arg;
    char chatroom_name[256];
    int n = recv(client_socket, chatroom_name, sizeof(chatroom_name) - 1, 0);

    if (n <= 0) {
        closesocket(client_socket);
        pthread_exit(0);
    }

    chatroom_name[n] = '\0';
    printf("Client joining chatroom: %s\n", chatroom_name);

    ChatRoom *chatroom = find_chatroom(chatroom_name);
    if (chatroom == NULL) {
        send(client_socket, "Error: Cannot create or join chatroom.\n", 40, 0);
        closesocket(client_socket);
        pthread_exit(0);
    }

    // Add client to the chatroom
    pthread_mutex_lock(&chatroom->lock);
    if (chatroom->client_count < MAX_CLIENTS) {
        chatroom->clients[chatroom->client_count++] = client_socket;
    } else {
        send(client_socket, "Error: Chatroom is full.\n", 25, 0);
        pthread_mutex_unlock(&chatroom->lock);
        closesocket(client_socket);
        pthread_exit(0);
    }
    pthread_mutex_unlock(&chatroom->lock);

    process_client(client_socket, chatroom);
    pthread_exit(0);
}
/*------------------------------------------------------------------------
 * * Program: server
 * *
 * * Purpose: allocate a socket and then repeatedly execute the following:
 * * (1) wait for the next connection from a client
 * * (2) send a short message to the client
 * * (3) close the connection
 * * (4) go back to step (1)
 * * Syntax: server [ port ]
 * *
 * * port - protocol port number to use
 * *
 * * Note: The port argument is optional. If no port is specified,
 * * the server uses the default given by PROTOPORT.
 * *
 * *------------------------------------------------------------------------
 * */

main(argc, argv)
	int argc;
	char *argv[];
{
	struct hostent *ptrh; /* pointer to a host table entry */
	struct protoent *ptrp; /* pointer to a protocol table entry */
	struct sockaddr_in sad; /* structure to hold server.s address */
	struct sockaddr_in cad; /* structure to hold client.s address */
	int sd, sd2; /* socket descriptors */
	int port; /* protocol port number */
	int alen; /* length of address */
    	pthread_t threads[MAX_CLIENTS];
    	int thread_count = 0;
	char buf[1000]; /* buffer for string the server sends */


#ifdef WIN32
    WSADATA wsaData;
    WSAStartup(0x0101, &wsaData);
#endif

    memset((char *)&sad,0,sizeof(sad)); /* clear sockaddr structure */
    sad.sin_family = AF_INET; /* set family to Internet */
    sad.sin_addr.s_addr = INADDR_ANY; /* set the local IP address */
	/* Check command-line argument for protocol port and extract */
	/* port number if one is specified. Otherwise, use the default */
	/* port value given by constant PROTOPORT */

   if (argc > 1) { /* if argument specified */
		port = atoi(argv[1]); /* convert argument to binary */
	} else {
		port = PROTOPORT; /* use default port number */
	}
	if (port > 0) /* test for illegal value */
		sad.sin_port = htons((u_short)port);
	else { /* print error message and exit */
		fprintf(stderr,"bad port number %s\n",argv[1]);
		exit(1);
	}

    ptrp = getprotobyname("tcp");
    if (!ptrp) {
        fprintf(stderr, "Cannot map \"tcp\" to protocol number.\n");
        exit(1);
    }
	/* Create a socket */
    sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
    if (sd < 0) {
        fprintf(stderr, "Socket creation failed.\n");
        exit(1);
    }
	/* Bind a local address to the socket */
    if (bind(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
        fprintf(stderr, "Bind failed.\n");
        closesocket(sd);
        exit(1);
    }
	/* Specify size of request queue */
    if (listen(sd, QLEN) < 0) {
        fprintf(stderr, "Listen failed.\n");
        closesocket(sd);
        exit(1);
    }

    printf("Chat server running on port %d...\n", port);

	/* Main server loop - accept and handle requests */
    while (1) {
        alen = sizeof(cad);
        client_s = accept(sd, (struct sockaddr *)&cad, &alen);
        if (client_s < 0) {
            fprintf(stderr, "Accept failed.\n");
            continue;
        }

        pthread_create(&threads[thread_count++], NULL, worker, &client_s);
    }

    closesocket(sd);
#ifdef WIN32
    WSACleanup();
#endif
    return 0;
}