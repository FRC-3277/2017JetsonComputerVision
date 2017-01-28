#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Message tags */
#define GET_XY		1
#define GET_QR		2

#define XY		101
#define QR		102

#define NET_FINISH	3
#define NET_ERROR	255

/* Common message struct */
struct message {
        int tag;
        int size;
        char * buff;
};

/* Server socket info */
struct sock_config {
        struct sockaddr_in addr;
        unsigned int addrsize;
        int sock;
};

/*
Function Prototypes
*/

/* Initialize TCP server socket */
struct sock_config * initServerSock(unsigned int);

/* Cleanup server socket */
void cleanServerSock(struct sock_config *);

/* Initialize TCP client socket */
int initClientSock(const char *, unsigned int);

/* Cleanup client socket */
void cleanClientSock( int );

/* Receive message over socket connection */
struct message * recvMsg( int );

/* Send message over socket connection */
int sendMsg( int, struct message * );

/* Free message structure and clear */
void free_msg( struct message * );

/* Create a new message */
struct message * newMsg(int);
