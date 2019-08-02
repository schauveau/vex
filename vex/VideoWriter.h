#ifndef VEX_VIDEO_WRITER_H
#define VEX_VIDEO_WRITER_H

#include "FFMpegCommon.h"

// This is a very simple class to write video files
// from a stream of RGB images.
//
// Most of the work is actually done by a separate ffmpeg process
// started in the shell script src/video-encoder
//
class VideoWriter : FFMpegCommon {
public:
  int width=0; 
  int height=0;
  AVPixelFormat pixfmt=AV_PIX_FMT_NONE;  
  FILE *pipe=NULL;
  std::string video_encoder{VideoWriter::default_video_encoder} ; 
public:
  static void set_default_video_encoder(std::string program) ;
private:
  static std::string default_video_encoder ;
public:
  void open(std::string preset, int w, int h, AVPixelFormat pixfmt, AVRational framerate, std::string filename) ;     
  void add_frame(uint8_t *data, int stride) ;
  void close() ;  
};

#endif
