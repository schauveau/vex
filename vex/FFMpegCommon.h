#ifndef VEX_FFMPEG_COMMON_H
#define VEX_FFMPEG_COMMON_H 1

#include <iostream>
#include <cassert>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/pixfmt.h>
#include <libavutil/pixdesc.h>
  //#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>
}

static const AVRational FRAMERATE_NTSC{30000,1001} ; // 'ntsc' ~= 29.970 fps
static const AVRational FRAMERATE_PAL{25,1} ;        // 'pal'  = 25 fps
static const AVRational FRAMERATE_FILM{24,1} ;       // 'film' = 24 fps

inline std::ostream & operator<<(std::ostream &s, const AVRational & v)
{
   s << v.num << "/" << v.den ;
   return s ; 
}

#if 0
inline std::string
AV_ERROR_TEXT(int errnum)
{
  char buf[AV_ERROR_MAX_STRING_SIZE] ;
  av_make_error_string(buf, AV_ERROR_MAX_STRING_SIZE, errnum) ;
  return std::string(buf) ;
}
#endif


class FFMpegCommon {
private:

  char dummy ; 

public:
  
  char error_buffer[AV_ERROR_MAX_STRING_SIZE] ;

  // Provide a textual description for a libav error code.
  //
  // This is basically equivalent to the av_err2str macro
  // except that the later does not work well in C. 
  //
  // Also, the result string is owned by the FFMpegCommon object and
  // should not be freed. It remains valid until the next call.
  //
  // So, this is thread safe as long as the multiple threads
  // do not operate on the same FFMpegCommon object.
  // 
  inline const char *ff_err2str(int errnum)
  {
    av_make_error_string(error_buffer, AV_ERROR_MAX_STRING_SIZE, errnum) ;
    return error_buffer ;
  }
  
  static void dump_hw_config(const AVCodec *codec)
  {
    // So, there is an API to figure out how to use the HW encoder
    // but it is currently (in FFMpeg 4.1) not always implemented for
    // vaapi encoders. 
    // QUESTION: Is that only supposed to work for decoders? 
    std::cout << "[\n";
    const AVCodecHWConfig *hw_config=NULL ;
    int index=0 ;    
    while ( const AVCodecHWConfig *cfg = avcodec_get_hw_config(codec,index++) )
      {
        bool use_device_type=false;
        std::cout << "AVCodecHWConfig #" << (index-1) << "\n" ;
        std::cout << "  pix_fmt = " << av_get_pix_fmt_name(cfg->pix_fmt) << "\n" ;
        if (cfg->methods != 0) {
          std::cout << "  methods = " ;
          const char *sep="" ;
          if ( cfg->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX ) {
            std::cout << "hw_device_ctx" ; sep = ",";
            use_device_type = true;
          }
          if ( cfg->methods & AV_CODEC_HW_CONFIG_METHOD_HW_FRAMES_CTX ) {
            std::cout << sep << "hw_frames_ctx" ; sep = ",";
            use_device_type = true;
          }
          if ( cfg->methods & AV_CODEC_HW_CONFIG_METHOD_INTERNAL) {
            std::cout << sep << "internal" ; sep = ",";
          }
          if ( cfg->methods & AV_CODEC_HW_CONFIG_METHOD_AD_HOC) {            
            std::cout << sep << "ad_hoc" ;  sep = ",";
          }
          std::cout << "\n";
        }
        if (use_device_type)
          std::cout << "  type    = " << av_hwdevice_get_type_name(cfg->device_type) << "\n";          
      }
    std::cout << "]\n";
  }
} ;


//
// A simple wrapper around SwsContext that 
// provides easy conversion to, from and
// between AVFrame.
//
class FFMpegFrameConverter
{
public:
  SwsContext *  ctx{0} ;
  int           srcW{0} ; 
  int           srcH{0} ; 
  AVPixelFormat srcFormat{AV_PIX_FMT_NONE}  ;
  int           dstW{0} ; 
  int           dstH{-100} ;
  AVPixelFormat dstFormat{AV_PIX_FMT_NONE} ;
  int           srcPlanes{0};
  int           dstPlanes{0};
public:

  FFMpegFrameConverter() {
  }
  
  FFMpegFrameConverter( int  	            srcW,
                        int  	            srcH,
                        enum AVPixelFormat  srcFormat,
                        int  	            dstW,
                        int  	            dstH,
                        enum AVPixelFormat  dstFormat,
                        int  	            flags,
                        SwsFilter *  	    srcFilter,
                        SwsFilter *  	    dstFilter,
                        const double *      param
                        ) :
    srcW(srcW),
    srcH(srcH),
    srcFormat(srcFormat),
    dstW(dstW),
    dstH(dstH),
    dstFormat(dstFormat)
  {    
    this->srcPlanes = av_pix_fmt_count_planes(srcFormat) ;
    this->dstPlanes = av_pix_fmt_count_planes(dstFormat) ;
    this->ctx = sws_getContext(srcW,
                               srcH,
                               srcFormat,
                               dstW,
                               dstH,
                               dstFormat,
                               flags,
                               srcFilter,
                               dstFilter,
                               param
                               ) ;
  }
  
public:
      
  inline bool convertFrameToPacked(AVFrame *srcFrame, void *dstData, int dstStride)
  {
    assert( this->srcFormat == AVPixelFormat(srcFrame->format) ) ;
    assert( this->srcW == srcFrame->width ) ;
    assert( this->srcH == srcFrame->height ) ;
    assert( this->dstPlanes == 1) ;

    int n = sws_scale( this->ctx,
                       srcFrame->data,
                       srcFrame->linesize,
                       0, this->srcH,
                       (uint8_t**) &dstData,
                       &dstStride
                       );
    return (n==this->srcH) ;
                       
  }
  
  inline bool convertPackedToFrame(void *srcData, int srcStride, AVFrame *dstFrame)
  {
    assert( this->dstFormat == AVPixelFormat(dstFrame->format) ) ;
    assert( this->dstW == dstFrame->width ) ;
    assert( this->dstH == dstFrame->height ) ;
    assert( this->srcPlanes == 1) ;
            
    int n = sws_scale( this->ctx,
                       (uint8_t**) &srcData,
                       &srcStride,
                       0, this->srcH,
                       dstFrame->data,
                       dstFrame->linesize
                       );
    return (n==this->srcH) ;
  }
  
  inline bool convertFrameToFrame(AVFrame *srcFrame, AVFrame *dstFrame)
  {
    assert( this->srcFormat == AVPixelFormat(srcFrame->format) ) ;
    assert( this->srcW == srcFrame->width ) ;
    assert( this->srcH == srcFrame->height ) ;

    assert( this->dstFormat == AVPixelFormat(dstFrame->format) ) ;
    assert( this->dstW == dstFrame->width ) ;
    assert( this->dstH == dstFrame->height ) ;
    
    int n = sws_scale( this->ctx,
                       srcFrame->data,
                       srcFrame->linesize,
                       0, this->srcH,
                       dstFrame->data,
                       dstFrame->linesize
                       );
    
    return (n==this->srcH) ;
  }

};


#endif
