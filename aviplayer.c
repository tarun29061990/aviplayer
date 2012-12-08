#include<gst/gst.h>
#include<glib.h>


static void on_pad_added (GstElement *element,GstPad *pad,gpointer data)
{
	GstPad *sinkpad;
	GstElement *s = (GstElement *) data;
	/* We can now link this pad with the vorbis-decoder sink pad */
	g_print ("Dynamic pad created, linking decoder/sink\n");
	sinkpad = gst_element_get_static_pad (s, "sink");
	gst_pad_link (pad, sinkpad);
	gst_object_unref (sinkpad);

}


static gboolean my_bus_callback (GstBus *bus,GstMessage *message,gpointer data)
{
	GMainLoop *loop = (GMainLoop *) data;
	g_print ("Got %s message\n", GST_MESSAGE_TYPE_NAME (message));
	switch (GST_MESSAGE_TYPE (message)) 
	{
		case GST_MESSAGE_ERROR: 
		{
			GError *err;
			gchar *debug;
			gst_message_parse_error (message, &err, &debug);
			g_print ("Error: %s\n", err->message);
			g_error_free (err);
			g_free (debug);
			g_main_loop_quit (loop);
			break;
		}	
		case GST_MESSAGE_EOS:
		/* end-of-stream */
		g_main_loop_quit (loop);
		break;
		
		default:
		/* unhandled message */
		break;
	}
/* we want to be notified again the next time there is a message
* on the bus, so returning TRUE (FALSE means we want to stop watching
* for messages on the bus and our callback should not be called again)
*/
return TRUE;
}

static gboolean cb_print_position (GstElement *pipeline)
{
	GstFormat fmt = GST_FORMAT_TIME;
	gint64 pos, len;
	/*gboolean b;
	b=gst_element_query_duration (pipeline, &fmt, &len);
	g_print("%d",b);*/
	if (gst_element_query_position (pipeline, &fmt, &pos) && gst_element_query_duration (pipeline, &fmt, &len)) 
	{
		g_print ("Time: %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r",
		GST_TIME_ARGS (pos), GST_TIME_ARGS (len));
	}
	/* call me again */
	return TRUE;
}







int main(int argc, char *argv[])
{
	GstElement *pipeline,*source,*demux,*buffer1,*buffer2,*mp3dec,*sink,*decodebin,*autovideosink;
	GstBus *bus;
	GMainLoop *loop;
	
	gst_init(&argc,&argv);
	loop = g_main_loop_new (NULL, FALSE);

	//create elements
	pipeline=gst_pipeline_new("AVI PLAYER");
	source=gst_element_factory_make("filesrc","source");
	demux=gst_element_factory_make("avidemux","demux");
	buffer1=gst_element_factory_make("queue","buffer1");
	buffer2=gst_element_factory_make("queue","buffer2");
	mp3dec=gst_element_factory_make("flump3dec","mp3dec");
	sink=gst_element_factory_make("alsasink","sink");
	decodebin=gst_element_factory_make("decodebin","decodebin");
	autovideosink=gst_element_factory_make("autovideosink","autovideosink");

	g_object_set(source,"location",argv[1],NULL);

	//add message handler	
	bus = gst_pipeline_get_bus(GST_PIPELINE (pipeline));
	gst_bus_add_watch(bus, my_bus_callback, loop);
	gst_object_unref(bus);
	
	gst_bin_add_many (GST_BIN (pipeline), source, demux, buffer1, buffer2, mp3dec, sink, decodebin, autovideosink, NULL);

	//linking

	gst_element_link(source,demux);
	gst_element_link_many(buffer1,mp3dec,sink,NULL);//linked audio elements
	g_signal_connect(demux, "pad-added", G_CALLBACK (on_pad_added), buffer1);
	//linking video elements
	
	gst_element_link(buffer2,decodebin);
	g_signal_connect (decodebin, "pad-added", G_CALLBACK (on_pad_added), autovideosink);

	//linking demuxer to buffers
	
	g_signal_connect(demux, "pad-added", G_CALLBACK (on_pad_added), buffer2);

	//starting playback
	g_print("playing file %s \n", argv[1]);
	gst_element_set_state (pipeline, GST_STATE_PLAYING);
	
	//running the main loop
	g_print("running");
	g_timeout_add (100, (GSourceFunc) cb_print_position, pipeline);
	g_main_loop_run(loop);

	//stopping playback
	g_print("Freeing Pipeline\n");
	gst_element_set_state(pipeline, GST_STATE_NULL);
	g_print ("Deleting pipeline\n");	
	gst_object_unref(pipeline);
	g_main_loop_unref(loop);

	
	
	return 0;
		
}

