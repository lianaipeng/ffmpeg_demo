#include <iostream>

#ifdef __cplusplus
extern "C"
{
    #define __STDC_CONSTANT_MACROS
    #ifndef INT64_C
    #define INT64_C
    #define UINT64_C
    #endif
    #include <stdint.h>

    #include "libavutil/avutil.h"
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libavdevice/avdevice.h"


    #include "libswscale/swscale.h"
    #include "libswresample/swresample.h"
}
#endif

#include <iostream>
using namespace std;


int main(int argc, char **argv) {
    
    return 0;
}
