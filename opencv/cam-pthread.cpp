#include <pthread.h>
#include <stdio.h>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

pthread_t worker;
pthread_mutex_t lock;
VideoCapture cap(0);

void * grabFrame(void *arg) {

	while (true ) {
		/* Locak access to camera */
		pthread_mutex_lock(&lock);

		/* Grab next frame to keep buffer empty */
		if ( (cap.grab()) == false ) {
			fprintf(stderr, "Failed to grab frame\n");
			return NULL;
		}

		/* Unlock access to camera */
		pthread_mutex_unlock(&lock);
		usleep(20000);
	}
}

int main( int argc, char **argv ) {

	namedWindow("withLight", CV_WINDOW_AUTOSIZE);
	namedWindow("noLight", CV_WINDOW_AUTOSIZE);
	namedWindow("image", CV_WINDOW_AUTOSIZE);

	/* Initialize putex lock */
	if ( pthread_mutex_init(&lock, NULL) != 0 ) {
		fprintf(stderr, "Failed to initialize mutex lock\n");
		return 1;
	}

	/* Open capture device */
	//VideoCapture cap(0);
	if ( !cap.isOpened() ) {
		fprintf(stderr, "Failed to open capture device\n");
		return 1;
	}

	/* Launch pthread */
	int ret = pthread_create(&worker, NULL, &grabFrame, NULL);
	if ( ret != 0 ) {
		fprintf(stderr, "Failed to launch worker thread\n");
		return 1;
	}

	while ( true ) {


		/* Get frame with no light */
		pthread_mutex_lock(&lock);		
		Mat noLight;
		if ( (cap.read(noLight)) == false ) {
			fprintf(stderr, "Failed to read image from camera\n");
			return 1;
		}
		pthread_mutex_unlock(&lock);
		imshow("noLight", noLight); waitKey(0);

		/* Get frame with light */
 		pthread_mutex_lock(&lock);
		Mat withLight;
                if ( (cap.read(withLight)) == false ) {
                        fprintf(stderr, "Failed to read image from camera\n");
                        return 1;
                }
		pthread_mutex_unlock(&lock);
                imshow("withLight", withLight); waitKey(0);

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
		
		imshow("image", image); waitKey(0);
	}
}
