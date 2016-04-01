//
//  video_stream.cpp
//  test_libvlc
//
//  Created by Alexander on 01.03.16.
//  Copyright © 2016 RoadAR. All rights reserved.
//

#include <mutex>
#include <condition_variable>
#include <iostream>
#include "video_vlc_stream.hpp"


using namespace std;
using namespace cv;

class Frame {
public:
    bool isUsing = false;

    int64_t time;
    unsigned long buffersize;
    uint8_t* buffer;
    int frame = 0;
    int width = 0, height = 0;

    bool imgAvaiable = false;

    Frame() {
        buffersize = 0;
        buffer = (uint8_t*)malloc(sizeof(uint8_t) * 1);
    }

    ~Frame() {
        free(buffer);
    }
};

class VideoLibVLCStream_impl {
public:
    // в локальном видео мы хотим обрабатывать все кадры
    VideoLibVLCStream_impl(std::string path);
    void initalize(string path);
    void reinitalize();

    int lockFrameForProcess(bool imgAvaiable);
    bool getNext(cv::Mat &mat, int &frameIdx);
    void log();
    ~VideoLibVLCStream_impl();

    string mediaPath;
    int reinitalizeCount;

    libvlc_instance_t *vlc;
    libvlc_media_player_t *vlcmp;
    bool finished;

    condition_variable newFrameAvaiableCV;
    mutex framesMutex;
    vector<Frame*> frames;
    int readingFrameIdx = -1;
    int processingFrameIdx = -1;
    int frameNum = 0;
};

/**
 * lock and set the pointer to image buffer
 */
void pf_video_prerender_callback(void* p_video_data, uint8_t** pp_pixel_buffer, size_t size) {

    VideoLibVLCStream_impl *stream = (VideoLibVLCStream_impl*) p_video_data;
    vector<Frame*> &frames = stream->frames;


    int lockIdx = -1;
    while (lockIdx < 0) {
        lockIdx = stream->lockFrameForProcess(false);
    }
    stream->readingFrameIdx = lockIdx;

    Frame *fr = frames[lockIdx];
    if (size > fr->buffersize){
        fr->buffer = (uint8_t*)realloc(fr->buffer,size);
        fr->buffersize = size;
    }

    *pp_pixel_buffer = fr->buffer;
}

/**
 * image buffer full, unlock at end
 */
void pf_video_postrender_callback(void* p_video_data, uint8_t* p_pixel_buffer, int width, int height, int pixel_pitch, size_t size, libvlc_time_t pts ) {

    VideoLibVLCStream_impl *stream = (VideoLibVLCStream_impl*) p_video_data;

    Frame *fr = stream->frames[stream->readingFrameIdx];

    fr->time = pts;
    fr->width = width;
    fr->height = height;
    fr->imgAvaiable = true;
    stream->frameNum++;
    fr->frame = stream->frameNum;

    stream->framesMutex.lock();
    fr->isUsing = false;
    stream->framesMutex.unlock();

    stream->newFrameAvaiableCV.notify_one();

    stream->log();
}

static void handleEvent(const libvlc_event_t* p_evt, void* p_user_data) {
    VideoLibVLCStream_impl *stream = (VideoLibVLCStream_impl*) p_user_data;
    switch(p_evt->type)
    {
        case libvlc_MediaPlayerEncounteredError:
            cout << "LibVLC error received" << endl;
            if (stream->reinitalizeCount < 10) {
                stream->reinitalize();
            } else {
                stream->finished = true;
                // чтобы пробудить получение фрейма
                stream->newFrameAvaiableCV.notify_one();
            }
            break;

        case libvlc_MediaPlayerEndReached:
            cout << "LibVLC finish received" << endl;
            stream->finished = true; // чтобы пробудеть получение фрейма
            stream->newFrameAvaiableCV.notify_one();
            break;
    }
}

VideoLibVLCStream_impl::VideoLibVLCStream_impl(string path) {
    reinitalizeCount = 0;
    for (int i = 0; i < 30; i++) {
        frames.push_back(new Frame());
    }
    finished = false;

    initalize(path);
}

void VideoLibVLCStream_impl::initalize(string path) {

    mediaPath = path;

    // VLC options
    char smem_options[1000];

    sprintf(smem_options
            , "#transcode{vcodec=MJPG}:smem{"
            "video-prerender-callback=%lld,"
            "video-postrender-callback=%lld,"
            "video-data=%lld},"
            , (long long int)(intptr_t)(void*)&pf_video_prerender_callback
            , (long long int)(intptr_t)(void*)&pf_video_postrender_callback
            , (long long int)this); //This would normally be useful data, 100 is just test data

    const char * const vlc_args[] = {
        "-I", "dummy", // Don't use any interface
        "--ignore-config", // Don't use VLC's config
        "--verbose=0", // Be verbose
        "--no-audio",
        "--sout", smem_options // Stream to memory
    };

    // We launch VLC
    vlc = libvlc_new(sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);


    // Media and Player
    libvlc_media_t *vlcm;
    bool isFile = (path.find("://") == std::string::npos);
    if (isFile) {
        vlcm = libvlc_media_new_path(vlc, path.c_str());
    } else {
        vlcm = libvlc_media_new_location(vlc, path.c_str());
    }
    vlcmp = libvlc_media_player_new_from_media(vlcm);
    libvlc_media_release (vlcm);


    finished = false;
    libvlc_event_manager_t* eventManager = libvlc_media_player_event_manager(vlcmp);
    libvlc_event_attach(eventManager, libvlc_MediaPlayerEncounteredError, handleEvent, this);
    libvlc_event_attach(eventManager, libvlc_MediaPlayerEndReached, handleEvent, this);

    libvlc_media_player_play(vlcmp);
}

