
#include "VideoReader.h"

#include <libavutil/timestamp.h>

static inline void save_argb_frame(const uint8_t *data, int linesize, int w, int h, const char *filename)
{
    FILE * f = fopen(filename,"w");
    fprintf(f, "P6\n%d %d\n%d\n", w, h, 255);

    uint8_t *out = new uint8_t[w*3] ; // R+G+B 
    // writing line by line
    for (int j=0 ; j<h; j++) {
      uint32_t * in = (uint32_t*)(data+j*linesize) ;
      for (int i=0; i<w ; i++) {
        uint32_t pix = in[i];
        out[3*i+0] = (pix>>16) ; // red
        out[3*i+1] = (pix>>8)  ; // green        
        out[3*i+2] = (pix>>0)  ; // blue
      }
      fwrite(out, 1, w*3, f);
    }
    delete out;
    fclose(f);
}


static inline void save_rgb_frame(const uint8_t *data, int linesize, int w, int h, const char *filename)
{
    FILE * f = fopen(filename,"w");
    fprintf(f, "P6\n%d %d\n%d\n", w, h, 255);    
    for (int j=0 ; j<h; j++) {
      uint32_t * line = (uint32_t*)(data+j*linesize) ;
      fwrite(line, 1, w*3, f);
    }
    fclose(f);
}


static void log(const char *fmt, ...)
{
  va_list args;
  fprintf( stdout, "LOG: " );
  va_start( args, fmt );
  vfprintf( stdout, fmt, args );
  va_end( args );
  fprintf( stdout, "\n" );
}


VideoReaderBase::VideoReaderBase(std::string filename) :
  m_filename(filename)    
{
}

VideoReaderBase::~VideoReaderBase()
{
  // TODO: Free everything
  if (m_packet) {
    av_packet_free(&m_packet);
  }
}


int
VideoReaderBase::frameWidth()
{
  return m_video_codec_context->width ; 
}

int
VideoReaderBase::frameHeight()
{
  return m_video_codec_context->height ;
}

AVPixelFormat
VideoReaderBase::frameFormat()
{
  return m_video_codec_context->pix_fmt ;
}

void
VideoReaderBase::seek_before(double timestamp)
{
  //           
  int     index  = -1 ; // stream index  
  int64_t pos     = AV_TIME_BASE * timestamp ;
  int     flags   = AVSEEK_FLAG_BACKWARD ;
  
#if 0 
  // Another possibility is to seek using one of the streams.
  // Typically, that would be the video stream unless this is
  // an audio file
  
  
  if ( has_video() ) {
    index = m_video_stream_index ;
  } else if ( has_audio() ) {
    index = m_audio_stream_index ;
  }
  
  pos = XXX( timestamp,
             m_format_ctxt->streams[index]->start_time,
             m_format_ctxt->streams[index]->time_base
             ) ;    
#endif
  
  int err = av_seek_frame(m_format_ctxt, index, pos, flags) ;
  if ( err < 0 ) {
    // That should not be a fatal error. 
    std::cerr << "SEEK ERROR: TODO \n" ;
    abort();
  }
  
  if (m_video_codec_context) {
    avcodec_flush_buffers(m_video_codec_context);
  }    

}

FFMpegFrameConverter
VideoReaderBase::frameConverter(AVPixelFormat dstFormat,
                             int flags,
                             SwsFilter *srcFilter,
                             SwsFilter *dstFilter,
                             const double *param
                             )
{
  int srcW = frameWidth() ;
  int srcH = frameHeight() ;
  AVPixelFormat srcFormat = frameFormat() ;
  
  return FFMpegFrameConverter(srcW, srcH, srcFormat,
                              srcW, srcH, dstFormat,
                              flags,
                              srcFilter,
                              dstFilter,
                              param
                              );
}

