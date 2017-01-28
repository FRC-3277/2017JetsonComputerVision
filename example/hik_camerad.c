#include "hik_camerad.h"


int main(int argc, char *argv[]) {

        struct cam_info *ci;

        /* Fire-up logging */
	elk = initELK(ELK_ADDR, ELK_PORT, ELK_TYPE, "CameraDev", true);
	if ( elk == NULL ) {
		fprintf(stderr, "Failed to initialize logging\n");
		return 1;
	}

        /* Initialize conglomerate camera struct */
        ci = init_caminfo();
        if ( ci == NULL ) {
		logELK(elk, FATAL, "Failed to allocate memory for cam_info struct");
                return 1;
        }

        /* Initialize HIK login parameters */
        ci->hc = init_hikconf(argc, argv);
        if ( ci->hc == NULL ) {
                logELK(elk, FATAL, "Failed initializing HIK config params");
                main_cleanup(ci);
                return 1;
        }

	/* Initialize shared memory */
	ci->si = create_shm(GSTREAM_SEM_NAME, GSTREAM_SHM_NAME, sizeof(struct gst_stream));
	if ( ci->si == NULL ) {
		logELK(elk, FATAL, "Failed to allocate POSIX shared memroy");
		main_cleanup(ci);
		return 1;
	}

        /* Initialize GStreamer framework */
        GError* vGErr = NULL;
        if ( !gst_init_check(&argc, &argv, &vGErr) ) {
		logELK(elk, FATAL, "Failed to initialize gstreamer framework");
		main_cleanup(ci);
                return 1;
        }

        /* Initialize HIK SDK */
        if ( !NET_DVR_Init() ) {
		logELK(elk, FATAL, "Failed to initialize Hik SDK");
		main_cleanup(ci);
                return 1;
        }

        /* Initialize gstreamer pipeline */
        ci->gp = init_pipeline();
        if ( ci->gp == NULL ) {
		logELK(elk, FATAL, "Failed initializing gstreamer pipeline");
                main_cleanup(ci);
                return 1;
        }

	/* Configure gstreamer pipeline */
	if ( !config_pipeline(ci) ) {
		logELK(elk, FATAL, "Failed configuring gstreamer pipeline");
		main_cleanup(ci);
		return 1;
	}

        /* Initialize HikSDK */
        ci->hd = init_hikdev();
        if ( ci->hd == NULL ) {
		logELK(elk, FATAL, "Failed to initialize HikSDK");
                main_cleanup(ci);
                return 1;
        }

        /* Start streaming */
        if ( hik_play(ci) ) {
                gst_element_set_state((GstElement*)ci->gp->pipeline, GST_STATE_PLAYING);

		/* Create pipeline graph for debugging */
		//GST_DEBUG_BIN_TO_DOT_FILE((GstBin*)ci->gp->pipeline, GST_DEBUG_GRAPH_SHOW_ALL, "pipeline");

                g_main_loop_run(ci->gp->loop);

                /* Stop Hik stream and logout */
                gst_element_set_state((GstElement*)ci->gp->pipeline, GST_STATE_NULL);
                if ( !NET_DVR_StopRealPlay(ci->hd->rrHandle) ) {
			logELK(elk, ERROR, "Failed to stop play");
                }

                if ( !NET_DVR_Logout_V30(ci->hd->userID) ) {
			logELK(elk, ERROR, "Failed to logout");
                }
        }

        /* Free structs before exiting */
        main_cleanup(ci);

        return 0;
}


bool hik_play( struct cam_info *ci ) {

        struct hik_dev  *hd = ci->hd;
        struct hik_conf *hc = ci->hc;
        struct gst_pipe *gp = ci->gp;

        /* Initialize logging */
        if ( !NET_DVR_SetLogToFile(3, hc->log_dir, true) ) {
                logELK(elk, ERROR, "HIk logging initialization failed: %d", NET_DVR_GetLastError());
                return false;
        }

        /* Login to camera */
        hd->userID = NET_DVR_Login_V30(hc->host, hc->port, hc->user, hc->pass, hd->deviceInfo);
        if ( hd->userID == -1 ) {
                logELK(elk, ERROR, "Failed logging into camera: %d", NET_DVR_GetLastError());
                return false;
        }

        /* Play Stream */
        hd->rrHandle = NET_DVR_RealPlay_V30(hd->userID, hd->clientInfo, HikDataCallBack, (void *)gp, 1);
        if ( hd->rrHandle < 0 ) {
                logELK(elk, ERROR, "Failed trying to play HikSDK stream: %d", NET_DVR_GetLastError());
                return false;
        }

        return true;
}


