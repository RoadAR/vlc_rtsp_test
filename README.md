# Simple and working example, how to use libvlc and opencv throught vlc's vmem.

No smem, no magic, Instead of smem reads video is real time, so reading of 10 minutes
video file will take 10 minutes.

Works great with rtps, http and other supported by vlc protocols.

# Building
```
mkdir build
cd build
cmake ..
make
```

# Running

`./vlc_rtsp_test file:///home/user/video.avi` or `./vlc_rtsp_test rtsp://127.0.0.1:5554/video.avi`