FFMpegFrameConverter
VideoReaderBase::frameScaler(int dstW,
                          int dstH,
                          AVPixelFormat dstFormat,
                          int flags,
                          SwsFilter * srcFilter,
                          SwsFilter * dstFilter,
                          const double * param
                          )
{
  int srcW = frameWidth() ;
  int srcH = frameHeight() ;
  AVPixelFormat srcFormat = frameFormat() ;
  
  return FFMpegFrameConverter(srcW, srcH, srcFormat,
                              dstW, dstH, dstFormat,
                              flags,
                              srcFilter,
                              dstFilter,
                              param);
}

SwsContext *
VideoReaderBase::createSwsConverter(AVPixelFormat dstFormat,
                                 int flags,
                                 SwsFilter *srcFilter,
                                 SwsFilter *dstFilter,
                                 const double *param
                                 )
{
  int srcW = frameWidth() ;
  int srcH = frameHeight() ;
  AVPixelFormat srcFormat = frameFormat() ;
  
  return sws_getContext(srcW, srcH, srcFormat,
                        srcW, srcH, dstFormat,
                        flags,
                        srcFilter,
                        dstFilter,
                        param);
}

SwsContext *
VideoReaderBase::createSwsScaler(int dstW,
                              int dstH,
                              AVPixelFormat dstFormat,
                              int flags,
                              SwsFilter *srcFilter,
                              SwsFilter * dstFilter,
                              const double * param
                              )
{
  int srcW = frameWidth() ;
  int srcH = frameHeight() ;
  AVPixelFormat srcFormat = frameFormat() ;
  
  return sws_getContext(srcW, srcH, srcFormat,
                        dstW, dstH, dstFormat,
                        flags,
                        srcFilter,
                        dstFilter,
                        param);
}


bool
VideoReaderBase::convertFrame1(SwsContext *swsCtx,
                            AVFrame *srcFrame,
                            void * dst,
                            int    dstStride
                            )
{
  // It is unfortunate that SwsContext is an opaque structure.
  // No way to check that everything is correct.
  // TODO: create a wrapper around SwsContext? Could make sense.
  
  int n = sws_scale( swsCtx,
                     srcFrame->data,
                     srcFrame->linesize,
                     0,             // No clipping at top
                     srcFrame->height, // No clipping at bottom
                     (uint8_t**) &dst,
                     &dstStride
                     ) ;
  return (n == srcFrame->height) ;
}

void
VideoReaderBase::init_video(int index, AVStream *stream, AVCodecParameters *params)
{
  
  if ( has_video() ) {
    // Multiple video stream?
    // Is that even possible?
    std::cerr << "Warning: Found additional video stream. Ignoring\n";
    return ; 
  }
  
  m_video_stream_index = index ;    
  m_video_stream       = stream ;
  m_video_codec_params = params;
  
  AVCodecID    codec_id   = m_video_codec_params->codec_id ;
  const char * codec_name = avcodec_get_name(codec_id) ; // never NULL
  
  m_video_codec = avcodec_find_decoder(codec_id);
  
  if (m_video_codec==NULL) {
    std::cerr << "No decoder for Video codec '" << codec_name << "\n" ;
    exit(1);
  }
  
  m_video_codec_context = avcodec_alloc_context3(m_video_codec);
  
  avcodec_parameters_to_context(m_video_codec_context,
                                m_video_codec_params) ;

  // Enable reference count on all generated frames
  AVDictionary *opts = NULL;
  av_dict_set(&opts, "refcounted_frames", "1" , 0);
  
  if ( avcodec_open2(m_video_codec_context, m_video_codec, &opts) < 0 ) {
    std::cerr << "Failed to open codec through avcodec_open2\n" ;
    exit(1); 
  }
  
  m_width  = m_video_codec_context->width ;
  m_height = m_video_codec_context->height ;
  
  m_decoded_frame = av_frame_alloc();
  if (!m_decoded_frame) {
    log("failed to allocate memory for AVFrame");
    exit(1);
  }
  
  
  // AV_PIX_FMT_0RGB32 or AV_PIX_FMT_RGB32 ?
  // Animated GIFs and PNG can produce transparent images.   
  // Does that make a difference?
  // TODO: Autodected transparency from the codec pixel format? 
  AVPixelFormat argb_format = AV_PIX_FMT_0RGB32;
  
  // Prepare the future conversions to RGB
  // TODO: We could also scale the frame during the conversion.
  m_sws_to_argb = sws_getContext(m_width, m_height, m_video_codec_context->pix_fmt,
                                 m_width, m_height, argb_format, 
                                 SWS_BILINEAR, // SWS_BICUBIC? 
                                 NULL,
                                 NULL,
                                 NULL
                                 );

#if 0            
  // Allocate an RGB frame.
  {
    m_rgb_frame = av_frame_alloc();
    if(m_rgb_frame==NULL) {
      log("failed to allocate memory for AVFrame");
      exit(1);
    }
    uint8_t *buffer = NULL;
    int len = av_image_get_buffer_size(argb_format, width, height, 1);
    buffer = (uint8_t *) av_malloc(len*sizeof(uint8_t));
    av_image_fill_arrays(m_rgb_frame->data, m_rgb_frame->linesize, buffer, argb_format, width, height, 1);
    printf("===== %d %d\n",m_rgb_frame->width,m_rgb_frame->height) ;
    printf("===== %d %d\n",width,height) ;
  }
#endif
  
}


