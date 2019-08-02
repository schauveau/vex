#ifndef VEX_VIDEO_PLAYER_H
#define VEX_VIDEO_PLAYER_H 1

#include "VideoReader.h"


class VideoReader : VideoReaderBase {

public:

  VideoReader() : VideoReaderBase() {
    m_trace = false ;
  }

  ~VideoReader() {
  }
  
private:
  
  virtual run_proceed_t onReadVideoPacket(AVPacket *m_packet, bool &ignore) override 
  {
    return RUN_CONTINUE;
  }
  
  virtual run_proceed_t onSendVideoPacket(AVPacket *m_packet) override 
  {
    return RUN_CONTINUE;
  }

  // virtual void onFailSendVideoPacket(AVPacket *packet,int err) override
  // virtual void onEndOfVideoStream() override ;
  virtual run_proceed_t onReceiveVideoFrame(AVFrame *frame) override
  {
    return RUN_CONTINUE;   
  }
  
  // virtual void onNoMorePackets() ;
  // virtual run_proceed_t onRequireNextVideoPacket();
  // virtual void onFailReceiveVideoFrame(int err);
  // virtual void onNoMorePackets() ;
  // virtual run_proceed_t onRequireNextVideoPacket();
  // virtual void onFailReceiveVideoFrame(int err);

};
    

//
// VideoReader is a very low level class that only takes care of producing the frames
// for a single video input file. VideoReader takes care of more advanced features:
//   - Can manage two videos and take care of the transitions between them. 
//   - Play, Pause, Seek, and other common actions.
//   - Filters and special effects.
//   - Can play a static image
//   - Fit/Crop/Resize videos in the player.
//   - Background color or image when not playing.
//
class VideoPlayer {
public:
  
  VideoReader *reader[2] ;

public:

  
  
};


#endif
