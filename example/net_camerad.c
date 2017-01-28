#include "net_camerad.h"

int main(int argc, char *argv[]) {

	struct net_config * nc;

        /* Register signal handlers */
	signal(SIGCHLD, SIG_IGN);
        signal(SIGHUP,  sig_handle);
        signal(SIGINT,  sig_handle);
        signal(SIGKILL, sig_handle);

	/* Fire-up logging */
	elk = initELK(ELK_ADDR, ELK_PORT, ELK_TYPE, "CameraService", true);
	if ( elk == NULL ) {
		fprintf(stderr, "Failed to initialize logging\n");
		return 1;
	}

	/* Allocate memory for struct */
	nc = (struct net_config *)malloc(sizeof(struct net_config));
	if ( nc == NULL ) {
		logELK(elk, FATAL, "Error allocating memory for net_config struct");
		return 1;
	}

	/* Initialize struct */
	memset(nc, '\0', sizeof(struct net_config));

	/* Get passed-in opts */
	nc->opts = init_options(argc, argv); 
	if ( nc->opts == NULL ) {
		logELK(elk, FATAL, "Failed to parsing/setting getopts");
		cleanup(nc);
		return 1;
	}

        /* Setup shared memory */
        nc->si = (struct shm_info *)attach_shm(QR_SEM_NAME, QR_SHM_NAME);
        if ( nc->si == NULL ) {
		logELK(elk, FATAL, "Shared memory initialization failed");
		cleanup(nc);
                return 1;
        }

	/* Initialize server socket */
	nc->sc = initServerSock(nc->opts->port);
	if ( nc->sc == NULL ) {
		logELK(elk, FATAL, "Failed to initialize server socket");
		cleanup(nc);
		return 1;
	}

	/* Handy short-cut pointers */
	struct sock_config *sc = nc->sc;
	struct shm_info *si = nc->si;

	while ( 1 ) {

		/* Wait for client connection */
		int clientsock = accept( sc->sock, (struct sockaddr *) &(sc->addr), &(sc->addrsize) );
		if ( clientsock == -1 ) {
			logELK(elk, ERROR, "Failed to accept client connection");
			continue;
		}

		/* Fork child worker */
		int pid = fork();
		if ( pid == -1 ) {
			logELK(elk, ERROR, "Error occured while forking worker");
			close(clientsock);
			continue;
		}

		/* Check if we are the child */
		if ( pid == 0 ) {
			logELK(elk, INFO, "Processing client connection from %s", inet_ntoa(sc->addr.sin_addr));
			process(clientsock, si);
			exit(0);
		}

		/* Close parent copy of clientsock */
		close(clientsock);
	}

	cleanup(nc);
	return(0);
}

void process(int clientsock, struct shm_info * qr_si) {

	int listen = 1;
	struct message *reply, *msg;

	while ( listen ) {

		/* Process client connection */
		msg = recvMsg(clientsock);
		if ( msg == NULL ) {
			logELK(elk, ERROR, "Error receiving message, closing connection");
			break;
		}

		/* Parse msg header */
		switch ( msg->tag ) {
			case GET_XY:
				logELK(elk, INFO, "Processing GET_XY\n");
				reply = getXY(qr_si);
				if ( reply == NULL ) {
					reply = newMsg(NET_FINISH);
					listen = 0;
				}
				break;
			case GET_QR:
				logELK(elk, INFO, "Processing GET_QRCODE");
				reply = getQR(qr_si);
				if ( reply == NULL ) {
					reply = newMsg(NET_FINISH);
					listen = 0;
				}
				break;
			case NET_FINISH:
				logELK(elk, INFO, "Processing NET_FINISH");
				reply = NULL;
				listen = 0;
				break;
			default:
				logELK(elk, ERROR, "Unknown header value");
				reply = newMsg(NET_FINISH);
				listen = 0;
				break;
		}

		/* Free client message */
		free_msg(msg);

		/* Loop early if reply is NULL */
		if ( reply == NULL ) {
			continue;
		}

		/* Send reply to client */
		if ( (sendMsg(clientsock, reply)) == -1 ) {
			logELK(elk, ERROR, "Error sending reply to client");
			listen = 0;
		}

		/* Free reply message */
		free_msg(reply);
	}

	/* Finish connection */
	shutdown(clientsock, SHUT_RDWR);
	close(clientsock);
	return;
}

struct message * getQR(struct shm_info * qr_si) {

	struct qr_info *qi = (struct qr_info *)qr_si->shm_ptr;

	/* Create new message */
	struct message *msg = newMsg(QR);
	if ( msg == NULL ) {
		return msg;
	}

	/* Signal camerad process and wait */
	if ( (killWait(SIGUSR1, SIG_TIMEOUT, qi->pid)) != 0 ) {
		free_msg(msg);
		return msg;
	} 

	/* Populate JSON objects */
	json_object *lock = json_object_new_int(qi->lock);
	json_object *jarray = json_object_new_array();
	for ( unsigned int i = 0; i < sizeof(qi->qr); i++ ) {
		json_object_array_add(jarray, json_object_new_int(qi->qr[i]));
	}