void CALLBACK HikDataCallBack(LONG rrHandle, DWORD dataType, BYTE *buff, DWORD size, void * userPtr) {

	struct gst_pipe *gp = (struct gst_pipe *)userPtr;

	GstFlowReturn ret;

	switch ( dataType ) {

                case NET_DVR_SYSHEAD:
			ret = push_buffer(buff, size, gp);
			if ( ret != GST_FLOW_OK ) {
				gst_app_src_end_of_stream(gp->appsrc);
				g_main_loop_quit(gp->loop);
			}
                        break;
                case NET_DVR_STREAMDATA:
			ret = push_buffer(buff, size, gp);
			if ( ret != GST_FLOW_OK ) {
				gst_app_src_end_of_stream(gp->appsrc);
				g_main_loop_quit(gp->loop);
			}
                        break;
                case NET_DVR_AUDIOSTREAMDATA:
                        logELK(elk, WARN, "Audio stream data");
                        break;
                case NET_DVR_PRIVATE_DATA:
                        logELK(elk, WARN, "Private stream data");
                        break;
                default:
                        logELK(elk, WARN, "Unknown data type: %d", dataType);
                        break;
        }
}


GstFlowReturn push_buffer( BYTE *buff, DWORD size, struct gst_pipe *gp ) {

        /* Allocate memory to store the stream packet */
        guint8 *ptr = (guint8*)g_malloc(size);
        if (  ptr == NULL ) {
                logELK(elk, ERROR, "Error allocatiing memory for stream data");
                return GST_FLOW_ERROR;
        }

        /* Copy stream packet from Hik -> gstreamer memory */
        memcpy(ptr, buff, size);

        /* Create a wrapped buffer object */
        GstBuffer *buffer = gst_buffer_new_wrapped(ptr, size);

        /* Push buffer (stream data) into appsrc element */
        return gst_app_src_push_buffer(gp->appsrc, buffer);
}


/* Bus callback handler */
static gboolean bus_callback( GstBus *bus, GstMessage *msg, gpointer *ptr ) {

	struct gst_pipe *gp = (struct gst_pipe *)ptr;

	GError* err;
	gchar* debug_info;
	const gchar *type;
 
	switch ( GST_MESSAGE_TYPE(msg) ) { 
		case GST_MESSAGE_ERROR:
			gst_message_parse_error (msg, &err, &debug_info);
			logELK(elk, ERROR, "Error received from element %s: %s", GST_OBJECT_NAME (msg->src), err->message);
			logELK(elk, ERROR, "Debugging information: %s", debug_info ? debug_info : "none");
			g_clear_error(&err); g_free(debug_info);
			gst_app_src_end_of_stream(gp->appsrc);
			g_main_loop_quit(gp->loop);
			break;
 
		case GST_MESSAGE_WARNING:
			gst_message_parse_warning(msg, &err, &debug_info);
			logELK(elk, WARN, "Warning received from element %s: %s", GST_OBJECT_NAME (msg->src), err->message);
			logELK(elk, WARN, "Debugging information: %s", debug_info ? debug_info : "none");
			g_clear_error(&err); g_free(debug_info);
			break;

		case GST_MESSAGE_INFO:
			gst_message_parse_info(msg, &err, &debug_info);
			logELK(elk, INFO, "Info received from element %s: %s", GST_OBJECT_NAME (msg->src), err->message);
			logELK(elk, INFO, "Debugging information: %s", debug_info ? debug_info : "none");
			g_clear_error(&err); g_free(debug_info);
			break;
 
		case GST_MESSAGE_EOS:
			logELK(elk, WARN, "End-Of-Stream (EOS) detected");
			gst_app_src_end_of_stream(gp->appsrc);
			g_main_loop_quit(gp->loop);
			break;
 
		default:
			type = gst_message_type_get_name(GST_MESSAGE_TYPE(msg));
			logELK(elk, WARN, "Unhandled message of type: %s caught on pipeline", type);
			break;
	}
 
	return true;
}

