
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <getopt.h>

#include <cstring>
#include <optional>

#include <blend2d.h>

#include <vex/FFMpegCommon.h>
#include <vex/Timestamp.h>
#include <vex/VideoWriter.h>
#include <vex/blend2d-support.h>
#include <vex/TextBox.h>
#include <vex/Color.h>
#include <vex/ArgManager.h>

#include <fontconfig/fontconfig.h>

#include "tests.h"

namespace col=colors ;

namespace video {

  // Some convenienent video sizes.
  // Unless specified otherwise, they all have the same ratio 16/9 = 1.7777

  struct Size {
    int w, h; 
  };
  
  // DVD video size (aka 720p)
  static constexpr Size Size_DVD = {1280, 720};
  // HD = 1.5 * DVD   (aka 1080p)
  static constexpr Size Size_HD = {1920,1080};  
  // 4K = 2 * HD = 6 * DVD
  static constexpr Size Size_4K = {3840,2160};  
  // Small = HD/2 = 640x360
  static constexpr Size Size_Small= { Size_DVD.w/2 , Size_DVD.h/2 };  
  // Tiny = HD/4 = 320x180
  static constexpr Size Size_Tiny = { Size_DVD.w/4, Size_DVD.h/4 };  

}; 


#define autosave(ctx) bl::ContextRestorer _cr_##__LINE__(ctx)

// Some predefined font sizes
namespace fontsize {
  constexpr float XS   =   6.0f;
  constexpr float S    =  10.0f;
  constexpr float M    =  20.0f;
  constexpr float L    =  30.0f;
  constexpr float XL   =  60.0f;
  constexpr float XXL  = 100.0f;
  constexpr float XXXL = 180.0f;
} ;


void draw_cross(BLContext &ctx, double x, double y, double size=20, Color color = col::White ) {
  autosave(ctx); 
  ctx.setFillStyle(color);
  ctx.fillRect(x-size, y     , 2*size,      1);
  ctx.fillRect(x     , y-size,      1, 2*size);
}


class Background {
public:
  virtual ~Background() {} 
  virtual void draw(BLContext &ctx, Timestamp &ts)=0 ;
};

class SolidBackground {
public:
  Color color;  
public:

  SolidBackground(Color &c) : color(c) {
  }                      
                                
  virtual ~SolidBackground() { }

  virtual void
  draw(BLContext &ctx, Timestamp &ts) {
    ctx.setCompOp(BL_COMP_OP_SRC_COPY);
    ctx.setFillStyle(col::Black);
    ctx.fillAll();        
    ctx.setCompOp(BL_COMP_OP_SRC_OVER);
  }
  
};
  
  

class Video {
public:
  Video() {}
  virtual ~Video() {}
  virtual void init()=0;
  virtual void render_image(BLImage &frame, int framenum, Timestamp &ts)=0;
  virtual void render(BLContext &ctx, int framenum, Timestamp &ts)=0;
  virtual int  width()=0;
  virtual int  height()=0;
};

// Common fetures of all animations in this demo
class VideoCommon : public Video {
public:

  BLBox  fullBox;
  BLBox  fullBoxI;
  BLRect fullRect; 
  BLRect fullRectI; 
    
  VideoCommon(video::Size s) {
    fullRect  = BLRect(0,0,s.w,s.h);
    fullRectI = BLRectI(0,0,s.w,s.h);
    fullBox   = BLBox(0,0,s.w,s.h);
    fullBoxI  = BLBoxI(0,0,s.w,s.h);
  }

  virtual ~VideoCommon() {}

