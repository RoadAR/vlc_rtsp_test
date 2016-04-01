#include <iostream>
#include <opencv/cv.h>
#include <unistd.h>
#include <highgui.h>
#include <vlc/vlc.h>
#include <mutex>

using namespace std;
using namespace cv;

std::mutex muutex;

size_t videoBufferSize = 0;
uint8_t *videoBuffer = 0;

bool play = true;

void pf_video_prerender_callback(void* p_video_data, uint8_t** pp_pixel_buffer, size_t size) {
    muutex.lock();

    if (size > videoBufferSize || !videoBuffer) {
            if(videoBuffer) free(videoBuffer);
            videoBuffer = new uint8_t[size];
            videoBufferSize = size;
     }
    memset(videoBuffer, 0, videoBufferSize*sizeof(uint8_t));
    *pp_pixel_buffer = videoBuffer;
}

/**
 * image buffer full, unlock at end
 */
void pf_video_postrender_callback(void* p_video_data, uint8_t* p_pixel_buffer, int width, int height, int pixel_pitch, size_t size, libvlc_time_t pts ) {

    Mat img(Size(width, height), CV_8U, p_pixel_buffer);
    cout << width << " " << height << " " << size << endl;

    imshow("TEST", img);
    waitKey(1);
    muutex.unlock();
}

/**
 * lock and set the pointer to image buffer
 */

static void handleEvent(const libvlc_event_t* p_evt, void* p_user_data) {
    switch(p_evt->type)
    {
        case libvlc_MediaPlayerEncounteredError:
            cout << "LibVLC error received" << endl;
            break;

        case libvlc_MediaPlayerEndReached:
            play = false;
            break;
    }
}


int main(int argc, char *argv[])
{
    std::string path(argv[1]);

    Mat img_gray;

    // VLC options
    char smem_options[1000];

    sprintf(smem_options
            , "#transcode{vcodec=MJPG}:smem{"
            "video-prerender-callback=%lld,"
            "video-postrender-callback=%lld,"
            "video-data=%lld},"
            , (long long int)(intptr_t)(void*)&pf_video_prerender_callback
            , (long long int)(intptr_t)(void*)&pf_video_postrender_callback
            , (long long int)0);

    const char * const vlc_args[] = {
        "-I", "dummy", // Don't use any interface
        "--ignore-config", // Don't use VLC's config
        "--verbose=5", // Be verbose
        "--no-audio",
        "--sout", smem_options // Stream to memory
    };

    // We launch VLC
    auto vlc = libvlc_new(sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);


    // Media and Player
    libvlc_media_t *vlcm;
    bool isFile = (path.find("://") == std::string::npos);
    if (isFile) {
        vlcm = libvlc_media_new_path(vlc, path.c_str());
    } else {
        vlcm = libvlc_media_new_location(vlc, path.c_str());
    }
    auto vlcmp = libvlc_media_player_new_from_media(vlcm);
    libvlc_media_release (vlcm);


    libvlc_event_manager_t* eventManager = libvlc_media_player_event_manager(vlcmp);
    libvlc_event_attach(eventManager, libvlc_MediaPlayerEncounteredError, handleEvent, 0);
    libvlc_event_attach(eventManager, libvlc_MediaPlayerEndReached, handleEvent, 0);

    libvlc_media_player_play(vlcmp);

    while(play) {
        sleep(1000);
    }
    return 0;
}
