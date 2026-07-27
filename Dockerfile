FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    pkg-config \
    ca-certificates \
    libgstreamer1.0-dev \
    libgstrtspserver-1.0-dev \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-ugly \
    gstreamer1.0-libav \
    libgstreamer1.0-0 \
    libgstrtspserver-1.0-0 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY Makefile .
COPY include/ include/
COPY src/ src/
COPY docker/entrypoint.sh /entrypoint.sh

RUN make && chmod +x /entrypoint.sh

EXPOSE 8554

ENTRYPOINT ["/entrypoint.sh"]
# Host file is mounted at /app/samples via docker-compose.
CMD ["--file", "/app/samples/15158346_3840_2160_60fps.mp4", "--port", "8554", "--mount", "/stream"]
