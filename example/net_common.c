#include "net_common.h"

struct sock_config * initServerSock( unsigned int port ) {

	int retval;
	struct sock_config *sc;

	/* Allocate memory for struct */
	sc = (struct sock_config *)malloc(sizeof(struct sock_config));
	if ( sc == NULL ) {
		fprintf(stderr, "Faile to allocate memory for sock_config\n");
		return NULL;
	}

	/* Initialize struct */
	memset(sc, '\0', sizeof(struct sock_config));

        /* Create Socket */
        sc->sock = socket(AF_INET, SOCK_STREAM, 0);
        if ( sc->sock == -1 ) {
		fprintf(stderr, "Failed to create socket: %s\n", strerror(errno));
		free(sc);
                return NULL;
        }

        /* Configure Socket parameters */
        sc->addr.sin_family = AF_INET;
        sc->addr.sin_port = htons(port);
        sc->addr.sin_addr.s_addr = htonl(INADDR_ANY);

        /* Bind to the socket */
        retval = bind(sc->sock, (struct sockaddr *) &(sc->addr), sizeof(struct sockaddr_in));
        if ( retval == -1 ) {
		fprintf(stderr, "Failed to bind to socket: %s\n", strerror(errno));
		free(sc);
                return NULL;
        }

        /* Listen for client connections */
        retval = listen(sc->sock, 1);
        if ( retval == -1 ) {
		fprintf(stderr, "Failed to listen on socket: %s\n", strerror(errno));
		free(sc);
                return NULL;;
        }

	/* Set size */
	sc->addrsize = sizeof(sc->addr);

        return sc;
}

void cleanServerSock( struct sock_config * sc ) {

	if ( sc == NULL ) {
		return;
	}

	/* Close socket */
	close(sc->sock);

	free(sc);
}

int initClientSock( const char *ip, unsigned int port ) {

	int sock;
	struct sockaddr_in addr;

	/* Fill values into our structure used to do make a connection */
	addr.sin_family = AF_INET;

	/* Set port number */
	addr.sin_port = htons(port);

	/* Server IP Address - convert to Network Byte Order */
	addr.sin_addr.s_addr = inet_addr(ip);

        /* Create a socket */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		fprintf(stderr, "Failed to create socket: %s\n", strerror(errno));
                return -1;
        }
	
	/* Make a connection to the server */
	if (connect(sock, (struct sockaddr *) &addr, sizeof (struct sockaddr_in)) == -1) {
		fprintf(stderr, "Failed to connect to server socket: %s\n", strerror(errno));
		return -1;
        }

	return sock;
}

void cleanClientSock( int sock ) {
	if ( sock >= 0 ) {
		close(sock);
	}
}

struct message * recvMsg( int sock ) {

	/* Allocate message structure */
	struct message *msg = newMsg(0);
	if ( msg == NULL ) {
		fprintf(stderr, "Failed to create new message object\n");
		return msg;
	}

	/* Header buffer for recv */
        char header[sizeof(msg->tag) + sizeof(msg->size)];

        /* Retreive the message header - 8 bytes */
        int mlen = recv(sock, header, sizeof(header), 0);
        if ( mlen != sizeof(header) ) {
		fprintf(stderr, "Failed to recv on socket: %s\n", strerror(errno));
		free_msg(msg);
                return(NULL);
        }

        /* Get header values */
        memcpy(&msg->tag,  &header[0], sizeof(msg->tag));
        memcpy(&msg->size, &header[4], sizeof(msg->size));

	/* Convert to Host Byte Order */
        msg->tag = ntohl(msg->tag);
        msg->size = ntohl(msg->size);

        /* Return early if buffer size is 0 */
        if ( msg->size == 0 ) {
                msg->buff = NULL;
                return(msg);
        }

        /* Allocate memory for receiving the buffer */
        msg->buff = (char *)malloc(msg->size + 1);
        if ( msg->buff == NULL ) {
		fprintf(stderr, "Failed to allocate memory for msg buff\n");
                free_msg(msg);
                return(NULL);
        }

        /* Memset allocation to '\0' */
        memset(msg->buff, '\0', msg->size + 1);

        /* Retreive the rest of the message */
        mlen = recv(sock, msg->buff, msg->size, 0);
        if ( mlen != msg->size ) {
		fprintf(stderr, "Failed to recv msg buff on socket: %s\n", strerror(errno));
                free_msg(msg);
                return(NULL);
        }

        return msg;
}

int sendMsg(int sock, struct message * msg) {

	/* Sanity check pointer */
        if ( msg == NULL ) {
                return -1;
        }

	/* Calculate send_buff size */
	int buff_size = sizeof(msg->tag) + sizeof(msg->size) + msg->size;

	/* Allocate memory for send buffer */
	char * send_buff = (char *)malloc(buff_size);
	if ( send_buff == NULL ) {
		fprintf(stderr, "Failed to allocate memory for msg\n");
		return -1;
	}
	/* Convert to Network Byte Order */
	int nbo_tag = htonl(msg->tag);
	int nbo_size = htonl(msg->size);

	/* Copy data into send buffer */
	memcpy(&send_buff[0], &nbo_tag,  sizeof(nbo_tag));
	memcpy(&send_buff[sizeof(nbo_tag)], &nbo_size,  sizeof(nbo_size));
	memcpy(&send_buff[sizeof(nbo_tag) + sizeof(nbo_size)], msg->buff, msg->size);

	/* Send the message */
        int mlen = send(sock, send_buff, buff_size, 0);
        if ( mlen != buff_size ) {
		fprintf(stderr, "Failed to send msg on socket: %s\n", strerror(errno));
		free(send_buff);
                return -1;
	}

	/* Free buffer and return */
	free(send_buff);
        return 0;
}

void free_msg( struct message *msg ) {

        /* Sanity check passed-in pointer */
        if ( msg == NULL ) {
                return;
        }

        /* Free buffer is not null */
        if ( msg->buff != NULL ) {
                free(msg->buff);
		msg->buff = NULL;
        }

        /* Free message */
        free(msg);
	msg = NULL;
}

struct message * newMsg( int tag ) {

	struct message *msg;

	/* Allocate memory for new message */
	msg = (struct message *)malloc(sizeof(struct message));
	if ( msg == NULL ) {
		fprintf(stderr, "Failed to allocate memory for new msg\n");
		return msg;
	}

	/* Initialize struct */
	msg->tag = tag;
	msg->size = 0;
	msg->buff = NULL;

	return msg;
}
