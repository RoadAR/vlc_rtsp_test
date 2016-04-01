#pragma once

#include <cv.h>

class ImageStream {
public:
    virtual bool getNext(cv::Mat &mat, int &frameIdx, std::string& fileName) = 0;
    virtual ~ImageStream();
};
