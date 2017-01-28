#include "camerad.h"

struct caller * call;

int main( int argc, char **argv ) {

	struct camera_config *cc;

        /* Fire-up logging */
        elk = initELK(ELK_ADDR, ELK_PORT, ELK_TYPE, "CameraService", true);
        if ( elk == NULL ) {
                fprintf(stderr, "Failed to initialize logging\n");
		return 1;
        }

        /* Allocate memory for struct */
	cc = (struct camera_config *)malloc(sizeof(struct camera_config));
	if ( cc == NULL ) {
		logELK(elk, FATAL, "Error allocating memory for camera_config struct");
		cleanup(cc);
		return 1;
	}

	/* Initialize struct */
	memset(cc, '\0', sizeof(struct camera_config));

	/* Get passed-in opts */
	cc->opts = init_options(argc, argv);
	if ( cc->opts == NULL ) {
		logELK(elk, FATAL, "Failed to parsing/setting  getopts");
		cleanup(cc);
		return 1;
	}

	/* Setup shared memory - GST and QR*/
	cc->qr_si = (struct shm_info *)create_shm(QR_SEM_NAME, QR_SHM_NAME, sizeof(struct qr_info));
	if ( cc->qr_si == NULL ) {
		logELK(elk, FATAL, "Shared memory initialization failed");
		cleanup(cc);
		return 1;
	}
        cc->gst_si = (struct shm_info *)attach_shm(GSTREAM_SEM_NAME, GSTREAM_SHM_NAME);
        if ( cc->gst_si == NULL ) {
                logELK(elk, FATAL, "Shared memory initialization failed");
		cleanup(cc);
                return 1;
        }

        /* Setup signal handlers */
        if ( initSIG() != 0 ) {
                logELK(elk, FATAL, "Failed registering signal handler");
                cleanup(cc);
                return 1;
        }

	/* Pointer for ease-of-access */
	struct shm_info *gst_si = cc->gst_si;
	struct qr_info *qi = (struct qr_info *)cc->qr_si->shm_ptr;

	// FIXME: This should go away when using a semaphore
	qi->pid = getpid();

        /* Create windows for debug information */
        namedWindow( "Image", CV_WINDOW_AUTOSIZE );
        namedWindow( "thresh", CV_WINDOW_AUTOSIZE );
	namedWindow( "canny", CV_WINDOW_AUTOSIZE );

	while ( true ) {

		/* Allocate memory for raw frame data */
		struct gst_stream *gs = (struct gst_stream *)malloc(sizeof(struct gst_stream));
		if ( gs == NULL ) {
			logELK(elk, ERROR, "Failed to allocate memory for raw frame data");
			return 1;
		}

		/* Get a copy of the raw frame data from shm */
		sem_wait(gst_si->sem);
		memcpy(gs, gst_si->shm_ptr, sizeof(struct gst_stream));
		sem_post(gst_si->sem);

		/* Convert raw data into OpenCV Mat opbject */
		Mat image(Size(gs->x, gs->y), CV_8UC3, (void *)gs->buff, Mat::AUTO_STEP);

		/* Rotate image CW 90 degrees */
		transpose(image,image);
		flip(image,image,1);

		/* Check for signal */
		switch ( call->signal ) {
                        case SIGUSR1:
				call->signal = 0;
				qi->lock = getQR(image, qi);
				kill(call->pid, SIGUSR1);
				break;
			case SIGUSR2:
				call->signal = 0;
				qi->lock = getCoords(image, qi, UPDATE);
				kill(call->pid, SIGUSR2);
				break;
			default:
				getCoords(image, qi, IGNORE);
				break;
		}

		/* Free raw frame data */
		image.release();
		free(gs);
		
	}

	cleanup(cc);
	return 0;
}

int recordPID(char * pid_file, pid_t pid) {

	/* Write out to file */
	if ( pid_file != NULL ) {
		
		FILE *fp = fopen(pid_file, "w");
		if ( fp == NULL ) {
			return 1;
		}

        	fprintf(fp, "%d\n", getpid());
        	fclose(fp);
	}

	return 0;
}

