#include "elk.h"

char * getGMT(time_t curtime) {

	/* Sanity check passed-in parameter */
	if ( curtime <= 0 ) {
		fprintf(stderr, "Invalid time specified\n");
		return NULL;
	}

	/* Allocate memory for ISO 8601 formated date/time */
	char * gmt_time = (char *)malloc(ISO8601_SIZE + 1);
	if ( gmt_time == NULL ) {
		return NULL;
	}

	/* Get ISO8601 formated string */
	int len = strftime(gmt_time, ISO8601_SIZE + 1, "%Y-%m-%dT%H:%M:%SZ", gmtime(&curtime));
	if ( len != ISO8601_SIZE ) {
		fprintf(stderr, "ISO8601 time format size incorrect\n");
		free(gmt_time);
		return NULL;
	}

	return gmt_time;
}

int logELK( struct elk_sock * elk, enum Level elk_level, const char * format, ... ) {

	char * elk_msg;
	va_list args;
	size_t msg_len;

	/* Sanity Checking*/
	if ( elk == NULL ) {
		fprintf(stderr, "ELK logging not initialized\n");
		return -1;
	}
	if ( format == NULL || format == '\0' ) {
		fprintf(stderr, "Invalid ELK_MSG\n");
		return -1;
	}
	if ( elk_level < 1 || elk_level > 6 ) {
		fprintf(stderr, "Invalid ELK_LEVEL\n");
		return -1;
	}

	/* First pass - get string length */
	va_start(args, format);
	msg_len = vsnprintf(NULL, 0, format, args);

	/* Allocate memory for message */
	elk_msg = (char *)malloc(msg_len + 1);
	if ( elk_msg == NULL ) {
		fprintf(stderr, "Error allocating memory for ELK logging\n");
		return -1;
	}

	/* Null the allocation and close the list */
	memset(elk_msg, '\0', msg_len + 1);
	va_end(args);

	/* Second pass - store the string */
	va_start(args, format);
	vsnprintf(elk_msg, msg_len + 1, format, args);
	va_end(args);

	/* JSON Key/Value pair */
	json_object * jobj = json_object_new_object();

	/* Set elk_type */
	json_object *jtype = json_object_new_string(elk->type);
	json_object_object_add(jobj,elk->fields.type, jtype);

	/* Set elk_app */
	json_object *japp = json_object_new_string(elk->app);
	json_object_object_add(jobj, elk->fields.app, japp);

	/* Set elk_message */
	json_object *jmsg = json_object_new_string(elk_msg);
	json_object_object_add(jobj, elk->fields.msg, jmsg);
	free(elk_msg);

	/* Set elk_level */
	json_object *jlevel = json_object_new_string(getLevel(elk_level));
	json_object_object_add(jobj, elk->fields.level, jlevel);

	/* Set elk_timestamp */
	char * elk_time = getGMT(time(NULL));
	if ( elk_time != NULL ) {
		json_object *jtimestamp = json_object_new_string(elk_time);
		json_object_object_add(jobj, elk->fields.timestamp, jtimestamp);
		free(elk_time);
        } else {
		fprintf(stderr, "Failed setting timestamp, defaulting to ELK provided value\n");
	}

	/* Send the log message to ELK */
	size_t mlen = sendto(elk->sock, json_object_to_json_string(jobj), strlen(json_object_to_json_string(jobj)),
                             0, (struct sockaddr*) &elk->addr, sizeof(elk->addr));
	if ( mlen != strlen(json_object_to_json_string(jobj)) ) {
		fprintf(stderr, "Failed sending message to ELK: %s\n", json_object_to_json_string(jobj));
		return -1;
	}

	/* Send log to stderr as well */
	if ( elk->log_stderr ) {
		fprintf(stderr, "%s\n", json_object_to_json_string(jobj));
	}

	return 0;
}

struct elk_sock * initELK( const char * address, unsigned int port, const char * elk_type, const char * elk_app, bool log_stderr ) {

        /* Sanity Checking*/
	if ( address == NULL || address == '\0') {
		fprintf(stderr, "Invalid address\n");
		return NULL;
        }
	if ( elk_type == NULL || elk_type == '\0' ) {
		fprintf(stderr, "Invalid elk_type\n");
		return NULL;
	}
	if ( elk_app == NULL || elk_app == '\0' ) {
		fprintf(stderr, "Invalid elk_app\n");
		return NULL;
	}

	/* Create ELK socket connection structure */
	struct elk_sock * elk = (struct elk_sock *)malloc(sizeof(struct elk_sock));
	if ( elk == NULL ) {
		fprintf(stderr, "Failed initializing memory\n");
		return NULL;
	}

	/* Create a socket */
	elk->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (elk->sock == -1) {
		fprintf(stderr, "Failed creating ELK socket\n");
		free(elk);
		return NULL;
	}

	/* Setup connection information */
	elk->addr.sin_family = AF_INET;
	elk->addr.sin_port = htons(port);
	elk->addr.sin_addr.s_addr = inet_addr(address);

	/* Log to stderr flag */
	elk->log_stderr = log_stderr;

	/* Set default ELK values */
	elk->type = elk_type;
	elk->app  = elk_app;

	/* Set ELK field names */
	elk->fields.type = "type";
	elk->fields.timestamp = "@timestamp";
	elk->fields.app = "app";
	elk->fields.msg = "message";
	elk->fields.level = "level";

	return elk;
}

const char * getLevel( enum Level l ) {

	switch (l) {
		case TRACE:
			return "TRACE";
		case DEBUG:
			return "DEBUG";
		case INFO:
			return "INFO";
		case WARN:
			return "WARN";
		case ERROR:
			return "ERROR";
		case FATAL:
			return "FATAL";
	}

	return NULL;
}

void cleanupELK( struct elk_sock * elk ) {

	if ( elk != NULL ) {
		close(elk->sock);
		free(elk);
	}
}