static void on_pad_added( GstElement *element, GstPad *pad, gpointer *ptr ) {

	struct gst_pipe *gp = (struct gst_pipe *)ptr;

	/* Get caps and struct objects for the passed in element */
	GstCaps *caps = gst_pad_query_caps(pad, NULL);
	GstStructure *str = gst_caps_get_structure(caps, 0);

	/* Get the name (type) of the element pad passed-in */
	gchar *name = (gchar*)gst_structure_get_name(str);

        /* Looking for pad with "video" caps */
        if ( !g_strrstr(name, "video") ) {
                logELK(elk, ERROR, "on_pad_added - advertised caps does not support video: %s", name);
		gst_caps_unref(caps);
		return;
        }

	/* Get static sink pad for h264parse */
	GstPad *h264parsesink = gst_element_get_static_pad(gp->h264parse, "sink");
	if ( h264parsesink == NULL ) {
		logELK(elk, ERROR, "Failed to get static pad for h264parse element");
		gst_app_src_end_of_stream(gp->appsrc);
		g_main_loop_quit(gp->loop);
		gst_caps_unref(caps);
		return;
	}

	/* Link-up the pads if not linked already */
	if ( !gst_pad_is_linked(h264parsesink) ) {
		if ( !GST_PAD_LINK_FAILED(gst_pad_link(pad, h264parsesink)) ) {
			logELK(elk, INFO, "Added pad: %s", name);
		} else {
			logELK(elk, INFO, "Pad linking failed");
			gst_app_src_end_of_stream(gp->appsrc);
			g_main_loop_quit(gp->loop);
		}
	}
	gst_object_unref(h264parsesink);
	gst_caps_unref(caps);
}


struct hik_dev * init_hikdev() {

	struct hik_dev *hd;

	/* Allocate memory for hik_dev struct */
	hd = (struct hik_dev *)malloc(sizeof(struct hik_dev));
	if ( hd == NULL ) {
		logELK(elk, ERROR, "Failed to allocate memory for hik_dev struct");
		return NULL;
	}

	/* Null set memory allocation */
	memset(hd, '\0', sizeof(struct hik_dev));

	/* Allocate memory for structs */
	hd->clientInfo = (NET_DVR_CLIENTINFO *)malloc(sizeof(NET_DVR_CLIENTINFO));
	if ( hd->clientInfo == NULL ) {
		logELK(elk, ERROR, "Failed to allocate memory for NET_DVR_CLIENTINFO struct");
		hikdev_cleanup(hd);
		return NULL;
	}

	hd->deviceInfo = (NET_DVR_DEVICEINFO_V30 *)malloc(sizeof(NET_DVR_DEVICEINFO_V30));
	if ( hd->deviceInfo == NULL ) {
		logELK(elk, ERROR, "Failed to allocate memory for NET_DVR_DEVICEINFO struct");
		hikdev_cleanup(hd);
		return NULL;
	}

	/* Initialize structs */
	memset(hd->clientInfo, '\0', sizeof(NET_DVR_CLIENTINFO));
	memset(hd->deviceInfo, '\0', sizeof(NET_DVR_DEVICEINFO));

	hd->clientInfo->hPlayWnd         = 0;
	hd->clientInfo->lChannel         = 1;
	hd->clientInfo->lLinkMode        = 0;
	hd->clientInfo->sMultiCastIP     = NULL;

	return hd;
}


struct gst_pipe * init_pipeline() {

	struct gst_pipe *gp;

	/* Allocate memory for conglomerate camera struct */
	gp = (struct gst_pipe *)malloc(sizeof(struct gst_pipe));
	if ( gp == NULL ) {
		logELK(elk, ERROR, "Failed to allocate memory for gst_pipe struct");
		return NULL;
	}

        /* Null set memory allocation */
        memset(gp, '\0', sizeof(struct gst_pipe));

        /* Create empty pipeline */
        gp->pipeline = (GstPipeline*)gst_pipeline_new("mypipeline");

