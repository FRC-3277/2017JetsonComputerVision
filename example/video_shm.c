#include "video_shm.h"

struct shm_info * attach_shm( const char * sem_name, const char * shm_name ) {

	struct shm_info *si;
	struct stat *s = NULL;

	/* Allocate memory for structs */
	si = (struct shm_info *)malloc(sizeof(struct shm_info));
	if ( si == NULL ) {
		fprintf(stderr, "Failed to allocate memory\n");
		clean_shm(si, s);
		return NULL;
	}

	s = (struct stat *)malloc(sizeof(struct stat));
	if ( s == NULL ) {
		fprintf(stderr, "Failed to allocate memory\n");
		clean_shm(si, s);
		return NULL;
	}

	/* Initialize structs */
	memset(si, '\0', sizeof(struct shm_info));
	memset(s, '\0', sizeof(struct stat));

	/* Allocate memory */
	si->sem_name = (char *)malloc(strlen(sem_name) + 1);
	if ( si->sem_name == NULL ) {
		fprintf(stderr, "Failed to allocate memory\n");
		clean_shm(si, s);
		return NULL;
	}

	si->shm_name = (char *)malloc(strlen(shm_name) + 1);
	if ( si->shm_name == NULL ) {
		fprintf(stderr, "Failed to allocate memory\n");
		clean_shm(si, s);
		return NULL;
	}

	/* Copy strings into memory allocations */
	strncpy(si->sem_name, sem_name, strlen(sem_name) + 1);
	strncpy(si->shm_name, shm_name, strlen(shm_name) + 1);

	/* Create the shared memory segment */
	si->fd = shm_open(shm_name, O_RDWR, 0600);
	if ( si->fd == -1 ) {
		fprintf(stdout, "Failed to open shared memory segment: %s\n", strerror(errno));
		clean_shm(si, s);
		return NULL;
	}

	/* Stat file and get size */
	fstat(si->fd, s);

	/* Map-in shared memory segment */
	si->shm_ptr = mmap(NULL, s->st_size, PROT_READ | PROT_WRITE, MAP_SHARED, si->fd, 0);
	if ( si->shm_ptr == MAP_FAILED ) {
		fprintf(stderr, "Failed to allocate shared memeory segment: %s\n", strerror(errno));
		clean_shm(si, s);
		return NULL;
	}

	/* Open semaphore */
	si->sem = sem_open(sem_name, S_IRUSR | S_IWUSR, 0);
	if ( si->sem == SEM_FAILED ) {
		fprintf(stderr, "Failed to open shared semaphore: %s\n", strerror(errno));
		clean_shm(si, s);
		return NULL;
	}

	free(s);
	return si;
}



struct shm_info * create_shm( const char * sem_name, const char * shm_name, size_t shm_size ) {

	struct shm_info *si;

	/* Allocate memory for structs */
	si = (struct shm_info *)malloc(sizeof(struct shm_info));
	if ( si == NULL ) {
		fprintf(stderr, "Failed to allocate memory\n");
		clean_shm(si, NULL);
		return NULL;
	}

	/* Initialize struct */
	memset(si, '\0', sizeof(struct shm_info));

	/* Allocate memory */
	si->sem_name = (char *)malloc(strlen(sem_name) + 1);
	if ( si->sem_name == NULL ) {
		fprintf(stderr, "Failed to allocate memory\n");
		clean_shm(si, NULL);
                return NULL;
        }

        si->shm_name = (char *)malloc(strlen(shm_name) + 1);
        if ( si->shm_name == NULL ) {
                fprintf(stderr, "Failed to allocate memory\n");
                clean_shm(si, NULL);
                return NULL;
        }

        /* Copy strings into memory allocations */
        strncpy(si->sem_name, sem_name, strlen(sem_name) + 1);
        strncpy(si->shm_name, shm_name, strlen(shm_name) + 1);

        /* Create the shared memory segment */
        si->fd = shm_open(shm_name, O_EXCL | O_CREAT | O_RDWR, 0600);
        if ( si->fd == -1 ) {
                fprintf(stdout, "Failed to open shm segment: %s\n", strerror(errno));
		clean_shm(si, NULL);
                return NULL;
        }

        /* Set segment size */
        if ( ftruncate(si->fd, shm_size) == -1 ) {
		fprintf(stderr, "Failed to truncate shm segment: %s\n", strerror(errno));
		clean_shm(si, NULL);
		return NULL;
	}

        /* Map-in shared memory segment */
        si->shm_ptr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, si->fd, 0);
        if ( si->shm_ptr == MAP_FAILED ) {
		fprintf(stderr, "Failed to allocate shared memeory segment: %s\n", strerror(errno));
		clean_shm(si, NULL);
                return NULL;
        }

	/* Open semaphore */
	si->sem = sem_open(sem_name, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
	if ( si->sem == SEM_FAILED ) {
		fprintf(stderr, "Failed to open shared semaphore: %s\n", strerror(errno));
		clean_shm(si, NULL);
		return NULL;
	}

	return si;
}

void destroy_shm( struct shm_info * si ) {

	/* Sanity check for NULL value */
	if ( si == NULL ) {
		return;
	}

	/* Close file descriptor */
	if ( close(si->fd) != 0 ) {
		fprintf(stderr, "Failed to close shared memory file descriptor: %s\n", strerror(errno));
	}

	/* Unlink shared memory */
	if ( shm_unlink(si->shm_name) != 0 ) {
		fprintf(stderr, "Failed to unlink shared memory: %s\n", strerror(errno));
	}

	/* Close semaphore */
	if ( sem_close(si->sem) != 0 ) {
		fprintf(stderr, "Failed to close semaphore: %s\n", strerror(errno));
	}

	/* Remove the named semaphone */
	if ( sem_unlink(si->sem_name) != 0 ) {
		fprintf(stderr, "Failed to unlink semaphore: %s\n", strerror(errno));
	}

	clean_shm(si, NULL);
}

void clean_shm( struct shm_info * si, struct stat *s ) {

	if ( s != NULL ) {
		free(s);
	}

	if ( si != NULL ) {
		if ( si->shm_name != NULL ) {
			free(si->shm_name);
		}
		if ( si->sem_name != NULL ) {
			free(si->sem_name);
		}
		free(si);
	}
}

void detach_shm( struct shm_info * si ) {
        clean_shm(si, NULL);
}


