#ifndef VEX_VIDEO_READER_H
#define VEX_VIDEO_READER_H 1

#include "FFMpegCommon.h"


//
// A base class to implement a video reader.
//
//
class VideoReaderBase : public FFMpegCommon {
protected:
  int                 m_verbosity{1};
  bool                m_trace{true};
  
  std::string         m_filename ;

  AVFormatContext *   m_format_ctxt{NULL};
  AVPacket *          m_packet{NULL};
  
  // ====== Video =======

  bool                has_video()  { return m_video_stream ; } 

  AVStream *          m_video_stream{NULL};     
  int                 m_video_stream_index{-1};
  AVCodec *           m_video_codec{NULL} ;
  AVCodecParameters * m_video_codec_params{NULL};
  AVCodecContext *    m_video_codec_context{NULL};
  int                 m_width{0};
  int                 m_height{0};
  AVFrame *           m_decoded_frame{NULL}; // The frame produced by the code 
  AVFrame *           m_rgb_frame{NULL};     // RGB frame (OBSOLETE)

  SwsContext *        m_sws_to_argb{NULL} ; // Context to convert native frame to argb
  
  // ====== Audio =======

  bool                has_audio() { return m_audio_stream ; }
  
  AVStream *          m_audio_stream{NULL};     
  int                 m_audio_stream_index{-1};
  AVCodec *           m_audio_codec{NULL};
  AVCodecParameters * m_audio_codec_params{NULL};
  AVCodecContext *    m_audio_codec_context{NULL};

  // ============= Play ================

  // Describe the next action that will be performed by play() 
  
  enum run_state_t {
     PSTATE_ERROR,             // An error was detected (bad packet ...) 
     PSTATE_EOF,               // End of file or video was reached.
     PSTATE_READ_PACKET,       // Read the next packet.
     PSTATE_SEND_VIDEO_PACKET, // Send a packet to the video decoder.
     PSTATE_RECEIVE_FRAME,     // Try to receive a frame from the decoder. 
  } ;
  
  run_state_t m_run_state{PSTATE_READ_PACKET} ;

  // The value type of most onXXX callbacks. Indicates
  // how to proceed in the current run.   
  enum run_proceed_t {
     RUN_CONTINUE,   // Continue the run
     RUN_INTERRUPT,  // Interrupt the run temporarily
     RUN_FAIL,       // A failure occurred
     RUN_EOF         // The end of the video was reached
  } ;
  
public:

  VideoReaderBase() {} ;
  VideoReaderBase(std::string filename) ;
  virtual ~VideoReaderBase();


private:

  void init_video(int index, AVStream *stream, AVCodecParameters *params) ;
  void init_audio(int index, AVStream *stream, AVCodecParameters *params) ;

  void dump_stream_info(std::ostream &out, int index) ;

protected: 
  
  // Virtual members of the form onXXX() are expected to 
  // be implemented by a derived class. Each one represents
  // an event occuring during a run() command.

  
  // 
  // Called after a video packet was successfully read
  // from the file.
  //
  // - [in]  packet is the Video Packet
  // - [out] ignore if assigned to false, then the
  //         packet will not be decoded.  
  //
  //
  // Result: if true then continue running else interrupt.
  //
  virtual run_proceed_t onReadVideoPacket(AVPacket *packet, bool &ignore);

  // 
  // Called after a video packet was successfully sent to
  // the decoder.
  //
  // Result: if true then continue running else interrupt.
  //
  virtual run_proceed_t onSendVideoPacket(AVPacket *packet);

  //
  // Called after a video packet was unsuccessfully sent to
  // the decoder. The argument err is an FFMpeg error code.
  //
  // According to the FFMpeg documentation the following errors
  // are possible in that context:
  //
  //  AVERROR(EAGAIN):
  //        Input is not accepted in the current state.
  //        User must read output with avcodec_receive_frame().
  //        Once all output is read, the packet should be resent,
  //        and the call will not fail with EAGAIN.
  //
  //  AVERROR_EOF
  //        the decoder has been flushed, and no new packets
  //        can be sent to it (also returned if more than 1
  //        flush packet is sent)
  //
  //  AVERROR(EINVAL)
  //        Codec not opened, it is an encoder, or requires flush
  //
  //  AVERROR(ENOMEM)
  //        Failed to add packet to internal queue, or similar
  //        other errors: legitimate decoding errors
  //
  // In practice, AVERROR(EAGAIN), AVERROR_EOF or AVERROR(EINVAL)
  // are not expected to happen if decoding is implemented properly.
  //
  //  
  //
  virtual void onFailSendVideoPacket(AVPacket *packet,int err);

  //
  // Called when the end of the video stream is reached. 
  //
  // The run will be interrupted if the return value is false.
  //
  // Reminder: Interrupting is not always necessary since there
  //           could still be a few packets to process (audio,...)
  // 
  virtual void onEndOfVideoStream();

  // Called for each video frame that is successfully received.
  //
  // The run will be interrupted if the return value is false.
  //
  virtual run_proceed_t onReceiveVideoFrame(AVFrame *frame);
  
  // Called when no more packets can be obtained.
  //
  // This is basically EOF.
  //
  virtual void onNoMorePackets() ;

  // Called when the last video packet has been fully
  // processed by the decoder.  
  //
  // The run will be interrupted if the return value is false.
  //
  virtual run_proceed_t onRequireNextVideoPacket();