  virtual void init() override {
    using namespace fontsize ;

    static bool initialized=false;
    if (!initialized) {
      initialized=true;

      BLFontFace face_default;
      btb::createFaceFromFcPattern(face_default,"sans-serif:style=Regular");        
      btb::registerFont("XS",        face_default, XS);
      btb::registerFont("S",         face_default, S);
      btb::registerFont("M",         face_default, M);
      btb::registerFont("L",         face_default, L);
      btb::registerFont("XL",        face_default, XL);
      btb::registerFont("XXL",       face_default, XXL);
      btb::registerFont("XXXL",      face_default, XXXL);
      
      BLFontFace face_bold;
      btb::createFaceFromFcPattern(face_bold,"sans-serif:style=Bold");  
      btb::registerFont("bold-XS",   face_bold, XS);
      btb::registerFont("bold-S",    face_bold, S);
      btb::registerFont("bold-M",    face_bold, M);
      btb::registerFont("bold-L",    face_bold, L);
      btb::registerFont("bold-XL",   face_bold, XL);
      btb::registerFont("bold-XXL",  face_bold, XXL);
      btb::registerFont("bold-XXXL", face_bold, XXXL);
      
      BLFontFace face_italic;
      btb::createFaceFromFcPattern(face_italic,"sans-serif:style=Italic");
      btb::registerFont("italic-XS",   face_italic, XS);
      btb::registerFont("italic-S",    face_italic, S);
      btb::registerFont("italic-M",    face_italic, M);
      btb::registerFont("italic-L",    face_italic, L);
      btb::registerFont("italic-XL",   face_italic, XL);
      btb::registerFont("italic-XXL",  face_italic, XXL);
      btb::registerFont("italic-XXXL", face_italic, XXXL);
      
      BLFontFace face_mono ;
      btb::createFaceFromFcPattern(face_mono,"monospace:");
      btb::registerFont("mono-XS",     face_mono, XS);
      btb::registerFont("mono-S",      face_mono, S);
      btb::registerFont("mono-M",      face_mono, M);
      btb::registerFont("mono-L",      face_mono, L);
      btb::registerFont("mono-XL",     face_mono, XL);
      btb::registerFont("mono-XXL",    face_mono, XXL);
      btb::registerFont("mono-XXXL",   face_mono, XXXL);
      
      // Provide a few alternative names
      btb::registerFont("default", "M");
      btb::registerFont("bold",    "bold-M");
      btb::registerFont("italic",  "italic-M");
      btb::registerFont("mono",    "mono-M");
      
      // The fallback font is used when an unknown font name is specified
      btb::registerFont("fallback", "italic-M");
    }
    
  };  

  virtual int width()  override { return fullRectI.w ; }
  virtual int height() override { return fullRectI.h ; }
  
  virtual void render_image(BLImage &frame, int framenum, Timestamp &ts) override {
    BLContext ctx(frame);
    
    int actual_w = frame.width();
    int actual_h = frame.height();

    double scale_x = double(actual_w) / width();
    double scale_y = double(actual_h) / height();

    ctx.scale(scale_x,scale_y);
    ctx.userToMeta();

    ctx.save();
    this->render(ctx,framenum,ts);
    ctx.restore();

    this->draw_grid(ctx);
      
    ctx.end();
  }

  void draw_grid(BLContext &ctx) {
    double w = this->width();
    double h = this->height();
    double x,y;

    ctx.setStrokeWidth(1.0);
    ctx.setStrokeStyle(col::White*0.1);

    y=h*0.1 ; ctx.strokeLine(0.0, y, w, y); 
    y=h*0.2 ; ctx.strokeLine(0.0, y, w, y); 
    y=h*0.3 ; ctx.strokeLine(0.0, y, w, y); 
    y=h*0.4 ; ctx.strokeLine(0.0, y, w, y); 
    y=h*0.6 ; ctx.strokeLine(0.0, y, w, y); 
    y=h*0.7 ; ctx.strokeLine(0.0, y, w, y); 
    y=h*0.8 ; ctx.strokeLine(0.0, y, w, y); 
    y=h*0.9 ; ctx.strokeLine(0.0, y, w, y); 
    
    x=w*0.1 ; ctx.strokeLine(x, 0.0, x, h); 
    x=w*0.2 ; ctx.strokeLine(x, 0.0, x, h); 
    x=w*0.3 ; ctx.strokeLine(x, 0.0, x, h); 
    x=w*0.4 ; ctx.strokeLine(x, 0.0, x, h); 
    x=w*0.6 ; ctx.strokeLine(x, 0.0, x, h); 
    x=w*0.7 ; ctx.strokeLine(x, 0.0, x, h); 
    x=w*0.8 ; ctx.strokeLine(x, 0.0, x, h); 
    x=w*0.9 ; ctx.strokeLine(x, 0.0, x, h); 

    ctx.setStrokeWidth(2.0);
    ctx.setStrokeStyle(col::Yellow*0.1);
    y=h*0.5 ; ctx.strokeLine(0.0, y, w, y);
    x=w*0.5 ; ctx.strokeLine(x, 0.0, x, h);
    
  }
  
};



class VideoTextBox : public VideoCommon {
public:

  static constexpr auto VSIZE = video::Size_HD;

