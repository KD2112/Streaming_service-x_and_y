# Streaming_service-x_and_y

Demo GStreamer RTSP streaming pipeline for XandY.

Phase 1 streams a sample MP4 over RTSP with a **shared encode** (one pipeline, many clients). A WebRTC browser gateway is planned later — see [docs/streaming-architecture.md](docs/streaming-architecture.md).

## Quick start (Docker — no host GStreamer deps)

1. Sample media: `samples/15158346_3840_2160_60fps.mp4` (mounted into the container).
2. Build runs `make` inside the image; compose mounts `./samples` into the container.

```bash
docker compose up --build
```

Stream URL (from the host):

```text
rtsp://127.0.0.1:8554/stream
```

Play (TCP interleaved — simplest with Docker port mapping):

```bash
ffplay -rtsp_transport tcp rtsp://127.0.0.1:8554/stream
```

Or open the same URL in VLC (prefer TCP/RTSP if available).

## Layout

```text
Dockerfile           build + runtime image (apt deps + make)
docker-compose.yml   publish port 8554
docker/entrypoint.sh pick media file, start rtsp_server
include/             public headers
src/                 C++ sources
samples/             optional host MP4 override
build/               local Makefile output (gitignored)
docs/                architecture notes
```

## Native build (optional)

Only needed if you want to compile on the host instead of Docker.

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  pkg-config \
  libgstreamer1.0-dev \
  libgstrtspserver-1.0-dev \
  gstreamer1.0-plugins-base \
  gstreamer1.0-plugins-good \
  gstreamer1.0-plugins-bad \
  gstreamer1.0-plugins-ugly \
  gstreamer1.0-libav \
  ffmpeg

make
./build/rtsp_server --file samples/15158346_3840_2160_60fps.mp4
```

## CLI options

| Flag | Default | Meaning |
|------|---------|---------|
| `--file PATH` | `samples/15158346_3840_2160_60fps.mp4` | Input MP4 |
| `--port PORT` | `8554` | RTSP listen port |
| `--mount PATH` | `/stream` | Mount point (must start with `/`) |

## Clean

```bash
make clean                 # native
docker compose down --rmi local
```