  // Called when the decoder failed to provide a
  // video frame. This is an error.
  //
  // The run will be interrupted.
  //
  virtual void onFailReceiveVideoFrame(int err);

public:

  // Provide a converter without scaling for the AVFrame produced
  // by this reader.
  //
  // The following arguments of sws_getContext() are computed as follow:
  //   dstW = srcW = this->frameWidth() 
  //   dstH = srcH = this->frameHeight()
  //
  // The other arguments are identical to sws_getContext():
  //   - dstFormat is the pixel format of the destination (AV_PIX_FMT_GRAY8, AV_PIX_FMT_RGB32, ...)
  //   - flags is the scaling method (SWS_FAST_BILINEAR, SWS_BILINEAR, SWS_POINT, ...)
  //   - srcFilter, dstFilter and param are optional arguments.
  //
  FFMpegFrameConverter frameConverter(AVPixelFormat dstFormat,
                                      int flags = SWS_FAST_BILINEAR,
                                      SwsFilter *srcFilter=0,
                                      SwsFilter *dstFilter=0,
                                      const double *param=0
                                      );
  
                      
  // Similar to frameConverter() except that the size of the destination is specified
  // thus allowing scaling.
  FFMpegFrameConverter frameScaler(int dstW,
                                   int dstH,
                                   AVPixelFormat dstFormat,
                                   int flags = SWS_FAST_BILINEAR,
                                   SwsFilter * srcFilter=0,
                                   SwsFilter * dstFilter=0,
                                   const double * param=0
                                   );
  
  // Simple wrapper around sws_getContext() that makes the following assumptions:
  //  - The source will be an AVFrame produced by this reader.
  //  - No scaling! The destination has the same size than the source frame.
  //
  // The following arguments of sws_getContext() are computed as follow:
  //   dstW = srcW = this->frameWidth() 
  //   dstH = srcH = this->frameHeight()
  //
  // The other arguments are identical to sws_getContext():
  //   - dstFormat is the pixel format of the destination (AV_PIX_FMT_GRAY8, AV_PIX_FMT_RGB32, ...)
  //   - flags is the scaling method (SWS_FAST_BILINEAR, SWS_BILINEAR, SWS_POINT, ...)
  //   - srcFilter, dstFilter and param are optional arguments.
  //
  SwsContext *createSwsConverter(AVPixelFormat dstFormat,
                                 int flags = SWS_FAST_BILINEAR,
                                 SwsFilter * srcFilter=0,
                                 SwsFilter * dstFilter=0,
                                 const double * param=0
                                 );
    
 
  // Similar to createSwsConverter except that the size of the destination is specified.
  SwsContext *createSwsScaler(int dstW,
                              int dstH,
                              AVPixelFormat dstFormat,
                              int flags = SWS_FAST_BILINEAR,
                              SwsFilter * srcFilter=0,
                              SwsFilter * dstFilter=0,
                              const double * param=0
                              );

  // Wrapper around sws_scale to convert a decoded AVFrame
  // to a single plane pixel format (e.g. RGB or Grayscale).  
  //
  
  //  - swsCtx is the SWS context created, for example, by
  //    VideoReaderBase::createSwsConverter or VideoReaderBase::createSwsScaler() 
  //  - srcFrame is the source frame (as produced by the decoder)
  //  - dst is the address of the destination data
  //  - dstStride is the stride (in bytes) between two destination 
  //    lines.
  //
  // Remark: Unlike in sws_scale(), dst and dstStride are not  
  //         per-plane arrays. 
  //
  // Return true in case of success or false otherwise.
  //
  // Example: Convert a frame to AV_PIX_FMT_RGB32 with scaling.
  //
  //       // Do that only once after a successful open()
  //
  //       bool scale = ... ; 
  //       int dstW, dstH;
  //       SwsContext *sws_rgb32 ;
  //
  //       if (scale==1.0) {
  //         dstH = this->frameWidth() ;
  //         dstH = this->frameHeight() ;
  //         sws_rgb32 = this->createSwsConverter(AV_PIX_FMT_RGB32,
  //                                              SWS_FAST_BILINEAR);
  //       } else { 
  //         int dstW = scale * this->frameWidth() ;
  //         int dstH = scale * this->frameHeight() ;
  //         sws_rgb32 = this->createSwsScaler(dstW, dstH,
  //                                           AV_PIX_FMT_RGB32,
  //                                           SWS_FAST_BILINEAR);
  //       }
  //
  //       ...
  //
  //       // Obtain a frame
  //       AVFrame * frame = ... ;    
  //      
  //       // Allocate the RGB32 data (so 4 bytes per pixel)
  //       uint8_t * data_rgb32 = new uint8_t[4*dstW*dstH] ;
  //
  //       // And do the conversion
  //       bool ok = this->convertFrame1(sws_rgb32, frame, data_rgb32, 4*dstW);
  //       if (!ok) abort() ;
  //
  bool convertFrame1(SwsContext * swsCtx,
                     AVFrame *    srcFrame,
                     void *       dst,
                     int          dstStride
                     );

  
  // The width of the video frames. 
  int frameWidth();

  // The height of the video frames. 
  int frameHeight();

  // The pixel format of the video frames. 
  AVPixelFormat frameFormat();

  void seek_before(double timestamp) ;

  // Open and prepare the file. 
  virtual bool open() ;
  
  //
  // Run the reader until it is interrupted by an error or by one of
  // the onXXX() callbacks returning false.
  //  
  run_proceed_t run() ;
    
};

#endif

