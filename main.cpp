#include <vlc/vlc.h>

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#include <mutex>
#include <unistd.h>

using namespace cv;
using namespace std;

struct ctx
{
    Mat* image;
    std::mutex mutex;
    uchar* pixels;
};
bool isRunning = true;

Size getsize(std::string path)
{
    libvlc_instance_t *vlcInstance;
    libvlc_media_player_t *mp;
    libvlc_media_t *media;

    char const *vlc_argv[] =
        {
            "--no-audio", /* skip any audio track */
            "--no-xlib", /* tell VLC to not use Xlib */
        };
        int vlc_argc = sizeof(vlc_argv) / sizeof(*vlc_argv);

    vlcInstance = libvlc_new(vlc_argc, vlc_argv);

    media = libvlc_media_new_location(vlcInstance, path.c_str());
    mp = libvlc_media_player_new_from_media(media);

    libvlc_media_release(media);
    libvlc_video_set_callbacks(mp, NULL, NULL, NULL, NULL);
    libvlc_video_set_format(mp, "RV24", 100, 100, 100 * 24 / 8); // pitch = width * BitsPerPixel / 8
    //libvlc_video_set_format(mp, "RV32", 100, 100, 100 * 4);
    libvlc_media_player_play(mp);

    unsigned int width = 0, height = 0;

    for (int i = 0; i < 10; i++) {
        sleep(1);
        libvlc_video_get_size(mp, 0, &width, &height);

        if (width > 0 && height > 0)
            break;
    }


    if (width == 0 || height == 0) {
        width = 640;
        height = 480;
    }

    libvlc_media_player_stop(mp);
    libvlc_release(vlcInstance);
    libvlc_media_player_release(mp);
    return Size(width, height);
}


void *lock(void *data, void**p_pixels)
{
    struct ctx *ctx = (struct ctx*)data;
    ctx->mutex.lock();
    *p_pixels = ctx->pixels;
    return NULL;
}

void display(void *data, void *id){
    struct ctx *ctx = (struct ctx*)data;
    Mat frame = *ctx->image;
    if (frame.data)
    {
        imshow("frame", frame);
        waitKey(1);
    }
}

void unlock(void *data, void *id, void *const *p_pixels)
{
    struct ctx *ctx = (struct ctx*)data;
    ctx->mutex.unlock();
}


int main(int argc, char *argv[]) {
    string url = argv[1];
    //vlc sdk does not know the video size until it is rendered, so need to play it a bit so that size is     known
    Size sz = getsize(url);
    cout << sz.width << sz.height<< endl;

    // VLC pointers
    libvlc_instance_t *vlcInstance;
    libvlc_media_player_t *mp;
    libvlc_media_t *media;

    char const *vlc_argv[] = {
            "--no-audio",
            "--no-xlib",
        };
    int vlc_argc = sizeof(vlc_argv) / sizeof(*vlc_argv);

    vlcInstance = libvlc_new(vlc_argc, vlc_argv);

    media = libvlc_media_new_location(vlcInstance, url.c_str());
    mp = libvlc_media_player_new_from_media(media);
    libvlc_media_release(media);


    struct ctx* context = new ctx;
    context->image = new Mat(sz.height, sz.width, CV_8UC3);
    context->pixels = (unsigned char *)context->image->data;

    libvlc_video_set_callbacks(mp, lock, unlock, display, context);
    libvlc_video_set_format(mp, "RV24", sz.width, sz.height, sz.width * 3);

    libvlc_media_player_play(mp);
    while (isRunning) {
        sleep(1);
    }

    libvlc_media_player_stop(mp);
    libvlc_release(vlcInstance);
    libvlc_media_player_release(mp);

    return 0;
}
