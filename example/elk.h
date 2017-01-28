#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <json/json.h>
#include <time.h>
#include <string.h>


/* ISO 8601 date/time string length */
#define ISO8601_SIZE 20

/* ELK Connection info */
#define ELK_TYPE "TapeEncabulator"
#define ELK_PORT 7000
#define ELK_ADDR "10.10.205.109"

/* ELK Logging Levels */
enum Level {
	TRACE,
	DEBUG,
	INFO,
	WARN,
	ERROR,
	FATAL
};

/* ELK field names */
struct elk_fields {
        const char *type; 
        const char *app;
        const char *level;
        const char *timestamp;
        const char *msg;
};

/* ELK connection information */
struct elk_sock {
        int sock;
	bool log_stderr;
	const char *app;
        const char *type;
        struct sockaddr_in addr;
	struct elk_fields fields;
};

/*
 * Function Prototypes
 */

/* Get GMT date/time in ISO8601 format */
char * getGMT(time_t);

/* Send log message to ELK stack */
int logELK(struct elk_sock *, enum Level, const char *, ...);

/* Initialize logging framework */
struct elk_sock * initELK(const char *, unsigned int, const char *, const char *, bool);

/* Return log level */
const char * getLevel(enum Level);

/* Clean-up ELK connection */
void cleanupELK(struct elk_sock *);
