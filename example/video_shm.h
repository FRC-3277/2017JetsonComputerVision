#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define GSTREAM_SHM_NAME "/gstream_shm"
#define GSTREAM_SEM_NAME "/gstream"

#define QR_SHM_NAME "/qr_shm"
#define QR_SEM_NAME "/qr"

#define LOCKED 1
#define NOLOCK 0

//#define GSTREAM_BUFF_SIZE 1382400
#define GSTREAM_BUFF_SIZE 2764800

/* Wrapper struct containing shm and sem pointers */
struct shm_info {
	char *sem_name;
	char *shm_name;
	int fd;
	sem_t *sem;
	void *shm_ptr;
};

/* Gstreamer raw frame data */
struct gst_stream {
	size_t frame;
	unsigned int x;
	unsigned int y;
	char buff[GSTREAM_BUFF_SIZE];
};

/* QR position and decode info */
struct qr_info {
        int x;
        int y;
        int lock;
        pid_t pid;
        char qr[26];
};

/* Unlink/close sem/shm resources - server only */
void destroy_shm( struct shm_info * );

/* Detach from shm objects */
void detach_shm( struct shm_info * );

/* Free shm_info resources - client/server */
void clean_shm( struct shm_info *, struct stat * );

/* Attach to sem/shm resources - client only */
struct shm_info * attach_shm( const char *,  const char * );

/* Create sem/shm resources - server only */
struct shm_info * create_shm( const char *, const char *, size_t );
