#include "pipeline.hpp"

#include <gst/rtsp-server/rtsp-server.h>

namespace {

struct DecodeLinkContext {
  GstElement* convert = nullptr;
};

void on_decode_pad_added(GstElement* /*src*/, GstPad* new_pad,
                         gpointer user_data) {
  auto* ctx = static_cast<DecodeLinkContext*>(user_data);
  GstPad* sink_pad = gst_element_get_static_pad(ctx->convert, "sink");
  if (sink_pad == nullptr) {
    return;
  }

  if (gst_pad_is_linked(sink_pad)) {
    gst_object_unref(sink_pad);
    return;
  }

  GstCaps* caps = gst_pad_get_current_caps(new_pad);
  if (caps == nullptr) {
    caps = gst_pad_query_caps(new_pad, nullptr);
  }
  if (caps == nullptr || gst_caps_is_empty(caps)) {
    if (caps != nullptr) {
      gst_caps_unref(caps);
    }
    gst_object_unref(sink_pad);
    return;
  }

  const GstStructure* structure = gst_caps_get_structure(caps, 0);
  const gchar* name = gst_structure_get_name(structure);
  if (name == nullptr || !g_str_has_prefix(name, "video/")) {
    gst_caps_unref(caps);
    gst_object_unref(sink_pad);
    return;
  }
  gst_caps_unref(caps);

  const GstPadLinkReturn ret = gst_pad_link(new_pad, sink_pad);
  if (ret != GST_PAD_LINK_OK) {
    g_warning("Failed to link decodebin video pad: %s",
              gst_pad_link_get_name(ret));
  }
  gst_object_unref(sink_pad);
}

void free_decode_link_context(gpointer data) {
  delete static_cast<DecodeLinkContext*>(data);
}

GstElement* make_element(const char* factory_name, const char* name) {
  GstElement* element = gst_element_factory_make(factory_name, name);
  if (element == nullptr) {
    g_error("Failed to create element '%s' (is the plugin installed?)",
            factory_name);
  }
  return element;
}

}  // namespace

// GObject subclass at global scope so G_DEFINE_TYPE symbols are unmangled and
// visible to create_media_factory().
struct StreamingMediaFactory {
  GstRTSPMediaFactory parent;
  streaming::Config* config;
};

struct StreamingMediaFactoryClass {
  GstRTSPMediaFactoryClass parent_class;
};

G_DEFINE_TYPE(StreamingMediaFactory, streaming_media_factory,
              GST_TYPE_RTSP_MEDIA_FACTORY)

static GstElement* streaming_media_factory_create_element(
    GstRTSPMediaFactory* factory, const GstRTSPUrl* /*url*/) {
  auto* self = reinterpret_cast<StreamingMediaFactory*>(factory);
  return streaming::build_media_bin(*self->config);
}

static void streaming_media_factory_finalize(GObject* object) {
  auto* self = reinterpret_cast<StreamingMediaFactory*>(object);
  delete self->config;
  self->config = nullptr;
  G_OBJECT_CLASS(streaming_media_factory_parent_class)->finalize(object);
}

static void streaming_media_factory_class_init(
    StreamingMediaFactoryClass* klass) {
  auto* factory_class = GST_RTSP_MEDIA_FACTORY_CLASS(klass);
  factory_class->create_element = streaming_media_factory_create_element;

  auto* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = streaming_media_factory_finalize;
}

static void streaming_media_factory_init(StreamingMediaFactory* self) {
  self->config = nullptr;
}

namespace streaming {

GstElement* build_media_bin(const Config& config) {
  GstElement* bin = gst_bin_new("media-bin");

  GstElement* source = make_element("filesrc", "source");
  GstElement* decode = make_element("decodebin", "decode");
  GstElement* convert = make_element("videoconvert", "convert");
  GstElement* scale = make_element("videoscale", "scale");
  GstElement* raw_caps = make_element("capsfilter", "raw-caps");
  GstElement* encoder = make_element("x264enc", "encoder");
  GstElement* h264_caps = make_element("capsfilter", "h264-caps");
  GstElement* parse = make_element("h264parse", "parse");
  // RTSP factory requires a payloader named pay0.
  GstElement* pay = make_element("rtph264pay", "pay0");

  g_object_set(source, "location", config.file.c_str(), nullptr);

  GstCaps* raw = gst_caps_new_simple("video/x-raw", "width", G_TYPE_INT,
                                     config.width, "height", G_TYPE_INT,
                                     config.height, nullptr);
  g_object_set(raw_caps, "caps", raw, nullptr);
  gst_caps_unref(raw);

  // GstX264EncTune: zerolatency = 4; GstX264EncPreset: ultrafast = 1
  g_object_set(encoder, "tune", 4, "bitrate", config.bitrate_kbps,
               "key-int-max", 30, "speed-preset", 1, nullptr);

  GstCaps* h264 = gst_caps_new_simple("video/x-h264", "profile", G_TYPE_STRING,
                                      "baseline", nullptr);
  g_object_set(h264_caps, "caps", h264, nullptr);
  gst_caps_unref(h264);

  g_object_set(pay, "pt", 96, "config-interval", 1, nullptr);

  gst_bin_add_many(GST_BIN(bin), source, decode, convert, scale, raw_caps,
                   encoder, h264_caps, parse, pay, nullptr);

  if (!gst_element_link(source, decode)) {
    g_error("Failed to link filesrc -> decodebin");
  }

  // decodebin exposes pads dynamically (GStreamer basic tutorials pattern).
  auto* link_ctx = new DecodeLinkContext{convert};
  g_object_set_data_full(G_OBJECT(bin), "decode-link-context", link_ctx,
                         free_decode_link_context);
  g_signal_connect(decode, "pad-added", G_CALLBACK(on_decode_pad_added),
                   link_ctx);

  if (!gst_element_link_many(convert, scale, raw_caps, encoder, h264_caps, parse,
                             pay, nullptr)) {
    g_error("Failed to link convert -> ... -> rtph264pay");
  }

  return bin;
}

GstRTSPMediaFactory* create_media_factory(const Config& config) {
  auto* factory = static_cast<StreamingMediaFactory*>(
      g_object_new(streaming_media_factory_get_type(), nullptr));
  factory->config = new Config(config);

  gst_rtsp_media_factory_set_shared(GST_RTSP_MEDIA_FACTORY(factory), TRUE);
  gst_rtsp_media_factory_set_eos_shutdown(GST_RTSP_MEDIA_FACTORY(factory),
                                          FALSE);
  return GST_RTSP_MEDIA_FACTORY(factory);
}

}  // namespace streaming