void VideoLibVLCStream_impl::log() {
    cout << "log> ";
    vector<int> packetOrder;
    vector<int64_t> packetOrderVal;
    int idx = 0;
    for (auto fr : frames) {
        if (fr->imgAvaiable) {
            int64_t val = fr->time;
            int i = 0;
            for (i = 0; i < packetOrderVal.size(); i++) {
                if (val > packetOrderVal[i]) {
                    break;
                }
            }
            packetOrder.insert(packetOrder.begin() + i, idx);
            packetOrderVal.insert(packetOrderVal.begin() + i, val);
        }
        idx++;
    }

    idx = 0;
    for (auto fr : frames) {
        if (fr->imgAvaiable) {
            for (int i = 0; i < packetOrder.size(); i++) {
                if (packetOrder[i] == idx) {
                    cout << i;
                    break;
                }
            }
        } else {
            cout << "ø";
        }
        cout << " ";
        idx++;
    }
    cout << endl;
//    cout << "log> ";
//    for (auto fr : frames) {
//        cout << (fr->isUsing ? "u " : "_ ");
//    }
//    cout << endl;
//    cout << "log> ";
//    for (auto fr : frames) {
//        cout << fr->frame << " ";
//    }
//    cout << endl;
}

// если чтото поломалось, перезапускаем плеер
void VideoLibVLCStream_impl::reinitalize() {
    reinitalizeCount++;
    cout << "LibVLC streaming is broken, reinitalizing" << endl;
    if (vlcmp) {
        libvlc_media_player_stop(vlcmp);
        libvlc_media_player_release(vlcmp);
    }
    libvlc_release(vlc);

    initalize(mediaPath);
}

// Изображение подготовленное для обработки
int VideoLibVLCStream_impl::lockFrameForProcess(bool imgAvaiable) {

    framesMutex.lock();
    int minFrameIdx = -1;
    int64_t minIdx = INT64_MAX;
    for (int i = 0; i < frames.size(); i++) {
        if (frames[i]->isUsing)
            continue;
        if (!frames[i]->imgAvaiable && imgAvaiable) {
            // когда запрашиваем подготовленное изображение, а текущее не подготовлено
            continue;
        }

        if (minIdx > frames[i]->frame) {
            minIdx = frames[i]->frame;
            minFrameIdx = i;
        }
    }
    if (minFrameIdx >= 0) {
        frames[minFrameIdx]->isUsing = true;
    }
    framesMutex.unlock();
    return minFrameIdx;
}

bool VideoLibVLCStream_impl::getNext(cv::Mat &mat, int &frameIdx) {
    if (processingFrameIdx >= 0) {
        framesMutex.lock();
        Frame *processFrame = frames[processingFrameIdx];
        processFrame->isUsing = false;
        processFrame->imgAvaiable = false;
        framesMutex.unlock();
        processingFrameIdx = -1;
    }

    mutex tempMutex;
    unique_lock<mutex> lock(tempMutex);
    bool success = false;
    while (!success) {
        if (finished) {
            return false;
        }

        processingFrameIdx = lockFrameForProcess(true);
        if (processingFrameIdx < 0) {
            newFrameAvaiableCV.wait(lock);
            continue;
        }
        Frame *fr = frames[processingFrameIdx];

        mat = Mat(Size(fr->width, fr->height), CV_8U, fr->buffer);
        frameIdx = fr->frame;
        success = true;
    }
    return success;
}


VideoLibVLCStream_impl::~VideoLibVLCStream_impl() {
    if (vlcmp) {
        libvlc_media_player_stop(vlcmp);
        libvlc_media_player_release(vlcmp);
    }
    libvlc_release(vlc);

    for (auto fr : frames) {
        delete fr;
    }
}

VideoLibVLCStream::VideoLibVLCStream(std::string path) {
    impl = new VideoLibVLCStream_impl(path);
}

VideoLibVLCStream::~VideoLibVLCStream() {
    delete impl;
}

bool VideoLibVLCStream::getNext(cv::Mat &mat, int &frameIdx, std::string &fileName) {
    bool result = impl->getNext(mat, frameIdx);
    fileName = to_string(frameIdx);
    return result;
}
