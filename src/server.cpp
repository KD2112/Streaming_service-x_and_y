#include "server.hpp"

#include "pipeline.hpp"

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

#include <csignal>
#include <iostream>
#include <string>

namespace streaming {
namespace {

GMainLoop* g_loop = nullptr;

void on_signal(int /*signum*/) {
  if (g_loop != nullptr) {
    g_main_loop_quit(g_loop);
  }
}

gboolean on_bus_message(GstBus* /*bus*/, GstMessage* message,
                        gpointer user_data) {
  auto* media = static_cast<GstRTSPMedia*>(user_data);

  switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_EOS: {
      GstElement* element = gst_rtsp_media_get_element(media);
      if (element == nullptr) {
        break;
      }
      // Loop the file for a continuous demo stream.
      if (!gst_element_seek(
              element, 1.0, GST_FORMAT_TIME,
              static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH |
                                       GST_SEEK_FLAG_KEY_UNIT),
              GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
        g_warning("Seek to start failed; stream may end");
      }
      gst_object_unref(element);
      break;
    }
    case GST_MESSAGE_ERROR: {
      GError* err = nullptr;
      gchar* debug = nullptr;
      gst_message_parse_error(message, &err, &debug);
      g_printerr("Pipeline error: %s\n", err != nullptr ? err->message : "?");
      if (debug != nullptr) {
        g_printerr("Debug: %s\n", debug);
      }
      if (err != nullptr) {
        g_error_free(err);
      }
      g_free(debug);
      break;
    }
    default:
      break;
  }

  return TRUE;
}

void release_media(gpointer data, GClosure* /*closure*/) {
  g_object_unref(data);
}

void on_media_configure(GstRTSPMediaFactory* /*factory*/, GstRTSPMedia* media,
                        gpointer /*user_data*/) {
  GstElement* element = gst_rtsp_media_get_element(media);
  if (element == nullptr) {
    return;
  }

  GstBus* bus = gst_element_get_bus(element);
  gst_bus_add_signal_watch(bus);
  g_object_ref(media);
  g_signal_connect_data(bus, "message", G_CALLBACK(on_bus_message), media,
                        release_media, static_cast<GConnectFlags>(0));
  gst_object_unref(bus);
  gst_object_unref(element);
}

}  // namespace

int run_server(const Config& config) {
  std::cout << std::unitbuf;

  gst_init(nullptr, nullptr);

  g_loop = g_main_loop_new(nullptr, FALSE);

  std::signal(SIGINT, on_signal);
  std::signal(SIGTERM, on_signal);

  GstRTSPServer* server = gst_rtsp_server_new();
  gst_rtsp_server_set_service(server, std::to_string(config.port).c_str());

  GstRTSPMountPoints* mounts = gst_rtsp_server_get_mount_points(server);
  GstRTSPMediaFactory* factory = create_media_factory(config);

  g_signal_connect(factory, "media-configure", G_CALLBACK(on_media_configure),
                   nullptr);

  gst_rtsp_mount_points_add_factory(mounts, config.mount.c_str(), factory);
  g_object_unref(mounts);

  const guint server_id = gst_rtsp_server_attach(server, nullptr);
  if (server_id == 0) {
    std::cerr << "Failed to attach RTSP server (is port " << config.port
              << " in use?)\n";
    g_object_unref(server);
    g_main_loop_unref(g_loop);
    g_loop = nullptr;
    return 1;
  }

  std::cout << "RTSP server listening on port " << config.port << '\n'
            << "Stream URL: rtsp://127.0.0.1:" << config.port << config.mount
            << '\n'
            << "Source file: " << config.file << '\n'
            << "Pipeline: filesrc -> decodebin -> videoconvert -> videoscale -> "
               "x264enc -> h264parse -> rtph264pay (pay0)\n"
            << "Press Ctrl+C to stop.\n"
            << std::flush;

  g_main_loop_run(g_loop);

  g_source_remove(server_id);
  g_object_unref(server);
  g_main_loop_unref(g_loop);
  g_loop = nullptr;

  std::cout << "Server stopped.\n";
  return 0;
}

}  // namespace streaming