void
VideoReaderBase::init_audio(int index, AVStream *stream, AVCodecParameters *params)
{
  // TODO
  if ( has_audio() ) {
    // Multiple video stream?
    // Is that even possible?
    std::cerr << "Warning: Found other video stream. Ignoring\n";
    return ; 
  } 
}


void
VideoReaderBase::dump_stream_info(std::ostream &out, int index)
{
  AVStream          * stream = m_format_ctxt->streams[index] ;
  AVCodecParameters * params = stream->codecpar;
  
  AVMediaType         media_type = params->codec_type; 
  
  const char *        media_type_str = av_get_media_type_string(media_type) ;
  
  bool audio = (media_type == AVMEDIA_TYPE_AUDIO);
  bool video = (media_type == AVMEDIA_TYPE_VIDEO);
  
  if (!media_type_str) {
    media_type_str = "Unknwon" ;
  }
   
  out << "Stream #" << index  ;
  out << " " << media_type_str ; 

  AVCodecID codec_id = params->codec_id ;    
  out << " (" << avcodec_get_name(codec_id) << ")" ;      

  if (video) {
    out << " " << params->width << "x" << params->height;
  }
    
  out << " time_base="  << stream->time_base  ;
    
  if (stream->start_time==AV_NOPTS_VALUE)
    out << " start_time=?";
  else
    out << " start_time=" << stream->start_time;
    
  if(stream->duration==AV_NOPTS_VALUE)
    out << " duration=?";
  else
    out << " duration=" << stream->duration ; 

      
  if (video) {
      
    out << " frame_rate=" << stream->r_frame_rate ;     
    out << " sample_aspect_ratio=" << stream->sample_aspect_ratio;

    AVPixelFormat format =  (AVPixelFormat) params->format;
    out << " format=" << av_get_pix_fmt_name(format);
      
  } else if (audio) {
    AVSampleFormat format =  (AVSampleFormat) params->format;
    out << " format=" << av_get_sample_fmt_name(format);
    // ... 
  }
        
  out << std::endl ;
}


bool
VideoReaderBase::open()
{
  m_packet = av_packet_alloc();
  if (!m_packet) {
    std::cerr << "Failed to allocate memory for AVPacket\n";
    exit(1);
  }

  m_format_ctxt = avformat_alloc_context();
  if (!m_format_ctxt) {
    std::cerr << "failed to allocate memory for ffmpeg format context\n";
    exit(1);
  }
    
  if (avformat_open_input(&m_format_ctxt, m_filename.c_str() , NULL, NULL) != 0) {
    std::cerr << "failed to open file '" << m_filename << "'\n";
    exit(1);
  }
    
  if (avformat_find_stream_info(m_format_ctxt,  NULL) < 0) {
    std::cerr << "failed to find stream info\n";
    exit(1);
  }
  
  // Find video and audio streams
  for (unsigned index = 0; index < m_format_ctxt->nb_streams; index++)
    {
      
      AVStream * stream = m_format_ctxt->streams[index] ;
      AVCodecParameters *params = stream->codecpar;
      
      dump_stream_info(std::cout, index) ;
      
      switch (params->codec_type) {
      case AVMEDIA_TYPE_VIDEO:
        init_video(index, stream, params) ;         
        break ;
        
      case AVMEDIA_TYPE_AUDIO:
        init_audio(index, stream, params) ;
        break;
        
      default:
        // Other streams are possible (subtitle, ...)
        break ;
      }
      
    }
  
  return has_video() || has_audio() ;
}