  static constexpr int WIDTH  = VSIZE.w;
  static constexpr int HEIGHT = VSIZE.h;
  
  VideoTextBox() : VideoCommon(VSIZE) {
  }

public:

  virtual void init() override {
    using namespace fontsize ;

    this->VideoCommon::init() ;
    
    static bool initialized=false;
    if (!initialized) {
      initialized=true;
      
      BLFontFace face_unifont ;
      btb::createFaceFromFcPattern(face_unifont,"Unifont");
      btb::registerFont("unifont-L",   face_unifont, L);
      
      BLFontFace face_symbols ;
      btb::createFaceFromFcPattern(face_symbols,":charset=2714");
      btb::registerFont("symb-M",     face_symbols, M);
      btb::registerFont("symb-L",     face_symbols, L);
      btb::registerFont("symb-XL",    face_symbols, XL);
      
      // Egyptian hieroglyphs
      BLFontFace face_egypt;
      btb::createFaceFromFcPattern(face_egypt,":charset=0x13200");
      btb::registerFont("egypt-XL",   face_egypt, XL);
    }

  }  
  
  virtual void render(BLContext &ctx, int framenum, Timestamp &ts) override {

    double t = ts.eval() ;
    
   
    ctx.save();
    if (false) {
      ctx.setCompOp(BL_COMP_OP_SRC_COPY);
      ctx.setFillStyle(col::Black);
      ctx.fillAll();        
      ctx.setCompOp(BL_COMP_OP_SRC_OVER);
    } else if (true) {
        // Fill background with a gradient
        ctx.setCompOp(BL_COMP_OP_SRC_COPY);

#if 1
        BLGradient grad1(BLLinearGradientValues(200, 0, 0, 180+cos(t*3.2)*80));
        grad1.addStop(0.0, col::DodgerBlue4 / 0.1);
        grad1.addStop(0.5, col::Chocolate4  / 0.1);
        grad1.addStop(1.0, col::DarkOrchid4 / 0.1);      
        // ctx.setCompOp(BL_COMP_OP_SRC_OVER);
        grad1.setExtendMode(BL_EXTEND_MODE_REFLECT);
        ctx.setFillStyle(grad1);
        ctx.fillAll(); 

        ctx.setCompOp(BL_COMP_OP_SRC_OVER);        
#endif
        
        BLGradient grad2(BLRadialGradientValues(50, 50,
                                                50+cos(t)*40, 50+sin(t*1.5)*40,
                                                150));
        Color bg = 0.4*col::Chocolate4 - 0.7 ;
        grad2.addStop(0.0, 0.20*col::Bisque4     || bg );
        grad2.addStop(0.5, 0.20*col::DarkOrchid4 || bg );
        grad2.addStop(1.0, 0.20*col::Firebrick4  || bg );      
        // ctx.setCompOp(BL_COMP_OP_SRC_OVER);
        grad2.setExtendMode(BL_EXTEND_MODE_REFLECT);
        ctx.setFillStyle(grad2);
        ctx.fillAll();
        
        ctx.setCompOp(BL_COMP_OP_SRC_OVER);
    }
    ctx.restore();
    
    if (true) {
      ctx.setStrokeStyle(0.1*col::Yellow);
      ctx.setStrokeWidth(40.0);
      //  int b=3;
      //  ctx.strokeBox(b,b,WIDTH-2*b,HEIGHT-2) ;
      ctx.strokeBox(this->fullBox) ;
    }

    
    if (true)
    {   
      autosave(ctx);
    
      // Set the context fill color and stroke color to
      // bright green and yellow. 
      ctx.setFillStyle(col::Green4);    
      ctx.setStrokeStyle(col::Yellow);
      ctx.setStrokeWidth(0.5);

      btb::SimpleTextBox tb("default");
      tb.setFillColor(col::DarkRed) ;
      tb.append( u8""
                 "TexBox\n"
                 "^F[bold-XL]^C[FFF]^c[3FFF]^>»»»\r^=Title\r^<«««\n^R"
                 "^="
                 "^F[bold-XL]^C[000]^c[FF0]H^c[FF0]ello^r W^C[FF0]o^rrld^R\n"      
                 "^F[bold-XL]Symb:^F[symb-XL]^C[056]✐ ✑ ✓ ✖ ✗^r\n"
                 "^F[bold-XL]Egypt:^=^F[egypt-XL]^C[000]\U00013051\U00013055\U00013069^R\n"
        );

      // Use Esc-E to change the escape character. 
      tb.append( u8"^E~ aa~C[4f5]xxx^Ryyy~Rzzz~E^");

      for ( int k=0;k<1 ; k++) {
      
        double x = 100+k*240 ;
        double y = 100+k*20 ;

        if (false) {
          tb.setBoxFillColor(col::Bisque4) ;
          tb.drawBox(ctx,x,y) ;
        } else {
          ctx.save();
          double bx = 18.0;
          double by = 18.0;
          double sw = 6.0 ; 
          tb.setBorder(bx,by,bx,by);
          ctx.setFillStyle( col::AntiqueWhite3 % 0.3);
          ctx.setStrokeStyle( col::Yellow % 0.1);
        
          ctx.setStrokeWidth(sw);
          if (false) {
            ctx.fillRect( tb.getRectAt(x,y) );
            ctx.strokeRect( tb.getRectAt(x,y) );            
          } else {            
            BLRect rect = tb.getRectAt(x,y);
            double r = 20.0;
            ctx.fillRoundRect(rect, r);
            ctx.strokeRoundRect( bl::shrink(rect,sw*0.4,sw*0.4), r);            
          }
          ctx.restore();
        }
      
        tb.draw(ctx,x,y);
      
        draw_cross(ctx,x,y,20,col::Red) ;
      
        // Draw points
        if (false) {
          for ( auto xpoint : { btb::xpoint_left, btb::xpoint_center, btb::xpoint_right } ) {
            for ( auto ypoint : { btb::ypoint_top, btb::ypoint_center, btb::ypoint_bottom } ) {
              draw_cross(ctx, x+tb.getPointX(xpoint), y+tb.getPointY(ypoint), 5, col::Black) ;
            }
          }
          for ( auto xpoint : { btb::xpoint_box_left, btb::xpoint_box_center, btb::xpoint_box_right } ) {
            for ( auto ypoint : { btb::ypoint_box_top, btb::ypoint_box_center, btb::ypoint_box_bottom } ) {
              draw_cross(ctx, x+tb.getPointX(xpoint), y+tb.getPointY(ypoint), 5, col::Yellow) ;
            }
          }
          for ( auto xpoint : { btb::xpoint_left, btb::xpoint_center, btb::xpoint_right } ) {
            for ( auto ypoint : { btb::ypoint_top_baseline, btb::ypoint_bottom_baseline } ) {
              draw_cross(ctx, x+tb.getPointX(xpoint), y+tb.getPointY(ypoint), 5, col::Green) ;
            }
          }
        }
      
      } // for k

    }    

  }

} ;



