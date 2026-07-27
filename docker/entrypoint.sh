#!/bin/sh
set -eu

MEDIA_FILE="${MEDIA_FILE:-/app/samples/15158346_3840_2160_60fps.mp4}"

has_file=0
for arg in "$@"; do
  if [ "$arg" = "--file" ] || [ "$arg" = "-f" ]; then
    has_file=1
    break
  fi
done

if [ "$has_file" -eq 0 ]; then
  set -- --file "$MEDIA_FILE" "$@"
fi

# Resolve the path after --file / -f for a clear error before GStreamer starts.
file_path=""
prev=""
for arg in "$@"; do
  if [ "$prev" = "--file" ] || [ "$prev" = "-f" ]; then
    file_path="$arg"
    break
  fi
  prev="$arg"
done

if [ -n "$file_path" ] && [ ! -f "$file_path" ]; then
  echo "Media file not found: $file_path" >&2
  echo "Place an MP4 under samples/ on the host (mounted to /app/samples)." >&2
  exit 1
fi

exec /app/build/rtsp_server "$@"