VideoReaderBase::run_proceed_t
VideoReaderBase::onReadVideoPacket(AVPacket *m_packet, bool &ignore)
{
  if (m_trace) std::cout << "=== onReadVideoPacket()\n"; 
  bool is_keyframe = m_packet->flags & AV_PKT_FLAG_KEY ;
  return RUN_CONTINUE;
}

VideoReaderBase::run_proceed_t
VideoReaderBase::onSendVideoPacket(AVPacket *m_packet)
{
  if (m_trace) std::cout << "=== onSendVideoPacket()\n"; 
  return RUN_CONTINUE;
}

void
VideoReaderBase::onFailSendVideoPacket(AVPacket *m_packet, int err)
{
  if (m_trace) std::cout << "TRACE: " << __func__ << "\n"; 
  switch (err)
    {
    case AVERROR(ENOMEM) :     // Malformed packet?
    case AVERROR(EAGAIN) :     // Not supposed to happen with our state machine.
    case AVERROR_EOF :         // Not supposed to happen with our state machine.
    case AVERROR(EINVAL) :     // Not supposed to happen with our state machine.
    default:
      std::cerr << "Unexpected send packet error in VideoReaderBase: " << ff_err2str(err) << "\n" ;
      exit(1);
      break;             
    }
}

VideoReaderBase::run_proceed_t
VideoReaderBase::onReceiveVideoFrame(AVFrame *frame) 
{
  if (m_trace) std::cout << "TRACE: " << __func__ << "\n"; 
  // Important: we own the frame data
  av_frame_unref(frame);
  return RUN_CONTINUE ;
}

void
VideoReaderBase::onEndOfVideoStream()
{
  if (m_trace) std::cout << "TRACE: " << __func__ << "\n"; 
}

void
VideoReaderBase::onNoMorePackets()
{
  if (m_trace) std::cout << "TRACE: " << __func__ << "\n"; 
}

VideoReaderBase::run_proceed_t
VideoReaderBase::onRequireNextVideoPacket()
{
  if (m_trace) std::cout << "TRACE: " << __func__ << "\n"; 
  return RUN_CONTINUE;
}

void
VideoReaderBase::onFailReceiveVideoFrame(int err)
{
  if (m_trace) std::cout << "TRACE: " << __func__ << "\n"; 

  std::cerr << "Unexpected send packet error in VideoReaderBase: " << ff_err2str(err) << "\n" ;
  exit(1);
}

