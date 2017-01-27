#include <opencv2/opencv.hpp>
#include <stdio.h>

using namespace cv;
using namespace std;

int main( int argc, char **argv ) {


        namedWindow( "Image", CV_WINDOW_AUTOSIZE );

        /* Open video capture device */
        VideoCapture cap(1);
        if ( !cap.isOpened() ) {
                fprintf(stderr, "Failed opening camera device\n");
                return 1;
        }

        while ( true ) {

                /* Read-in image */
                Mat image;
                if ( (cap.read(image)) == false ) {
                        fprintf(stderr, "Failed to read image from camera\n");
                        break;
                }

                imshow("Image", image); waitKey(5);
        }

        return 0;
}
