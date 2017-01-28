#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "video_shm.h"

using namespace std;
using namespace cv;

int main(int argc, char *argv[]) {

	struct shm_info *si;

        /* Initialize shared memory */
        si = attach_shm(GSTREAM_SEM_NAME, GSTREAM_SHM_NAME);
        if ( si == NULL ) {
                fprintf(stderr, "Failed to allocate POSIX shared memroy\n");
                return 1;
        }

	namedWindow( "Image", CV_WINDOW_AUTOSIZE );

	while(1) {

		struct gst_stream *gs = (struct gst_stream *)malloc(sizeof(struct gst_stream));
		if ( gs == NULL ) {
			fprintf(stderr, "Failed to allocate memory\n");
			return 1;
		}

		sem_wait(si->sem);
		memcpy(gs, si->shm_ptr, sizeof(struct gst_stream));
		sem_post(si->sem);

		//fprintf(stderr, "Frame: %lu\n", gs->frame);

		Mat image(Size(gs->x, gs->y), CV_8UC3, (void *)gs->buff, Mat::AUTO_STEP);
		imshow("Image", image); waitKey(5);

		free(gs);
	}

	return 0;
}
