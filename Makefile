CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
PKG      := gstreamer-1.0 gstreamer-rtsp-server-1.0
PKG_CONFIG ?= pkg-config

SRC_DIR   := src
BUILD_DIR := build
TARGET    := $(BUILD_DIR)/rtsp_server

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

.PHONY: all clean run check-deps

all: check-deps $(TARGET)

check-deps:
	@ $(PKG_CONFIG) --exists $(PKG) || ( \
	  echo "Missing GStreamer. Install pkg-config, libgstreamer1.0-dev, libgstrtspserver-1.0-dev (see README)"; \
	  exit 1)

PKG_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(PKG) 2>/dev/null)
PKG_LIBS   := $(shell $(PKG_CONFIG) --libs $(PKG) 2>/dev/null)

CXXFLAGS += $(PKG_CFLAGS) -Iinclude
LDFLAGS  += $(PKG_LIBS)

$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: all
	./$(TARGET) --file samples/15158346_3840_2160_60fps.mp4

clean:
	rm -rf $(BUILD_DIR)