void init_resources()
{
  using namespace fontsize ;
  
  BLFontFace face_default;
  btb::createFaceFromFcPattern(face_default,"sans-serif:style=Regular");
  // btb::createFaceFromFcPattern(face_default,"Droid Sans Fallback");

  // BLFontFace face_emoji;
  // btb::createFaceFromFcPattern(face_emoji,"Noto Color Emoji");
  
  BLFontFace face_bold;
  btb::createFaceFromFcPattern(face_bold,"sans-serif:style=Bold");

  BLFontFace face_italic;
  btb::createFaceFromFcPattern(face_italic,"sans-serif:style=Italic");

  BLFontFace face_mono ;
  // btb::createFaceFromFcPattern(face_mono,":mono:style=bold:charset=0x2710",U"\u2710");
  btb::createFaceFromFcPattern(face_mono,"monospace:",U"\u2710");
  //  btb::createFaceFromFcPattern(face_mono,"monospace:charset=0x45 0x10300",U"\u2710");

  BLFontFace face_unifont ;
  btb::createFaceFromFcPattern(face_unifont,"Unifont");

  BLFontFace face_symbols ;
  char buffer[1000] ;
  sprintf(buffer, ":charset=%x %x %x",
          0x10001,
          unsigned(U'✒'),
          unsigned(U'✔')
    );
  // std::cout << "@@@@" << buffer << "\n";
  btb::createFaceFromFcPattern(face_symbols,buffer);

  // Egyptian hieroglyphs
  BLFontFace face_egypt;
  btb::createFaceFromFcPattern(face_egypt,":charset=0x13200");
#if 0
  float XS   =   6.0f;
  float S    =  10.0f;
  float M    =  20.0f;
  float L    =  30.0f;
  float XL   =  60.0f;
  float XXL  = 100.0f;
  float XXXL = 180.0f;
#endif
  
  btb::registerFont("bold-XS",   face_bold, XS);
  btb::registerFont("bold-S",    face_bold, S);
  btb::registerFont("bold-M",    face_bold, M);
  btb::registerFont("bold-L",    face_bold, L);
  btb::registerFont("bold-XL",   face_bold, XL);
  btb::registerFont("bold-XXL",  face_bold, XXL);
  btb::registerFont("bold-XXXL", face_bold, XXXL);

  btb::registerFont("italic-XS",   face_italic, XS);
  btb::registerFont("italic-S",    face_italic, S);
  btb::registerFont("italic-M",    face_italic, M);
  btb::registerFont("italic-L",    face_italic, L);
  btb::registerFont("italic-XL",   face_italic, XL);
  btb::registerFont("italic-XXL",  face_italic, XXL);
  btb::registerFont("italic-XXXL", face_italic, XXXL);

  btb::registerFont("mono-M",     face_mono, M);
  btb::registerFont("mono-L",     face_mono, L);

  // btb::registerFont("emoji-L",    face_emoji, L);

  btb::registerFont("symb-M",     face_symbols, M);
  btb::registerFont("symb-L",     face_symbols, L);
  btb::registerFont("symb-XL",    face_symbols, XL);

  btb::registerFont("egypt-XL",   face_egypt, XL);

  btb::registerFont("unifont-L",   face_unifont, L);

  // Provide a few alternative names
  btb::registerFont("default", "M");
  btb::registerFont("bold",    "bold-M");
  btb::registerFont("italic",  "italic-M");
  btb::registerFont("mono",    "mono-M");

  // The fallback font is used when an unknown font name is specified
  btb::registerFont("fallback", "italic-M");
}

  
void
draw_frame(BLImage &frame, int framenum, Timestamp &ts)
{
  BLContext ctx(frame);

  double t = ts.eval() ;

  
  
  ctx.setCompOp(BL_COMP_OP_SRC_COPY);
  ctx.fillAll();        
   
  if (true) {
    // Fill background with a gradient
    BLGradient linear(BLLinearGradientValues(0, 0, 200, 180));
    linear.addStop(0.0, col::DodgerBlue1);
    linear.addStop(0.5, col::Chocolate4);
    linear.addStop(1.0, col::DarkOrchid4);   
    
    ctx.setCompOp(BL_COMP_OP_SRC_OVER);
    linear.setExtendMode(BL_EXTEND_MODE_REFLECT);
    ctx.setFillStyle(linear);
    ctx.fillAll(); 
  }  

  
  if (true)
  {   
    autosave(ctx);
    
    // Set the context fill color and stroke color to
    // bright green and yellow. 
    ctx.setFillStyle(col::Green4);    
    ctx.setStrokeStyle(col::Yellow);
    ctx.setStrokeWidth(0.5);

    btb::SimpleTextBox tb("default");
    tb.setFillColor(col::DarkRed) ;
    tb.append( u8""
      "TexBox\n"
      "^F[bold-XL]^C[752]^c[3FFF]^>»»»\r^=Title\r^<«««\n^R"
      "^="
      "^F[bold-XL]^C[000]^c[3F00]H^c[FF0]ello^r W^C[FF0]o^rrld^r\n"      
      "Symb:^F[symb-XL]^C[056]✐ ✑ ✓ ✖ ✗^r\n"

      "Egypt:^=^F[egypt-XL]^C[000]\U00013051\U00013055\U00013069^R\n"
      );

    // Use Esc-E to change the escape character. 
    tb.append( u8"^E~ aa~C[4f5]xxx^Ryyy~Rzzz~E^");
    
    for ( int k=0;k<1 ; k++) {
      
      double x = 100+k*240 ;
      double y = 100+k*20 ;

      if (false) {
        tb.setBoxFillColor(col::Bisque4) ;
        tb.drawBox(ctx,x,y) ;
      } else {
        ctx.save();
        double bx = 18.0;
        double by = 18.0;
        double sw = 6.0 ; 
        tb.setBorder(bx,by,bx,by);
        ctx.setFillStyle( col::AntiqueWhite3 % 0.3);
        ctx.setStrokeStyle( col::Yellow % 0.1);
        
        ctx.setStrokeWidth(sw);
        if (false) {
          ctx.fillRect( tb.getRectAt(x,y) );
          ctx.strokeRect( tb.getRectAt(x,y) );            
        } else {            
          BLRect rect = tb.getRectAt(x,y);
          double r = 20.0;
          ctx.fillRoundRect(rect, r);
          ctx.strokeRoundRect( bl::shrink(rect,sw*0.4,sw*0.4), r);            
        }
        ctx.restore();
      }
      
      tb.draw(ctx,x,y);
      
      draw_cross(ctx,x,y,20,col::Red) ;
      
      // Draw points
      if (false) {
        for ( auto xpoint : { btb::xpoint_left, btb::xpoint_center, btb::xpoint_right } ) {
          for ( auto ypoint : { btb::ypoint_top, btb::ypoint_center, btb::ypoint_bottom } ) {
            draw_cross(ctx, x+tb.getPointX(xpoint), y+tb.getPointY(ypoint), 5, col::Black) ;
          }
        }
        for ( auto xpoint : { btb::xpoint_box_left, btb::xpoint_box_center, btb::xpoint_box_right } ) {
          for ( auto ypoint : { btb::ypoint_box_top, btb::ypoint_box_center, btb::ypoint_box_bottom } ) {
            draw_cross(ctx, x+tb.getPointX(xpoint), y+tb.getPointY(ypoint), 5, col::Yellow) ;
          }
        }
        for ( auto xpoint : { btb::xpoint_left, btb::xpoint_center, btb::xpoint_right } ) {
            for ( auto ypoint : { btb::ypoint_top_baseline, btb::ypoint_bottom_baseline } ) {
              draw_cross(ctx, x+tb.getPointX(xpoint), y+tb.getPointY(ypoint), 5, col::Green) ;
            }
        }
      }
      
    } // for k

  }
  
  ctx.end();  
}

      
int
main(int argc, char **argv)
{
  ArgManager amgr;

  bool show_help = false;
  std::string output_file = "demo1.mkv"  ;
  
  enum Scale {
    scale_native,
    scale_tiny,
    scale_small,
    scale_dvd,
    scale_hd,
    scale_4k,
  };

  std::map<std::string, Scale> scale_values = {
    { "native" , scale_native },
    { "auto"   , scale_native },
    { "tiny"   , scale_tiny },
    { "small"  , scale_small },
    { "dvd"    , scale_dvd },
    { "720p"   , scale_dvd },
    { "hd"     , scale_hd },
    { "1024p"  , scale_hd },
    { "4k"     , scale_4k },
  };
  
  Scale scale = scale_native ;
    
  av_log_set_level(true ? AV_LOG_DEBUG : AV_LOG_ERROR);

  amgr.assign("-h --help -? ", show_help, true) ;

  amgr.parse("-o =FILE ", output_file).
    help("Set the output video file (default \"" + output_file + "\")");

  amgr.select("-S =SCALE", scale, scale_values)
    .help("Scale the output video (native, tiny, small, dvd, hd, 4k)")
    ;
  
  amgr.process(argc,argv);

  if ( argc==1 || show_help ) {
    amgr.usage(std::cout) ;
    exit(1);
  }
    
  // init_resources() ;
  Video *anim = NULL;
 
  anim = new VideoTextBox ;

  video::Size vsize ;
  
  switch (scale) {
  case scale_tiny:
    vsize = video::Size_Tiny;
    break;
  case scale_small:
    vsize = video::Size_Small;
    break;
  case scale_dvd:
    vsize = video::Size_DVD;
    break;
  case scale_hd:
    vsize = video::Size_HD;
    break;
  case scale_4k:
    vsize = video::Size_4K;
    break;
  default:
  case scale_native:
    vsize.w = anim->width();
    vsize.h = anim->height();
    break;
  }
  
  anim->init();
  
  AVRational framerate = FRAMERATE_PAL ;
  VideoWriter writer ;  
  writer.open( "fast", vsize.w, vsize.h, AV_PIX_FMT_BGRA, framerate, output_file );
  
  BLImage frame(vsize.w, vsize.h, BL_FORMAT_PRGB32);

  for (int f=0 ; f<100 ; f++) {     

    Timestamp ts = Timestamp::make_main(f, framerate.den, framerate.num); 
    
    anim->render_image(frame, f,ts);
      
    // Write frame to output
    BLImageData frame_data ;    
    frame.getData(&frame_data) ;     
    writer.add_frame( (uint8_t*) frame_data.pixelData,
                      frame_data.stride
                      );    
  }
  
  writer.close() ;

}

