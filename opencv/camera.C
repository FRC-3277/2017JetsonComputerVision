#include <opencv2/opencv.hpp>
#include <stdio.h>

using namespace cv;
using namespace std;

int main( int argc, char **argv ) {


        namedWindow( "withLight", CV_WINDOW_AUTOSIZE );
        namedWindow( "noLight", CV_WINDOW_AUTOSIZE );
        namedWindow( "Image", CV_WINDOW_AUTOSIZE );

        /* Open video capture device */
        VideoCapture cap(1);
        if ( !cap.isOpened() ) {
                fprintf(stderr, "Failed opening camera device\n");
                return 1;
        }

	cap.set(CV_CAP_PROP_BUFFERSIZE, 1);

        while ( true ) {


                /* Read-in image */
                Mat noLight;
                if ( (cap.read(noLight)) == false ) {
                        fprintf(stderr, "Failed to read image from camera\n");
                        break;
                }
		imshow("noLight", noLight); waitKey(0);

		/* Read-in image */
		Mat withLight;
		if ( (cap.read(withLight)) == false ) {
			fprintf(stderr, "Failed to read image from camera\n");
			break;
		}
		imshow("withLight", withLight); waitKey(0);


		Mat image;
		subtract(noLight, withLight, image);
		imshow("Image", image); waitKey(0);
		continue;


		/* Convert image to Gray Scale */
		cvtColor(image,image,CV_RGB2GRAY);

	        /* Reduce noise with a kernel 3x3 */
		GaussianBlur(image, image, Size(3,3), 0, 0);

		/* Apply threshold (polarization) */
		threshold(image,image,150,255,THRESH_BINARY);


		/* Apply Canny Edge detection */
		Canny(image, image, 100, 300, 3);


                imshow("Image", image); waitKey(5);
        }

        return 0;
}
