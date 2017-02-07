#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

// For an example of threads see http://deathbytape.com/articles/2015/02/03/cpp-threading.html
thread worker;
static mutex theLock;

void * grabFrame(VideoCapture *cap) {

	while (true ) {
		// Lock access to camera.  The lock will release when it falls out of scope.
		lock_guard<mutex> lock(theLock);

		// Grab next frame to keep buffer empty 
		if ( (cap->grab()) == false ) {
			fprintf(stderr, "Failed to grab frame\n");
			return NULL;
		}

		this_thread::sleep_for(chrono::milliseconds(20000));
	}
}

int main( int argc, char **argv ) {

	namedWindow("withLight", CV_WINDOW_AUTOSIZE);
	namedWindow("noLight", CV_WINDOW_AUTOSIZE);
	namedWindow("image", CV_WINDOW_AUTOSIZE);

	// Open capture device
	VideoCapture cap(0);

	//May not be necessary, but suggested from http://stackoverflow.com/questions/30032063/opencv-videocapture-lag-due-to-the-capture-buffer
	cap.set(CV_CAP_PROP_BUFFERSIZE, 1);

	if ( !cap.isOpened() ) {
		fprintf(stderr, "Failed to open capture device\n");
		return 1;
	}

	// Launch thread
	thread worker(grabFrame, &cap);

	while ( true ) {

		// Get frame with no light 
		Mat noLight;
		if ( (cap.read(noLight)) == false ) {
			fprintf(stderr, "Failed to read image from camera\n");
			return 1;
		}
		
		imshow("noLight", noLight); waitKey(0);

		// Get frame with light 
		Mat withLight;
        if ( (cap.read(withLight)) == false ) {
                fprintf(stderr, "Failed to read image from camera\n");
                return 1;
        }
        imshow("withLight", withLight); waitKey(0);

		// Convert images to gray scale 
		cvtColor(noLight,noLight,CV_RGB2GRAY);
		cvtColor(withLight,withLight,CV_RGB2GRAY);

		// Get delta image 
		Mat image;
		subtract(withLight, noLight, image);

		// Reduce noise 
		GaussianBlur(image,image, Size(3,3), 0, 0);

		// Apply threshold
		threshold(image,image, 150, 255, THRESH_BINARY);

		// Apply Canny algorithm
		Canny(image, image, 100, 300, 3);
		
		imshow("image", image); waitKey(0);
	}
  
	worker.join();
}