        /* Create pipline elements */
        gp->appsrc = (GstAppSrc*)gst_element_factory_make("appsrc", "myappsrc");
        if ( !gp->appsrc ) {
                logELK(elk, ERROR, "appsrc element could not be created");
                gstpipe_cleanup(gp);
		return NULL;
        }
        gp->mpegpsdemux = gst_element_factory_make("mpegpsdemux", "mympegpsdemux");
        if ( !gp->mpegpsdemux ) {
                logELK(elk, ERROR, "mpegpsdemux element could not be created");
		gstpipe_cleanup(gp);
                return NULL;
        }
        gp->h264parse = gst_element_factory_make("h264parse", "myh264parse");
        if ( !gp->h264parse ) {
                logELK(elk, ERROR, "h264parse element could not be created");
		gstpipe_cleanup(gp);
                return NULL;
        }
        gp->avdec_h264 = gst_element_factory_make("avdec_h264", "myavdec_h264");
        if ( !gp->avdec_h264 ) {
                logELK(elk, ERROR, "avdec_h264 element could not be created");
		gstpipe_cleanup(gp);
                return NULL;
        }
        gp->videoconvert = gst_element_factory_make("videoconvert", "myvideoconvert");
        if ( !gp->videoconvert ) {
                logELK(elk, ERROR, "videoconvert element could not be created");
		gstpipe_cleanup(gp);
                return NULL;
        }
        gp->appsink = (GstAppSink*)gst_element_factory_make("appsink", "myappsink");
        if ( !gp->appsink ) {
                logELK(elk, ERROR, "appsink element could not be created");
		gstpipe_cleanup(gp);
                return NULL;
        }

        /* Build the pipeline */
        if ( !gst_bin_add(GST_BIN(gp->pipeline), (GstElement*)gp->appsrc) ) {
                logELK(elk, ERROR, "failed to add appsrc to pipeline");
		gstpipe_cleanup(gp);
                return NULL;
        }
        if ( !gst_bin_add(GST_BIN(gp->pipeline), gp->mpegpsdemux) ) {
                logELK(elk, ERROR, "failed to add  mpegpsdemux to pipeline");
		gstpipe_cleanup(gp);
                return NULL;
        }
        if ( !gst_bin_add(GST_BIN(gp->pipeline), gp->h264parse) ) {
                logELK(elk, ERROR, "failed to add h264parse to pipeline");
		gstpipe_cleanup(gp);
                return NULL;
        }
        if ( !gst_bin_add(GST_BIN(gp->pipeline), gp->avdec_h264) ) {
                logELK(elk, ERROR, "failed to add avdec_h264 to pipeline\n");
		gstpipe_cleanup(gp);
                return NULL;
        }
        if ( !gst_bin_add(GST_BIN(gp->pipeline), gp->videoconvert) ) {
                logELK(elk, ERROR, "failed to add videoconvert to pipeline");
		gstpipe_cleanup(gp);
                return NULL;
        }
        if ( !gst_bin_add(GST_BIN(gp->pipeline), (GstElement*)gp->appsink) ) {
                logELK(elk, ERROR, "failed to add appsink to pipeline");
		gstpipe_cleanup(gp);
                return NULL;
        }

        /* Link the elements - NOTE: mpegpsdemux->h264parse linking is done at runtime! */
        if ( !gst_element_link((GstElement*)gp->appsrc, gp->mpegpsdemux) ) {
                logELK(elk, ERROR, "failed to link src and mpegpsdemux");
		gstpipe_cleanup(gp);
                return NULL;
        }
        if ( !gst_element_link(gp->h264parse, gp->avdec_h264) ) {
                logELK(elk, ERROR, "failed to link h264parse and avdec_h264");
		gstpipe_cleanup(gp);
                return NULL;
        }
        if ( !gst_element_link(gp->avdec_h264, gp->videoconvert) ) {
                logELK(elk, ERROR, "failed to link avdec_h264 and videoconvert");
		gstpipe_cleanup(gp);
                return NULL;
        }

