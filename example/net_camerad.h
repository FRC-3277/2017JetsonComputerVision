#include "video_shm.h"
#include "net_common.h"
#include <json/json.h>
#include "elk.h"

#define SIG_TIMEOUT  10

struct options {
	char * pid_file;
	int background;
	unsigned long int port;
};

struct net_config {
	struct options *opts;
	struct shm_info *si;
	struct sock_config *sc;
};

/* Global logging handle */
struct elk_sock * elk;

/*
 *Function Prototypes
 */

/* Main entry point */
int main(int argc, char *argv[]);

struct options * init_options(int argc, char *argv[]);

/* IPC communtication */
int killWait(int, int, pid_t);

/* Process client request */
void process(int, struct shm_info *);

/* Get XY coordinates of QR Code */
struct message * getXY(struct shm_info *);

/* Get decoded QR data */
struct message * getQR(struct shm_info *);

/* Record PID to pide file */
int recordPID(char *,pid_t);

/* Shutdown signal handler function */
void sig_handle(int) { exit(0); }

/* Main entry for cleanup */
void cleanup(struct net_config *);