	/* JSON Key/Value pair object */
	json_object * jobj = json_object_new_object();
	json_object_object_add(jobj,"QR", jarray);
	json_object_object_add(jobj,"LOCK", lock);

	/* Get length of JSON string */
	msg->size = strlen(json_object_to_json_string(jobj)) + 1;

	/* Allocate memory for JSON message */
	msg->buff = (char *)malloc(msg->size);
	if ( msg->buff == NULL ) {
		free_msg(msg);
		return msg;
	}

	/* Load JSON string into buffer */
	fprintf(stdout, "%s\n", json_object_to_json_string(jobj));
	snprintf(msg->buff, msg->size, "%s", json_object_to_json_string(jobj));

	return msg;
}	

struct message * getXY(struct shm_info * qr_si) {

	struct qr_info *qi = (struct qr_info *)qr_si->shm_ptr;

	/* Create new message */
	struct message *msg = newMsg(XY);
	if ( msg == NULL ) {
		return msg;
	}

	/* Signal camerad process and wait */
	if ( (killWait(SIGUSR2, SIG_TIMEOUT, qi->pid)) != 0 ) {
		free_msg(msg);
		return msg;
	} 

	/* Create JSON int objects */
	json_object *x = json_object_new_int(qi->x);
	json_object *y = json_object_new_int(qi->y);
	json_object *lock = json_object_new_int(qi->lock);

	/* JSON Key/Value pair object */
	json_object * jobj = json_object_new_object();

	/* Add key/values to object */
	json_object_object_add(jobj,"X", x);
	json_object_object_add(jobj,"Y", y);
	json_object_object_add(jobj,"LOCK", lock);

	/* Get length of JSON string */
	msg->size = strlen(json_object_to_json_string(jobj)) + 1;

	/* Allocate memory for JSON message */
	msg->buff = (char *)malloc(msg->size);
	if ( msg->buff == NULL ) {
		free_msg(msg);
		return msg;
	}

	/* Load JSON string into buffer */
	snprintf(msg->buff, msg->size, "%s", json_object_to_json_string(jobj));

	return msg;
}

struct options * init_options( int argc, char *argv[] ) {

	struct options *opts;
	int gopt;

	/* Allocate memory for structure */
	opts = (struct options *)malloc(sizeof(struct options));
	if ( opts == NULL ) {
		logELK(elk, ERROR, "failed to allocate memory for options struct");
		return NULL;
	}

	/* Initialize struct */
	memset(opts, '\0', sizeof(struct options));

	/* Get passed arguments */
	while ( (gopt = getopt(argc, argv, "p:P:d")) != EOF ) {

		switch (gopt) {
			case 'P':
				opts->pid_file = (char *)malloc(strlen(optarg) + 1);
				strncpy(opts->pid_file, optarg, strlen(optarg) + 1);
				break;
			case 'd':
				opts->background = 1;
				break;
			case 'p':
				opts->port = strtoul(optarg, 0, 10);
				break;
			default:
				logELK(elk, WARN, "Unknown argument(s)");
		}
	}

	/* Check for a valid port value */
	if ( opts->port <= 0 ||  opts->port > 65536) {
		logELK(elk, ERROR, "Please specify a valid port");
		free(opts);
		return NULL;
	}

	/* Run in the background */
	if ( opts->background ) {
		if ( daemon(0,0) != 0 ) {
			logELK(elk, ERROR, "Failed to fork to background");
			free(opts);
			return NULL;
		}
	}

	/* Write pid out to file */
	if ( opts->pid_file != NULL ) {
		if ( recordPID(opts->pid_file, getpid()) != 0 ) {
			logELK(elk, ERROR, "Error recording PID");
			free(opts);
			return NULL;
		}
	}

	return opts;
}


void cleanup( struct net_config *nc ) {

	/* Sanity check */
	if ( nc == NULL ) {
		return;
	}

	/* Cleanup opts */
	if ( nc->opts != NULL ) {
		if ( nc->opts->pid_file != NULL ) {
			free(nc->opts->pid_file);
		}
		free(nc->opts);
	}

	/* Cleanup shared memory */
	detach_shm(nc->si);

	/* Cleanup server socket */
	cleanServerSock(nc->sc);

	/* Cleanup ELK logging */
	cleanupELK(elk);

	free(nc);
}

	

int killWait(int signal, int timeout, pid_t pid) {

	sigset_t set;
	timespec to;

	/* Setup signal set */
	sigemptyset(&set);
	sigaddset(&set, signal);
	sigprocmask(SIG_BLOCK, &set, NULL);

	/* Set timeout values */
        to.tv_sec = timeout;
        to.tv_nsec = 0;

        /* Send signal to decode barcode */
        int ret = kill(pid, signal);
        if ( ret != 0 ) {
                return -1;
        }

        /* Wait for signal */
        if ( (sigtimedwait(&set, NULL, &to)) == -1 ) {
		return -1;
        }

	return 0;
}

int recordPID(char * pid_file, pid_t pid) {

        /* Open file for writing */
        FILE *fp = fopen(pid_file, "w");
        if ( fp == NULL ) {
                return 1;
        }

        /* Write pid out to file */
        fprintf(fp, "%d\n", getpid());

        /* Close file handle */
        fclose(fp);

        return 0;
}
