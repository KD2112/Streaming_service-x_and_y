#pragma once

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

#include "config.hpp"

namespace streaming {

// Build the media bin with gst_element_factory_make / link (pay0 = RTP H.264).
GstElement* build_media_bin(const Config& config);

// Custom GstRTSPMediaFactory that creates elements via the C API (no launch string).
GstRTSPMediaFactory* create_media_factory(const Config& config);

}  // namespace streaming
