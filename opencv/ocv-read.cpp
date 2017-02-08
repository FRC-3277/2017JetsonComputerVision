#include <stdio.h>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

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

	imshow("image", image );
	waitKey(0);


	return 5;
}
