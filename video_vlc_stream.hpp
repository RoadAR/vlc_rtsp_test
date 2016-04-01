//
//  video_stream.hpp
//  test_libvlc
//
//  Created by Alexander on 01.03.16.
//  Copyright © 2016 RoadAR. All rights reserved.
//

#ifndef video_stream_hpp
#define video_stream_hpp

#include <stdio.h>
#include <vlc/vlc.h>
#include <pthread.h>
#include "image_stream.hpp"

class VideoLibVLCStream_impl;

class VideoLibVLCStream: public ImageStream {
public:
    VideoLibVLCStream(std::string path);
    bool getNext(cv::Mat &mat, int &frameIdx, std::string& fileName);
    ~VideoLibVLCStream();
private:
    VideoLibVLCStream_impl *impl;
};

#endif /* video_stream_hpp */