	/* Configure caps filter to force BGR24*/
	GstCaps* caps = gst_caps_from_string("video/x-raw,width=1280,height=720,depth=24,format=BGR");
	if ( caps == NULL ) {
        	logELK(elk, ERROR, "Failed to create caps from string, setting BGR formatn");
        	gstpipe_cleanup(gp);
        	return NULL;
	}
	if ( !gst_element_link_filtered(gp->videoconvert, (GstElement*)gp->appsink, caps) ){
		logELK(elk, ERROR, "failed to link videoconvert and appsink");
		gstpipe_cleanup(gp);
		return NULL;
	}
	gst_caps_unref(caps);

        /* Create new gstreamer loop */
        gp->loop = g_main_loop_new(NULL, FALSE);

	return gp;
}

bool config_pipeline( struct cam_info * ci ) {

	struct gst_pipe *gp = ci->gp;
	struct shm_info *si = ci->si;

        /* Configure misc. appsrc settings */
        gst_app_src_set_max_bytes(gp->appsrc, 800000);
        g_object_set(G_OBJECT(gp->appsrc), "block", TRUE, NULL);
        g_object_set(G_OBJECT(gp->appsrc), "do-timestamp", TRUE, NULL);

        /* Configure misc. appsink settings */
        gst_app_sink_set_emit_signals((GstAppSink*)gp->appsink, true);
        gst_app_sink_set_drop((GstAppSink*)gp->appsink, true);
        gst_app_sink_set_max_buffers((GstAppSink*)gp->appsink, 1);


        /* Get pipeline bus */
        gp->bus = gst_pipeline_get_bus(gp->pipeline);
        if ( !gst_bus_add_watch(gp->bus, (GstBusFunc)bus_callback, gp) ) {
                logELK(elk, ERROR, "Failed to add bus watch to pipeline");
                return false;
        }

        /* Register call-back handler for pad-added signal from mpegpsdemux */
        if ( !g_signal_connect(gp->mpegpsdemux, "pad-added", G_CALLBACK(on_pad_added), gp) ) {
                logELK(elk, ERROR, "Failed to register pad-added callback handler for mpegpsdemux");
                return false;
        }

        /* Register call-back handler for new-sample signal from appsink element */
        if ( !g_signal_connect(gp->appsink, "new-sample", G_CALLBACK(get_buffer), si) ) {
                logELK(elk, ERROR, "Failed to register new-buffer callback handler for appsink");
                return false;
        }

	return true;
}


GstFlowReturn get_buffer(GstAppSink *appsink, gpointer *ptr) {

	GstSample *sample;
	GstBuffer *buffer;
	GstMapInfo info;

	struct shm_info *si = (struct shm_info *)ptr;
	struct gst_stream *gs = (struct gst_stream *)si->shm_ptr;

	/* Pull sample from appsink */
	sample = gst_app_sink_pull_sample(appsink);
	if ( sample == NULL ) {
		logELK(elk, ERROR, "Failed to pull sample from appsink");
		return GST_FLOW_ERROR;
	}

	/* Pull buffer data */
	buffer = gst_sample_get_buffer(sample);
	if ( buffer == NULL ) {
		logELK(elk, ERROR, "No buffer found in sample");
		gst_sample_unref(sample);
		return GST_FLOW_ERROR;
	}

	/* Map buffer into info struct */
	if ( !gst_buffer_map(buffer, &info, GST_MAP_READ) ) {
		logELK(elk, ERROR, "Failed to map buffer info");
		gst_sample_unref(sample);
		return GST_FLOW_ERROR;
	}

	/* Grow shm segment if needed */
	if ( info.size != GSTREAM_BUFF_SIZE ) {
		logELK(elk, ERROR, "Buffer size does not match shm size: %lu", info.size);
		return GST_FLOW_ERROR;
	}

	/* Get lock and copy date into shared memory */
	sem_wait(si->sem);
	memcpy(&(gs->buff), info.data, info.size);
	gs->x = 1280;
	gs->y = 720;
	gs->frame++;
	sem_post(si->sem);

	/* Cleanup resources before exiting */
	gst_buffer_unmap(buffer, &info);
	gst_sample_unref(sample);

	return GST_FLOW_OK;
}


struct hik_conf * init_hikconf(int argc, char *argv[]) {

	struct hik_conf *hc;
	int gopt;