VideoReaderBase::run_proceed_t
VideoReaderBase::run() 
{
  run_proceed_t proceed = RUN_CONTINUE;
  
  while(proceed == RUN_CONTINUE)
    {
     switch(m_run_state)
       {
       case PSTATE_ERROR:
         proceed = RUN_FAIL;
         break ;

       case PSTATE_EOF:
         proceed = RUN_EOF;
         break ;
         
       case PSTATE_READ_PACKET:
         {
           int err = av_read_frame(m_format_ctxt, m_packet) ;
           if (err>=0) {
             // Success
             if (m_packet->stream_index == m_video_stream_index) {
               bool ignore = false; 
               proceed = onReadVideoPacket(m_packet, ignore) ;
               if (ignore) {
                 // We are not decoding so go read the next packet 
                 m_run_state = PSTATE_READ_PACKET;
               } else {                 
                 m_run_state = PSTATE_SEND_VIDEO_PACKET;
               }
             } else {
               // Not a video packet.
               // Ignore and continue reading packets.
             }
           } else {
             // Failure.  
             // TODO: Not an error code? Differentiate Error from EOF?
             // TODO: Do we need to flush the decoder to get a few more frames?
             this->onNoMorePackets() ;
             m_run_state = PSTATE_EOF;
           }           
         }
         break ;
     
       case PSTATE_SEND_VIDEO_PACKET:
         {
           int err = avcodec_send_packet(m_video_codec_context, m_packet);

           if ( err>=0 ) {
             // Success
             m_run_state = PSTATE_RECEIVE_FRAME;
             proceed = this->onSendVideoPacket(m_packet) ;
           } else {
             // Failure
             this->onFailSendVideoPacket(m_packet,err);
             m_run_state = PSTATE_ERROR ;
           }
         }
         break;

       case PSTATE_RECEIVE_FRAME: 
         {
           int err = avcodec_receive_frame( m_video_codec_context, m_decoded_frame);
           // From documentation:
           //
           //  AVERROR(EAGAIN)
           //        Output is not available in this state.
           //        User must try to send new input
           //  AVERROR_EOF
           //        The decoder has been fully flushed, and there
           //        Will be no more output frames
           //  AVERROR(EINVAL)
           //        Codec not opened, or it is an encoder
           //
           //  other negative values
           //        legitimate decoding errors

           if (err>=0) {
             // Success. We have a frame.
             // The default behavior is to try again because
             // frames can arrive in chunks. 
             m_run_state = PSTATE_RECEIVE_FRAME ;
             proceed = this->onReceiveVideoFrame(m_decoded_frame) ;
             av_frame_unref(m_decoded_frame);
           } else if (err==AVERROR_EOF) {
             // The decoder has been fully flushed, and there
             // will be no more output frames.
             this->onEndOfVideoStream() ;  
             m_run_state = PSTATE_EOF;
           } else if (err==AVERROR(EAGAIN)) {
             // Not an error: The decoder requires more video packets
             // before it can provide the next video frame.
             m_run_state = PSTATE_READ_PACKET;
             proceed = this->onRequireNextVideoPacket() ;  
           } else {
             m_run_state = PSTATE_ERROR;
             this->onFailReceiveVideoFrame(err) ;
           }
         }
         break ;
         
       } // end of switch(m_run_state) 
     
    } // of while(more)

  return proceed;
}

#ifdef TEST

#include "VideoWriter.h"


class TestVideoReader : public VideoReaderBase
{
private:

  
  int packet_counter{0} ;
  int frame_counter{0} ;

  FFMpegFrameConverter fconv;
 
  static const AVPixelFormat rgb_fmt=AV_PIX_FMT_RGB24 ;

  VideoWriter writer;
    
public:

  TestVideoReader(std::string filename) : VideoReaderBase(filename) {   

  }

  int stop_at_frame{0} ;
  
private:

  
  void save_frame(AVFrame *frame, int index)
  {
    if ( this->fconv.dstFormat != AV_PIX_FMT_RGB24)
      return ;      
    char filename[1024];   
    int w = this->fconv.dstW;
    int h = this->fconv.dstH;
    int stride = av_image_get_linesize(this->fconv.dstFormat,w,0) ; 
    assert(stride==3*w); 
    uint8_t * data = new uint8_t[stride*h] ;
    this->fconv.convertFrameToPacked(frame, data, stride) ; 
    
    // Quick save as a PPM file   
    sprintf(filename, "frame-%05d.ppm", frame_counter);
    FILE * f = fopen(filename,"w");
    fprintf(f, "P6\n%d %d\n%d\n", w, h, 255);
    fwrite(data, 1, stride*h, f);
    fclose(f);
    delete data ;
  }
  
  
protected:
  
  virtual run_proceed_t onReadVideoPacket(AVPacket *m_packet, bool &ignore) override    
  {
    // if (m_trace) std::cout << "TRACE: " << __func__ << "\n";   
    //  bool is_keyframe = m_packet->flags & AV_PKT_FLAG_KEY ;
    packet_counter++;
    return RUN_CONTINUE;
  }
  
