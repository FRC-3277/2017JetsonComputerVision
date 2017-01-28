#include "net_common.h"
#include "video_shm.h"

struct options {
	int getXY;
	int getQR;
	char * host;
	unsigned long int port;
};

void clean_opts( struct options * opts ) {
        if ( opts == NULL ) { return; }
        if ( opts->host != NULL ) { free(opts->host); }
        free(opts);
}

void sendCMD( int sock, int cmd ) {

        /* Create new message */
        struct message * msg = newMsg(cmd);
        if ( msg == NULL ) {
                fprintf(stdout, "Failed to get initial msg\n");
                return;
        }

        /* Send the message */
        int retval = sendMsg(sock, msg);
        if ( retval == -1 ) {
                fprintf(stderr, "Error sending message\n");
                free_msg(msg);
                return;
        }

	/* Return early as there is no response */
	if ( cmd == NET_FINISH ) {
		free(msg);
		return;
	}

        /* Receive message */
        struct message * reply = recvMsg(sock);
        if ( reply == NULL ) {
                fprintf(stderr, "Error receiving message\n");
                free_msg(msg);
                return;
        }

        /* Print reply message */
        fprintf(stdout, "Tag: %d\n", reply->tag);
        fprintf(stdout, "Size: %d\n", reply->size);
        fprintf(stdout, "Message: %s\n", reply->buff);

        /* Free messages */
        free_msg(msg);
        free_msg(reply);
}

struct options * init_options( int argc, char *argv[] ) {

	struct options *opts;
	int gopt;

        /* Allocate memory for structure */
        opts = (struct options *)malloc(sizeof(struct options));
        if ( opts == NULL ) {
                fprintf(stderr, "failed to allocate memory for options struct\n");
                return NULL;
        }

        /* Initialize struct */
        memset(opts, '\0', sizeof(struct options));

        /* Get passed arguments */
        while ( (gopt = getopt(argc, argv, "qxh:p:")) != EOF ) {

                switch (gopt) {
                        case 'q':
                                opts->getXY = 1;
                                break;
                        case 'x':
                                opts->getQR = 1;
                                break;
                        case 'h':
				opts->host = (char *)malloc(strlen(optarg) + 1);
				strncpy(opts->host, optarg, strlen(optarg) + 1);
                                break;
                        case 'p':
				opts->port = strtoul(optarg, 0, 10);
                                break;
                        default:
                                fprintf(stdout, "Unknown argument(s)\n");
                                break;
                }
        }

        /* Sanity check input */
        if ( !(opts->getXY | opts->getQR) ) {
                fprintf(stderr, "Please choose a cmd option(s) to perform\n");
		clean_opts(opts);
                return NULL;
        }

	if ( opts->port <= 0 ||  opts->port > 65536 ) {
		fprintf(stderr, "Please specify a valid port\n");
		clean_opts(opts);
		return NULL;
	}

	if ( opts->host == NULL ) {
		fprintf(stderr, "Please specify a host\n");
		clean_opts(opts);
		return NULL;
	}

	return opts;
}

int main(int argc, char *argv[]) {

	struct options *opts;

	/* Get/Parse options */
	opts = init_options(argc, argv);
	if ( opts == NULL ) {
		fprintf(stderr, "Failed to parse cmd line options\n");
		return -1;
	}

        /* Connect to server */
        int sock = initClientSock(opts->host, opts->port);
        if ( sock  == -1 ) {
                fprintf(stderr, "Failed initializing socket\n");
		clean_opts(opts);
                return -1;
        }

	/* Send the desired message(s) */
	if ( opts->getXY ) { sendCMD(sock, GET_XY); }
	if ( opts->getQR ) { sendCMD(sock, GET_QR); }

	/* Send server finish message */
	sendCMD(sock, NET_FINISH);

	/* cleanup and exit */
        cleanClientSock(sock);
	clean_opts(opts);

        return 0;
}
