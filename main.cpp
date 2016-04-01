#include <iostream>
#include <opencv/cv.h>
#include <unistd.h>
#include <highgui.h>

#include "video_vlc_stream.hpp"

using namespace std;
using namespace cv;

int main(int argc, char *argv[])
{
    auto stream = new VideoLibVLCStream(argv[1]);

    Mat img_gray;

    int frame = 0;

    while (true) {
        frame++;
        if (frame % 10 == 0) {
            cout << frame << endl;
        }

        string fileName;
        if(!stream->getNext(img_gray, frame, fileName)) {
            break;
        }
    }
    return 0;
}
