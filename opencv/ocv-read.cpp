#include <stdio.h>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

#define THRESHOLD .10

struct marker {
	RotatedRect rect;
	Point2f corners[4];
};

struct target {
	struct marker *a;
	struct marker *b;
	float distance;
};

float getDistance(Point2f P, Point2f Q) {
        return sqrt(pow(abs(P.x - Q.x),2) + pow(abs(P.y - Q.y),2)) ;
}

void cleanup( struct target *targ ) {

        if ( targ->a != NULL ) { free(targ->a); }
        if ( targ->b != NULL ) { free(targ->b); }
        if ( targ != NULL ) { free(targ); }
}

bool isVertical(Point2f large, Point2f small) {

	/* Get rise/run values for slope calc */
	double rise = large.y - small.y;
	double run  = large.x - small.x;

	/* Check for divide-by-zero condition [VERTICAL SLOPE]*/
	if ( run == 0 ) {
		return true;
	}

	/* Calculate slope */
	double slope = fabs(rise/run);
	if ( slope >= 1.5 || slope >= -1.5 ) {
		return true;
	}

	return false;
}

bool isRatio ( float a, float b ) {

	float ratio = a / b;

	/* Check for correct ratio */
	if ( ratio >= (0.5 - THRESHOLD) && ratio <= (0.5 + THRESHOLD) ) {
		return true;
	} else if ( ratio >= (2 - (2 * THRESHOLD)) && ratio <= (2 + (2 * THRESHOLD)) ) {
		return true;
	}

	return false;
}

struct target * findMarkers( vector<vector<Point> > contours ) {

	struct target *targ;

	/* Return early */
        if ( contours.size() < 2 ) {
                fprintf(stdout, "Need at least 2 contours to compare\n");
                return NULL;
        }

	/* Allocate memory for target struct */
	targ = (struct target *)malloc(sizeof(struct target));
	if ( targ == NULL ) {
		fprintf(stderr, "Failed allocating memory for target struct\n");
		return NULL;
	}

	/* Allocate memory for marker structs */
	targ->a = (struct marker *)malloc(sizeof(struct marker));
	if ( targ->a == NULL ) {
		fprintf(stderr, "Failed allocating memory for marker struct\n");
		return NULL;
	}
	targ->b = (struct marker *)malloc(sizeof(struct marker));
	if ( targ->b == NULL ) {
		fprintf(stderr, "Failed allocating memory for marker struct\n");
		return NULL;
	}

	/* Vector of rects for each contour */
	vector<RotatedRect> rect(contours.size());
	for ( unsigned int i = 0; i < contours.size(); i++ ) {
		rect[i] = minAreaRect(contours[i]);
	}

	/* Loop through contours and locate markers */
	for ( unsigned int i = 0; i < rect.size(); i++ ) {
		for ( unsigned int j = i; j < rect.size(); j++ ) {

			if ( i == j ) { continue; }

			RotatedRect a = rect[i];
			RotatedRect b = rect[j];

			/* Locate markers based on slope of centers and ratio of areas */
			if ( isRatio(a.size.area(), b.size.area()) && isVertical(a.center, b.center) ) {
				targ->a->rect = minAreaRect(contours[i]);
				targ->b->rect = minAreaRect(contours[i+1]);
				targ->a->rect.points(targ->a->corners);
				targ->b->rect.points(targ->b->corners);
				targ->distance = getDistance(a.center, b.center);
				return targ;
			}
		}
	}

	/* Free memory before returning NULL */
	cleanup(targ);

	return NULL;
}

int main(int argc, char *argv[] ) {

	namedWindow("noLight", CV_WINDOW_AUTOSIZE);	
	namedWindow("withLight", CV_WINDOW_AUTOSIZE);	
	namedWindow("image", CV_WINDOW_AUTOSIZE);	

	Mat noLight;
	noLight = imread("../images/noLight.jpg", CV_LOAD_IMAGE_COLOR);
	if ( !noLight.data ) {
		fprintf(stderr, "Could not open file\n");
		return 1;
	}

        Mat withLight;
        withLight = imread("../images/withLight.jpg", CV_LOAD_IMAGE_COLOR);
        if ( !withLight.data ) {
                fprintf(stderr, "Could not open file\n");
                return 1;
        }


	imshow("noLight", noLight );
	imshow("withLight", withLight);

        /* Convert images to gray scale */
        cvtColor(noLight,noLight,CV_RGB2GRAY);
        cvtColor(withLight,withLight,CV_RGB2GRAY);

        /* Get delta image */
        Mat image;
        subtract(withLight, noLight, image);

        /* Reduce noise */
        GaussianBlur(image,image, Size(3,3), 0, 0);

        /* Apply thresholdig */
        threshold(image,image, 150, 255, THRESH_BINARY);

        /* Apply Canny alg */
        Canny(image, image, 100, 300, 3);

	/* Find Contours */
	vector<vector<Point> > contours;
	findContours(image, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	/* Locate target */
	struct target *targ = findMarkers(contours);
	if ( targ == NULL ) {
		fprintf(stderr, "Failed to locate target\n");
		return 1;
	}

	/* Draw boxes */
        for ( int j = 0; j < 4; j++ ) {
        	line(image, targ->a->corners[j], targ->a->corners[(j+1)%4], Scalar(255,255,255));
        	line(image, targ->b->corners[j], targ->b->corners[(j+1)%4], Scalar(255,255,255));
	}

	/* Draw center line */
	line(image, targ->a->rect.center, targ->b->rect.center, Scalar(255,255,255));

	fprintf(stdout, "Line Distance: %f\n", targ->distance);

	imshow("image", image );
	waitKey(0);

	cleanup(targ);
	return 5;
}