void cleanup( struct camera_config *cc ) {

        /* Sanity check */
        if ( cc == NULL ) {
                return;
        }

        /* Cleanup opts */
        if ( cc->opts != NULL ) {
                if ( cc->opts->pid_file != NULL ) {
                        free(cc->opts->pid_file);
                }
                free(cc->opts);
        }

        /* Cleanup shared memory */
        detach_shm(cc->qr_si);
	destroy_shm(cc->gst_si);

	/* Cleanup ELK logging */
	cleanupELK(elk);

        free(cc);
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
        while ( (gopt = getopt(argc, argv, "P:d")) != EOF ) {

                switch (gopt) {
                        case 'P':
                                opts->pid_file = (char *)malloc(strlen(optarg) + 1);
                                strncpy(opts->pid_file, optarg, strlen(optarg) + 1);
                                break;
                        case 'd':
                                opts->background = 1;
                                break;
                        default:
				logELK(elk, ERROR, "Unknown argument(s)");
                }
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

int initSIG() {

	/* Register signal handlers */
	signal(SIGHUP,  sig_handle);
	signal(SIGINT,  sig_handle);
	signal(SIGKILL, sig_handle);

	/* Initialize global caller struct */
	call = (struct caller*)malloc(sizeof(struct caller));
	memset(call, '\0', sizeof(struct caller));

	/* Initialize sigaction struct */
        struct sigaction act;
        memset(&act, '\0', sizeof(struct sigaction));

	/* Register handler func and set flags */
        act.sa_sigaction = &sch;
        act.sa_flags = SA_SIGINFO;

	/* Register custom sig handler and return */
        sigaction(SIGUSR1, &act, NULL);
        sigaction(SIGUSR2, &act, NULL);

	//FIXME: Check for sigaction return
	return 0;
}

int getQR(Mat image, struct qr_info * qi) {

	/* Setup QR Scanner */
	ImageScanner scanner;
	scanner.set_config(ZBAR_NONE, ZBAR_CFG_ENABLE, 1);

	/* Convert image to Gray */
	Mat gray(image.size(), CV_MAKETYPE(image.depth(), 1));
	cvtColor(image,gray,CV_RGB2GRAY);

	/* Convert image to raw and wrap */
	uchar *raw = (uchar *)gray.data;
	Image wrap(gray.cols, gray.rows, "Y800", raw, gray.cols * gray.rows);

	/* Scan for barcodes */
	int count = scanner.scan(wrap);
	memset(qi->qr, '\0', sizeof(qi->qr));

	/* Make sure we only found 1 QR barcode */
	if ( count < 1 ) {
		logELK(elk, INFO, "No QR symbols found");
		return NOLOCK;
	}

	if ( count > 1 ) {
		logELK(elk, INFO, "More than 1 QR symbol found");
		return NOLOCK;
	}

	/* Get first (and only) symbol */
	Image::SymbolIterator symbol = wrap.symbol_begin();
	std::string data = symbol->get_data();

	/* Sanity check string length */
	if ( symbol->get_data_length() > sizeof(qi->qr) ) {
		logELK(elk, INFO, "Decoded QR string is too long");
		return NOLOCK;
	}

	/* Send QR into shared memory and notify caller */
	strncpy(qi->qr, data.c_str(), sizeof(qi->qr));

	/* Clean-up and return */
	wrap.set_data(NULL, 0);
	return LOCKED;
}

static void sch(int sig, siginfo_t *siginfo, void *context) {
	call->signal = siginfo->si_signo;
	call->pid = siginfo->si_pid;
}

int getCoords(Mat image, struct qr_info * qi, int shm_update) {

	Point2f center;
	vector<int> markerIds;
	vector<Vec4i> hierarchy;
	vector<vector<Point> > contours;
	struct marker *markers;

	/* Convert image to Gray Scale */
	Mat temp(image.size(), CV_MAKETYPE(image.depth(), 1));
	cvtColor(image,temp,CV_RGB2GRAY);

	/* Reduce noise with a kernel 3x3 */
	GaussianBlur(temp, temp, Size(3,3), 0, 0);

	/* Apply threshold (polarization) */
	threshold(temp,temp,THRESHOLD,255,THRESH_BINARY);
	imshow("thresh", temp); waitKey(5);

	/* Dilate and erode to remove excess noise */
	dilate(temp, temp, Mat(), Point(-1, -1), 1, 1, 1);
	erode(temp, temp, Mat(), Point(-1, -1), 1, 1, 1);

	/* Apply Canny Edge detection */
	Canny(temp, temp, CANNY_LO, CANNY_HI, 3);
	imshow("canny", temp); waitKey(5);

	/* Find contours with hierarchy info */
	findContours(temp, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);

	/* Locate QR corner markers */
	getMarkers(hierarchy, contours, markerIds);

	/* Structure containing marker information */
	markers = (marker *)malloc(sizeof(struct marker) * markerIds.size());
	for ( unsigned int i = 0; i < markerIds.size(); i++ ) {
		Moments mom = moments(contours[markerIds[i]], false);
		markers[i].mu = mom;
		markers[i].contour = markerIds[i];
		markers[i].mc = Point2f(mom.m10/mom.m00 , mom.m01/mom.m00);
		markers[i].oDist = getDistance(Point2f(0,0), markers[i].mc);
		drawContours(image, contours, markers[i].contour, Scalar(255,200,0), 2, 8, hierarchy, 0);
	}

	/* Find center if 2 OR 3 markers were found */
	if ( markerIds.size() == 2 ) {
		center = findCenter2(markers, markerIds.size());
	} else if ( markerIds.size() == 3 ) {
		center = findCenter3(markers, markerIds.size());
	} else {
		free(markers);
		copyMakeBorder(image, image, 15,15,15,15, BORDER_CONSTANT, Scalar(0,0,255));
		imshow("Image", image); waitKey(5);
		return NOLOCK;
	}

	/* Update coords in shared-memory if requested */
	if ( shm_update == UPDATE ) {
		qi->y = center.y - (image.rows / 2);
		qi->x = center.x - (image.cols / 2);
	}

	/* Draw mid-point and status border */
	circle( image, center, 2, Scalar(0,0,255), -1, 8, 0 );
	copyMakeBorder(image, image, 15,15,15,15, BORDER_CONSTANT, Scalar(0,255,0));
	imshow("Image", image); waitKey(5);

	free(markers);
	return LOCKED;
}

Point2f findCenter3( struct marker *markers, unsigned int count ) {

	Point2f center;
	double tempDist, distance = 0;

	tempDist = getDistance(markers[0].mc, markers[1].mc);
	if ( tempDist > distance ) {
		distance = tempDist;
		center = getMidpoint(markers[0].mc, markers[1].mc);
	}

	tempDist = getDistance(markers[1].mc, markers[2].mc);
        if ( tempDist > distance ) {
                distance = tempDist;
                center = getMidpoint(markers[1].mc, markers[2].mc);
        }

	tempDist = getDistance(markers[2].mc, markers[0].mc);
        if ( tempDist > distance ) {
                distance = tempDist;
                center = getMidpoint(markers[2].mc, markers[0].mc);
        }

	return center;
}

Point2f findCenter2( struct marker *markers, unsigned int count ) {

	Point2f center;

	/* Find near/far markers wrt origin */
	int near = getOriginNear(markers, count);
	int far  = getOriginFar(markers, count);

	/* Get rise/run values for slope calc */
	double rise = markers[near].mc.y - markers[far].mc.y;
	double run = markers[near].mc.x - markers[far].mc.x;

	/* Find midpoint and midpoint distance between mass-centers */
	Point2f midpoint = getMidpoint(markers[near].mc, markers[far].mc);
	int midDist = getDistance(markers[near].mc, midpoint);

	/* Check for divide-by-zero condition [VERTICAL SLOPE]*/
	if ( run == 0 ) {
		center.y = midpoint.y;
		center.x = midpoint.x + midDist;
		return center;
	}

	/* Calculate slope */
	double slope = fabs(rise/run);

	// Slope is Horizontal
	if ( slope >= 0 && slope < 0.5 ) {
		int dmcx = markers[far].mc.x - markers[near].mc.x;
		int dmcy = markers[far].mc.y - markers[near].mc.y;
		center.x = midpoint.x - (dmcy/2);
		center.y = midpoint.y + (dmcx/2);
		return center;
	// Slope is Diagonal
	} else if ( slope >= 0.5 && slope < 1.5 ) {
		return midpoint;
	// Slope is Vertical
	} else if ( slope >= 1.5 ) {
		int dmcx = markers[near].mc.x - markers[far].mc.x;
		int dmcy = markers[near].mc.y - markers[far].mc.y;
		center.x = midpoint.x - (dmcy/2);
		center.y = midpoint.y + (dmcx/2);
		return center;
	// We should never get here
	} else {
		return Point2f(-1,-1);
	}
}

int getOriginNear(struct marker *markers, unsigned int count) {

	double distance;
	int markerID = 0;

	/* Get initial distance value */
	distance = getDistance(Point2f(0,0), markers[0].mc);

	/* Compare origin distance to other marker */
	if ( distance > getDistance(Point2f(0,0), markers[1].mc)) {
		markerID = 1;
	}

	return markerID;
}

int getOriginFar(struct marker *markers, unsigned int count) {

        double distance;
        int markerID = 0;

        /* Get initial distance value */
        distance = getDistance(Point2f(0,0), markers[0].mc);

        /* Compare origin distance to other marker */
        if ( distance < getDistance(Point2f(0,0), markers[1].mc)) {
                markerID = 1;
        }

        return markerID;
}	

void getMarkers(vector<Vec4i> &hierarchy, vector<vector<Point> > &contours, vector<int> &markerIds) {

	/* Loop through contour hierarchy trees */
	for ( unsigned int i = 0; i < contours.size(); i++ ) {

		/* Get depth of tree */
		unsigned int count = 0;
		for ( int j = i; j != -1; j = hierarchy[j][2] ) {
			count++;
		}

		/* Markers have a depth of 6 */
		if ( count == 6 ) {

			/* Calc 10% epeilon before straitening countours */
			double epsilon = 0.1 * arcLength(contours[i], true);
			approxPolyDP(contours[i], contours[i], epsilon, true);

			/* Check area ratio(s) of markers to validate */
			if ( isMarker(hierarchy, contours, i, 6) ) {
				markerIds.push_back(i);
			}
		}
	}
}

bool isMarker(vector<Vec4i> &hierarchy, vector<vector<Point> > &contours, int index, unsigned int size) {

	/* Allocate memory for structures */
	struct area *areas = (area *)malloc(sizeof(struct area) * size);
	double delta, area;

	/* Calculate area for all contours */
	unsigned int count = 0;
	for ( int i = index; i != -1; i = hierarchy[i][2] ) {
		areas[count].contour = i;
		areas[count].area = contourArea(contours[i], false);
		count++;
	}

	/* Detect false positives */
	area = (areas[2].area / areas[0].area);
	delta = abs(area - MARKER_MIDDLE);
	if ( delta > MARKER_THRESHOLD ) {
		free(areas);
		return false;
	}

	/* Detect false positives */
        area = (areas[4].area / areas[0].area);
        delta = abs(area - MARKER_INNER);
        
	if ( delta > MARKER_THRESHOLD ) {
                free(areas);
                return false;
        }

	free(areas);
	return true;
}

Point2f getMidpoint(Point2f pt1, Point2f pt2) {

	Point2f mid;

	mid.x = (pt1.x + pt2.x) / 2;
	mid.y = (pt1.y + pt2.y) / 2;

	return mid;
}

float getDistance(Point2f P, Point2f Q) {
	return sqrt(pow(abs(P.x - Q.x),2) + pow(abs(P.y - Q.y),2)) ;
}
