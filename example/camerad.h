#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <cmath>
#include "video_shm.h"
#include "elk.h"
#include <signal.h>
#include <zbar.h>

using namespace zbar;
using namespace cv;
using namespace std;

#define MARKER_THRESHOLD .10
#define MARKER_INNER	 .15
#define MARKER_MIDDLE    .51

#define CANNY_LO  100
#define CANNY_HI  300
#define THRESHOLD 135

#define UPDATE	1
#define IGNORE	0

/* Global logging handle */
struct elk_sock * elk;

/* Getopt variable struct */
struct options {
        char * pid_file;
        int background;
};

/* Parent struct */
struct camera_config {
        struct options *opts;
        struct shm_info *qr_si;
        struct shm_info *gst_si;
};

/* Structure containing caller information */
struct caller {
	int signal;
	pid_t pid;
};

/* Structure containing QR Marker info */
struct marker {
	unsigned int oDist;
        unsigned int contour;
	Moments mu;
	Point2f mc;
};

/* Structure contining QR marker contour area size */
struct area {
	double area;
	unsigned int contour;
};

/*
Function Prototypes
*/

/* Get XY Coords of the QR barcode */
int getCoords(Mat, struct qr_info *, int);

/* Find center with 2 or 3 markers found */
Point2f findCenter3( struct marker *, unsigned int);
Point2f findCenter2( struct marker *, unsigned int);

/* Find near/far markers wrt origin */
int getOriginFar(struct marker *, unsigned int);
int getOriginNear(struct marker *, unsigned int);

/* Find and return the midpoint between 2 points */
Point2f getMidpoint(Point2f pt1, Point2f pt2);

/* Identify if a contour is a member of a QR Marker */
bool isMarker(vector<Vec4i> &hierarchy, vector<vector<Point> > &contours, int index, unsigned int size);

/* Get list initial list of contours (markers) that match depth requirements */
void getMarkers(vector<Vec4i> &hierarchy, vector<vector<Point> > &contours, vector<int> &markerIds);

/* Return the distance between 2 points */
float getDistance(Point2f P, Point2f Q);

/* Initialize POSIX shared memory */
void * initMem(const char * name);

/* Get options (parse getopts) */
struct options * init_options( int argc, char *argv[] );

/* Main cleanup entry point */
void cleanup( struct camera_config * );

/* Initialize signal handling */
int initSIG();

/* Custom signal handler */
static void sch(int, siginfo_t *, void *);

/* Get QR code (decode) */
int getQR(Mat image, struct qr_info * shm);

/* Record PID to shared memory and pid file */
int recordPID(char *,pid_t, struct qr_info * qi);

/* Shutdown signal handler */
void sig_handle(int) { exit(0); }