  virtual run_proceed_t onSendVideoPacket(AVPacket *m_packet) override 
  {
    // if (m_trace) std::cout << "TRACE: " << __func__ << "\n";   
    return RUN_CONTINUE;
  }

  virtual void onEndOfVideoStream() override 
  {
    if (m_trace) std::cout << "TRACE: " << __func__ << "\n";   
  }

  virtual run_proceed_t onReceiveVideoFrame(AVFrame *frame) override 
  {
    // The frame time is specified relative to its stream.
    if (m_trace) {
      char ts[AV_TS_MAX_STRING_SIZE] ;
      av_ts_make_time_string(ts, frame->pts, &m_video_stream->time_base);
      std::cout << "TRACE: " << __func__  ;
      std::cout << " pts=" << frame->pts
                << " pts_time=" << ts ;
      if (frame->key_frame)
        std::cout << " [KEYFRAME]";
      std::cout << "\n";
    }
    
    frame_counter++ ;
   
    // Consume the frame 
    if (false) {
      this->save_frame(frame, frame_counter);
    } else { 
      int w = this->fconv.dstW;
      int h = this->fconv.dstH;
      int stride = av_image_get_linesize(this->fconv.dstFormat,w,0) ; 
      assert(stride==3*w);
      // TODO: No need to reallocate each and every time
      uint8_t * data = new uint8_t[stride*h] ;
      this->fconv.convertFrameToPacked(frame, data, stride) ;
      // write RGB frame to video file
      writer.add_frame(data,stride);
      delete data ;
    }
    av_frame_unref(frame);
    
    if ( frame_counter == stop_at_frame )
      return RUN_INTERRUPT ;
    
    return RUN_CONTINUE;
  }

  virtual run_proceed_t onRequireNextVideoPacket() override 
  {
    // if (m_trace) std::cout << "TRACE: " << __func__ << "\n";
    return RUN_CONTINUE;
  }

  virtual void onFailReceiveVideoFrame(int err) override 
  {
    if (m_trace) std::cout << "TRACE: " << __func__ << "\n";
  }

  virtual void onNoMorePackets() override 
  {
    if (m_trace) std::cout << "TRACE: " << __func__ << "\n";
  }

  virtual void onFailSendVideoPacket(AVPacket *m_packet,int err) override 
  {
    if (m_trace) std::cout << "TRACE: " << __func__ << "\n";
  }

public:

  virtual bool open() override
  {
    bool ok = this->VideoReaderBase::open() ;

    if (!ok) {
      abort() ;
      return false ;
    }
    
    double scale = 1.0 ;    
    this->fconv = this->frameScaler( scale * this->frameWidth(),
                                     scale * this->frameHeight(),
                                     this->rgb_fmt,
                                     SWS_FAST_BILINEAR) ;
    
    //    this->sws_ctx = this->createSwsConverter(AV_PIX_FMT_0RGB32, SWS_FAST_BILINEAR) ;
    //if (!this->sws_ctx) {
    //  std::cerr << "Failed to create SWS context\n";
    //  exit(1);
    //}      

    writer.open("default",
                this->fconv.dstW,
                this->fconv.dstH,
                this->rgb_fmt,
                FRAMERATE_PAL, // wrong
                "test-reader.mkv" ) ; 

    return true;  
  }

  
};

int
main(int argc, char **argv)
{
  //  VideoReaderBase vid("sample.mp4");
  //  VideoReaderBase vid("sample-big.mp4");

  // Prevent libav to produce annoying warnings.
  av_log_set_level( AV_LOG_ERROR );

  const char *filename = "videos/testsrc2.mp4" ;
  if ( argc==2 ) {
    filename = argv[1]; 
  }
  
  TestVideoReader reader(filename);
  reader.open() ;
  reader.stop_at_frame = 40 ;

  reader.run() ;
  std::cout << "SEEK\n" ; reader.seek_before(23.0);
  
  reader.stop_at_frame = 80 ;
  reader.run() ;
  
  std::cout << "SEEK\n" ; reader.seek_before(59.9);
  reader.run() ;
  
  return 0 ;
}

#endif

