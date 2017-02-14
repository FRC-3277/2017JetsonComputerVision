#include "jetsonGPIO.h"
#include <opencv2/opencv.hpp>
#include <pthread.h>
#include <stdio.h>

using namespace cv;
using namespace std;

/*
 * Global defines
 */

#define THRESHOLD .10

/*
 * Structure definitions
 */

pthread_t worker;
pthread_mutex_t lock;

struct marker {
	RotatedRect rect;
	Point2f corners[4];
};

struct target {
	struct marker *a;
	struct marker *b;
	float distance;
};

/*
 * Function prototypes
 */

/* Threading function */
void * grabFrame(void *);

/* Get distance between 2 points */
float getDistance(Point2f, Point2f);

/* Clean-up function (free mem allocations) */
void cleanup(struct target *);

/* Check for vertical slope between 2 points */
bool isVertical(Point2f, Point2f);

/* Check for a ratio for 2:1 between numbers */ 
bool isRatio (float, float);

/* Find target markers */
struct target * findMarkers(vector<vector<Point> >);
