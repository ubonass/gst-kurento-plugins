/*
 * (C) Copyright 2014 Kurento (http://kurento.org/)
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include <glib.h>
#include "kmsuriendpointstate.h"

#include <kmstestutils.h>

#define IMG_PATH BINARY_LOCATION "/imgs/mario-wings.png"
#define VIDEO_PATH BINARY_LOCATION "/video/small.webm"

GMainLoop *loop;

static void
configure_structure (GstStructure * buttonsLayout)
{
  GstStructure *buttonsLayoutAux;
  int counter;

  //for (counter = 0; counter < 4; counter++) {
  for (counter = 0; counter < 2; counter++) {
    gchar *id;

    id = g_strdup_printf ("id%d", counter);
    buttonsLayoutAux = gst_structure_new (id,
        "upRightCornerX", G_TYPE_INT, 10 + counter,
        "upRightCornerY", G_TYPE_INT, 35 + counter,
        "width", G_TYPE_INT, 25 + counter,
        "height", G_TYPE_INT, 12 + counter,
        "id", G_TYPE_STRING, id,
        "inactive_uri", G_TYPE_STRING, IMG_PATH,
        "transparency", G_TYPE_DOUBLE, (double) 0.3, NULL);
    //"active_uri", G_TYPE_STRING, IMG_PATH, NULL);

    gst_structure_set (buttonsLayout,
        id, GST_TYPE_STRUCTURE, buttonsLayoutAux, NULL);

    gst_structure_free (buttonsLayoutAux);
    g_free (id);
  }
}

GST_START_TEST (set_properties)
{
  GstElement *pointerdetector;
  int nregions, scale;
  gboolean debug, message, show;
  GstStructure *buttonsLayout1, *buttonsLayout2;

  pointerdetector = gst_element_factory_make ("pointerdetector", NULL);

  nregions = 25;
  g_object_set (G_OBJECT (pointerdetector), "num-regions-eval", nregions, NULL);
  g_object_get (G_OBJECT (pointerdetector), "num-regions-eval", &nregions,
      NULL);

  if (nregions != 25)
    fail ("unexpected attribute value");

  scale = 50;
  g_object_set (G_OBJECT (pointerdetector), "scale-search-window", scale, NULL);
  g_object_get (G_OBJECT (pointerdetector), "scale-search-window", &scale,
      NULL);

  if (scale != 50)
    fail ("unexpected attribute value");

  debug = TRUE;
  g_object_set (G_OBJECT (pointerdetector), "show-debug-region", debug, NULL);
  g_object_get (G_OBJECT (pointerdetector), "show-debug-region", &debug, NULL);

  if (debug != TRUE)
    fail ("unexpected attribute value");

  buttonsLayout1 = gst_structure_new_empty ("windowsLayout1");
  configure_structure (buttonsLayout1);
  g_object_set (G_OBJECT (pointerdetector), "windows-layout", buttonsLayout1,
      NULL);
  gst_structure_free (buttonsLayout1);

  buttonsLayout2 = gst_structure_new_empty ("windowsLayout1");
  configure_structure (buttonsLayout2);
  g_object_set (G_OBJECT (pointerdetector), "windows-layout", buttonsLayout2,
      NULL);
  gst_structure_free (buttonsLayout2);

  message = FALSE;
  g_object_set (G_OBJECT (pointerdetector), "message", message, NULL);
  g_object_get (G_OBJECT (pointerdetector), "message", &message, NULL);

  if (message != FALSE)
    fail ("unexpected attribute value");

  show = FALSE;
  g_object_set (G_OBJECT (pointerdetector), "message", show, NULL);
  g_object_get (G_OBJECT (pointerdetector), "message", &show, NULL);

  if (show != FALSE)
    fail ("unexpected attribute value");

  g_object_unref (pointerdetector);
}

GST_END_TEST static void
bus_msg (GstBus * bus, GstMessage * msg, gpointer pipe)
{
  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_ERROR:{
      GST_ERROR ("Error: %" GST_PTR_FORMAT, msg);
      GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (pipe),
          GST_DEBUG_GRAPH_SHOW_ALL, "error");
      fail ("Error received on bus");
      break;
    }
    case GST_MESSAGE_EOS:{
      g_main_loop_quit (loop);
      break;
    }
    case GST_MESSAGE_WARNING:{
      GST_ERROR ("Warning: %" GST_PTR_FORMAT, msg);
      GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (pipe),
          GST_DEBUG_GRAPH_SHOW_ALL, "error");
      fail ("Warning received on bus");
      break;
    }
    default:
      break;
  }
}

GST_START_TEST (player_with_pointer)
{
  GstElement *player, *pipeline, *filter, *fakesink_audio, *fakesink_video;
  GstStructure *buttonsLayout;
  guint bus_watch_id;
  GstBus *bus;

  loop = g_main_loop_new (NULL, FALSE);
  pipeline = gst_pipeline_new ("pipeline_live_stream");
  g_object_set (G_OBJECT (pipeline), "async-handling", TRUE, NULL);
  player = gst_element_factory_make ("playerendpoint", NULL);
  filter = gst_element_factory_make ("pointerdetector", NULL);
  fakesink_audio = gst_element_factory_make ("fakesink", NULL);
  fakesink_video = gst_element_factory_make ("fakesink", NULL);
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));

  bus_watch_id = gst_bus_add_watch (bus, gst_bus_async_signal_func, NULL);
  g_signal_connect (bus, "message", G_CALLBACK (bus_msg), pipeline);
  g_object_unref (bus);

  g_object_set (G_OBJECT (player), "uri", VIDEO_PATH, NULL);

  buttonsLayout = gst_structure_new_empty ("windowsLayout");
  configure_structure (buttonsLayout);
  g_object_set (G_OBJECT (filter), "windows-layout", buttonsLayout, NULL);
  gst_structure_free (buttonsLayout);

  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  gst_bin_add (GST_BIN (pipeline), filter);
  gst_element_set_state (filter, GST_STATE_PLAYING);

  gst_bin_add (GST_BIN (pipeline), fakesink_audio);
  gst_element_set_state (fakesink_audio, GST_STATE_PLAYING);

  gst_bin_add (GST_BIN (pipeline), fakesink_video);
  gst_element_set_state (fakesink_video, GST_STATE_PLAYING);

  gst_bin_add (GST_BIN (pipeline), player);
  gst_element_set_state (player, GST_STATE_PLAYING);

  kms_element_link_pads (player, "audio_src_%u", fakesink_audio, "sink");
  kms_element_link_pads (player, "video_src_%u", filter, "sink");

  gst_element_link (filter, fakesink_video);

  /* Set player to start state */
  g_object_set (G_OBJECT (player), "state", KMS_URI_ENDPOINT_STATE_START, NULL);

  g_main_loop_run (loop);

  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (pipeline));
  g_source_remove (bus_watch_id);
  g_main_loop_unref (loop);
}

GST_END_TEST
/* Define test suite */
static Suite *
pointerdetector_suite (void)
{
  Suite *s = suite_create ("pointerdetector");
  TCase *tc_chain = tcase_create ("element");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, set_properties);
  tcase_add_test (tc_chain, player_with_pointer);

  return s;
}

GST_CHECK_MAIN (pointerdetector);
