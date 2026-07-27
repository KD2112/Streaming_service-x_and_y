# Sample media

Current demo file:

```text
samples/15158346_3840_2160_60fps.mp4
```

The pipeline scales to 1280x720 before encoding for a lighter demo stream.

```bash
docker compose up --build
# or native:
make && ./build/rtsp_server --file samples/15158346_3840_2160_60fps.mp4
```

MP4 binaries are gitignored and not committed to the repo.