	/* Allocate memory for conglomerate camera struct */
	hc = (struct hik_conf *)malloc(sizeof(struct hik_conf));
	if ( hc == NULL ) {
		logELK(elk, ERROR, "Failed to allocate memory for hik_conf struct");
		return NULL;
	}

	/* Null set memory allocation */
	memset(hc, '\0', sizeof(struct hik_conf));

	while ( (gopt = getopt(argc, argv, "h:p:u:x:l:")) != EOF ) {

		switch (gopt) {
			case 'h':
				hc->host = (char *)malloc(strlen(optarg) + 1);
				strncpy(hc->host, optarg, strlen(optarg) + 1);
				break;
			case 'u':
				hc->user = (char *)malloc(strlen(optarg) + 1);
				strncpy(hc->user, optarg, strlen(optarg) + 1);
				break;
			case 'x':
				hc->pass = (char *)malloc(strlen(optarg) + 1);
				strncpy(hc->pass, optarg, strlen(optarg) + 1);
				break;
			case 'l':
				hc->log_dir = (char *)malloc(strlen(optarg) + 1);
				strncpy(hc->log_dir, optarg, strlen(optarg) + 1);
				break;
			case 'p':
				hc->port = atoi(optarg);
				break;
			default:
				logELK(elk, ERROR, "Unknown argument(s)");
		}
	}

	/* Sanity check passed-in arguments */
	if ( hc->host == NULL ) {
		logELK(elk, ERROR, "camera host (-h) option was not specified");
		free(hc);
		return NULL;
	}
	if ( hc->user == NULL ) {
		logELK(elk, ERROR, "camera user (-u) option was not specified");
		free(hc);
		return NULL;
	}
	if ( hc->pass == NULL ) {
		logELK(elk, ERROR, "camera password (-x) option was not specified");
		free(hc);
		return NULL;
	}
	if ( hc->log_dir == NULL ) {
		logELK(elk, ERROR, "camera log dir (-l) option was not specified");
		free(hc);
		return NULL;
	}
	if ( hc->port <= 0 ) {
		logELK(elk, ERROR, "camera port (-p) option was not specified");
		free(hc);
		return NULL;
	}

	return hc;
}


struct cam_info * init_caminfo() {

	struct cam_info *ci;

        /* Allocate memory for conglomerate camera struct */
        ci = (struct cam_info *)malloc(sizeof(struct cam_info));
        if ( ci == NULL ) {
                logELK(elk, ERROR, "Failed to allocate memory for cam_info struct");
                return NULL;
        }

        /* Null set memory allocation */
        memset(ci, '\0', sizeof(struct cam_info));

	return ci;
}


void main_cleanup( struct cam_info *ci ) {

	/* Sanity check */
	if ( ci == NULL ) {
		return;
	}

	/* Clean-up shared memory objects */
	destroy_shm(ci->si);

	/* Clean-up sub-structures */
	hikdev_cleanup(ci->hd);
	gstpipe_cleanup(ci->gp);
	hikconf_cleanup(ci->hc);

	/* HikSDK clean-up function */
	NET_DVR_Cleanup();

        /* Cleanup ELK logging */
        cleanupELK(elk);

	free(ci);
}


void hikconf_cleanup( struct hik_conf *hc ) {

        if ( hc == NULL ) {
                return;
        }

        free(hc);
        hc = NULL;
}


void hikdev_cleanup( struct hik_dev *hd ) {

        /* Return early if NULL */
        if ( hd == NULL ) {
                return;
        }

        if ( hd->clientInfo != NULL ) {
                free(hd->clientInfo);
        }

        if ( hd->deviceInfo != NULL ) {
                free(hd->deviceInfo);
        }

        free(hd);
        hd = NULL;
}


void gstpipe_cleanup( struct gst_pipe *gp ) {

        /* Exit early if struct is NULL */
        if ( gp == NULL ) {
                return;
        }

        /* Remove bus watch */
        gst_bus_remove_watch(gp->bus);
	gst_object_unref(GST_OBJECT(gp->bus));

        /* Unref pipeline */
        if ( gp->pipeline != NULL ) {
                gst_object_unref(GST_OBJECT(gp->pipeline));
        }

        /* Unref the loop object */
        g_main_loop_unref(gp->loop);

        /* Free gst_pipe structure */
        free(gp);
        gp = NULL;
}
