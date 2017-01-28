#include <HCNetSDK.h>
#include <stdio.h>
#include <gst/gst.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <gst/gstbuffer.h>

#include "elk.h"
#include "video_shm.h"

/* Global logging handle */
struct elk_sock * elk;

/*
 * Structure Definitions
 */


/* HIK camera login information */
struct hik_conf {
	int   port;
	char *host;
	char *user;
	char *pass;
	char *log_dir;
};

/* HIK_SDK device structures */
struct hik_dev {
	LONG userID;
	LONG rrHandle;
	NET_DVR_DEVICEINFO_V30 *deviceInfo;
	NET_DVR_CLIENTINFO *clientInfo;
};

/* Gstreamer related structures needed to build the pipeline */
struct gst_pipe {
	GstBus      *bus;
	GMainLoop   *loop;
	GstAppSrc   *appsrc;
	GstElement  *mpegpsdemux;
	GstElement  *h264parse;
	GstElement  *avdec_h264;
	GstElement  *videoconvert;
	GstAppSink  *appsink;
	GstPipeline *pipeline;
};

/* Conglomerate Structure - all of the above */
struct cam_info {
	struct hik_conf *hc;
	struct hik_dev  *hd;
	struct gst_pipe *gp;
	struct shm_info *si;
};

/*
 * Function Prototypes
 */

/* HikNetSDK video stream call-back function */
void CALLBACK HikDataCallBack( LONG, DWORD, BYTE *, DWORD, void * );

/* Pipeline bus call-back function */
static gboolean bus_callback( GstBus *, GstMessage *, gpointer * );

/* Handle PAD_ADDED signal emited from mpegpsdemux element */
static void on_pad_added( GstElement *, GstPad *, gpointer * );

/* Push buffer function used to ingress data into appsrc element */
GstFlowReturn push_buffer( BYTE *, DWORD, struct gst_pipe * );

/* Pull buffer function used to get raw frames out of appsink element */
GstFlowReturn get_buffer( GstAppSink *, gpointer *);

/* Start HikNet video stream */
bool hik_play( struct cam_info * );

/* Configure pipeline */
bool config_pipeline( struct cam_info * );

/* Initialization Functions */
struct cam_info * init_caminfo();
struct gst_pipe * init_pipeline();
struct hik_conf * init_hikconf(int argc, char *argv[]);
struct hik_dev * init_hikdev();

/* Clean-up Functions */
void main_cleanup( struct cam_info * );
void hikconf_cleanup( struct hik_conf * );
void gstpipe_cleanup( struct gst_pipe * );
void hikdev_cleanup( struct hik_dev * );
