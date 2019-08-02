#include <optional>
#include <cassert>
#include <map>
#include <vex/vex.h>

namespace col=colors ;

class Font {
private:
  std::string m_filename;
  BLFontFace  m_face ;
  std::map<float,BLFont> m_fonts;
public:
  Font(std::string filename) :
    m_filename(filename)
  {
  }

  BLFont &
  get(float size)
  {
    
    if (!m_face) {
      auto err = m_face.createFromFile(m_filename.c_str()) ;
      if (err!=BL_SUCCESS) {
        std::cerr << "Failed to load font '" << m_filename << "'\n";
        abort();
      }      
    }

    BLFont & font = m_fonts[size] ;
    if (!font) {
      auto err = font.createFromFace(m_face,size) ;
    }
    return font; 
  }
} ;
  

// Hold some global resources 
struct AppResources {
  BLImage animals ;
 
  Font NotoSansBold{"data/NotoSans-Bold.ttf"} ; 
  Font DejaVuSansBold{"data/DejaVuSans-Bold.ttf"} ; 
  Font HackBold{"data/Hack-Bold.ttf"} ; 

  BLFont font1;
  BLFont font2;
  BLFont font3;
  BLFont info_font;
  
  double fps{0} ;

  bool show_fps{false} ;

  bool quit{false} ; 
  
  void init() {
    BL_FATAL_DECLARE ;
    animals.readFromFile("data/animals.jpg") || bl_fatal ;

    font1 = NotoSansBold.get(20);
    font2 = DejaVuSansBold.get(20);
    font3 = HackBold.get(20);
    info_font = NotoSansBold.get(20);
    // face1.createFromFile("data/NotoSans-Bold.ttf") || bl_fatal ;
    // face2.createFromFile("data/DejaVuSans-Bold.ttf") || bl_fatal ;
    // font1.createFromFace(face., 10.0);
    // info_font.createFromFace(face2, 20.0);
    // font3.createFromFace(face1, 100.0);    
  }
} ;

AppResources app ;
std::optional<BLRgba32>  xx;
inline void
drawOutline( BLContext &ctx,
             double x,
             double y,
             BLFont &font,
             const char *text,
             // Optional arguments.
             std::optional<BLRgba32> color_fill = std::nullopt,
             std::optional<BLRgba32> color_line = std::nullopt,
             std::optional<double>   stroke_width = std::nullopt
             )
{
  if (color_line) {
    ctx.setFillStyle(color_line.value());
  }
  if (color_fill) {
    ctx.setStrokeStyle(color_fill.value());
  }
  if (stroke_width) {
    ctx.setStrokeWidth(stroke_width.value());
  }
  ctx.strokeUtf8Text( BLPoint(x,y), font, text);
  ctx.fillUtf8Text(   BLPoint(x,y), font, text);
}

void
generate_image(BLImage &img, double t)
{
  BL_FATAL_DECLARE ;
  
  AutoTimer at ;
  AutoTimer at2 ;
  BLResult err ;

  int W = img.width();
  int H = img.height();
  
  BLContext ctx(img);
  
  // ctx.translate(200,200);

  // Clear the image.

  ctx.setCompOp(BL_COMP_OP_SRC_COPY);
  ctx.setFillStyle(col::AntiqueWhite2);
  //ctx.setFillStyle(BLColor::White);
  ctx.fillAll();

  int x0 = 0 ;
  int y0 = 0 ;
  int w  = W ;
  int h  = H ;

  if (false) {
    // Configure the metaMatrix. 
    // Reminder: That can only be undone by
    //   - reverting the operations (imprecise?)
    //   - or by reseting the whole context
    ctx.resetMatrix();
    ctx.translate(W/2,H/2);
    ctx.scale(1.0);
    ctx.rotate(0_deg);
    ctx.userToMeta();
  }
  

  // ctx.setPatternQuality(BL_PATTERN_QUALITY_NEAREST);
  // ctx.blitImage( BLPoint(0,0), app.animals ) || bl_fatal ;

  
  if (false) {
    ctx.resetMatrix();
    for (int i=-W;i<2*W;i+=10) {
        ctx.setStrokeStyle( (i%100==0) ? col::Black : col::Grey50 );
        if (i==0) {
          ctx.setStrokeStyle(col::Red);
        }
        ctx.strokeLine(-2*W, i, 2*W, i); 
        ctx.strokeLine(i, -2*H, i, 2*H); 
        if (i!=0) {
          ctx.strokeLine(-2*W, -i, 2*W, -i); 
          ctx.strokeLine(-i, -2*W, -i, 2*H); 
        }
      }
    }

  ctx.setFillStyle(col::Blue2);

  ctx.resetMatrix();
  ctx.setCompOp(BL_COMP_OP_SRC_OVER);    
  ctx.setStrokeStyle(col::Red4);


  if (0) { 
    char msg[100] ;
    sprintf(msg,"Hello %f",t) ;
    for (int i=0;i<5;i++) {
      ctx.save();
      ctx.scale(1.0+i);
      double ang = t*M_PI/4+i;
      ctx.strokeCircle(100+20*sin(ang), 100+20*cos(ang), 10);
      {
        ctx.setFillStyle(col::Yellow3);
        ctx.fillUtf8Text(BLPoint(0,20*i), app.font1, msg);
        ctx.setStrokeWidth(0.5);
        ctx.setStrokeStyle(col::Black);
        ctx.strokeUtf8Text(BLPoint(0,20*i), app.font1, msg);
      }
      ctx.restore();
    }
  }

  // Detach the rendering context from `img`.
  ctx.end();

  // Draw frame information
  {
    BLContext ctx(img);

    if (app.show_fps) 
    {
     
      // Draw FPS

      char buffer[20] ;
      sprintf(buffer,"FPS: %-6.1f", app.fps);  
      if (0)
        {
          BLPoint fps_coord(w-120,30) ;
          ctx.setFillStyle(col::AntiqueWhite1);
          ctx.setStrokeStyle(col::Black);
          ctx.setStrokeWidth(1.3);
          ctx.strokeUtf8Text( fps_coord, app.font1, buffer);
          ctx.fillUtf8Text( fps_coord, app.font1, buffer);          
        }
      else
        {
          ctx.setStrokeStyle(col::Blue);
          ctx.setFillStyle(col::Red);
          drawOutline(ctx,                    
                      w-120, 30,
                      app.font1,
                      buffer
                      //, col::AntiqueWhite1
                      //, col::Black                      
                      );
                      
                      
        }
    }
    
    const char *text = "FPS abcdef - Hello World 123";
    ctx.resetMatrix();
    ctx.setFillStyle(col::Blue1);
    ctx.setStrokeStyle(col::Yellow3);
    ctx.setStrokeWidth(1.3);
    ctx.strokeUtf8Text( BLPoint(30,30), app.font1, text);
    ctx.fillUtf8Text( BLPoint(30,30), app.font1, text);
    
    ctx.setStrokeStyle(col::Black);
    
    const int ht=20 ;
    int x=60 ;
    int y=60 ;
    for (int k = 0; k < int(BL_COMP_OP_COUNT) ; k++) {
      char buf[20] ;
      sprintf(buf,"%d",k);
      ctx.setStrokeWidth(1.0);
      ctx.setStrokeAlpha(1.0);
      
      ctx.strokeUtf8Text(BLPoint(80,y+k*ht), app.info_font, buf);


      if (0) {
        ctx.setFillStyle(col::Black);
        ctx.fillUtf8Text(BLPoint(500+0*80,y+k*ht), app.info_font, "Hello World");
      }
      
      {
        const char *text = "Salut";
        ctx.setFillStyle(col::White);

        {
          ctx.save();
          ctx.setCompOp(k);
          
          ctx.setStrokeStyle(col::Black);
          ctx.setStrokeWidth(0.1);
          for (int z=0;z<4;z++)
            ctx.strokeUtf8Text(BLPoint(160+0*80,y+k*ht), app.info_font, text);
          ctx.setStrokeWidth(2);
          for (int z=0;z<1;z++)
            ctx.strokeUtf8Text(BLPoint(260+0*80,y+k*ht), app.info_font, text);
          ctx.setStrokeWidth(1.0);
          for (int z=0;z<1;z++)
            ctx.strokeUtf8Text(BLPoint(360+0*80,y+k*ht), app.info_font, text);
          ctx.restore();
        }
        ctx.fillUtf8Text(BLPoint(160+0*80,y+k*ht), app.info_font, text);        
        ctx.fillUtf8Text(BLPoint(260+0*80,y+k*ht), app.info_font, text);        
        ctx.fillUtf8Text(BLPoint(360+0*80,y+k*ht), app.info_font, text);        

      }
    }
    
    ctx.end();
  }
  

  return ;
}

int
main(int argc, char* args[])
{  
  BL_FATAL_DECLARE ;
  
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "could not initialize sdl2: %s\n", SDL_GetError());
    return 1;
  }

  app.init();
  
  int w = 800 ;
  int h = 600 ;
   
  SDL_Window *win = SDL_CreateWindow( "vex",
                                      // 100,100,
                                      SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                      w, h,
                                        SDL_WINDOW_SHOWN
                                      | SDL_WINDOW_OPENGL
                                      | SDL_WINDOW_RESIZABLE
                                      );
  assert(win);
  
  SDL_Surface* win_surface = SDL_GetWindowSurface(win);
  assert(win_surface);
  
  RealTimeControler rtc ;
  rtc.playAction() ;
  
  BLImage image  ; 
  double  image_timestamp = -1.0 ; // when image was generated
  image.create(w,h,BL_FORMAT_XRGB32);

  BLImageCodec bmp_codec;
  bmp_codec.findByName("BMP") || bl_fatal ;

  // Used to compute app::fps 
  const int fps_len = 10;
  Timer::time_point fps_tp[fps_len] ;
  int fps_id = 0 ;

  // The supported playing speeds 
  double speeds[11] =
    {
     -10.0, -2.0, -1.0, -0.5, -0.25,
     0.0,
     +0.25, +0.5, +1.0, +2.0, +10.0
    };
  size_t speed_count = sizeof(speeds)/sizeof(speeds[0]) ;

  app.show_fps = true ; 
  
  while (!app.quit) {

    double tnow = rtc.now();
   
    // Render and display
    if (tnow != image_timestamp) {
      // Update FPS
      {
        Timer::time_point tp1 = Timer::now();
        if (fps_id>0) {
          int n = std::min(fps_id,fps_len) ;
          Timer::time_point & tp0 = fps_tp[(fps_id+fps_len-n)%fps_len] ;
          double dt = elapsed_s(tp1-tp0) ;
          app.fps = dt ? ( n / elapsed_s(tp1-tp0) ) : 999.0 ;
          // printf("fps = %.2f  (%g/%g)\n",app.fps,tnow,image_timestamp) ;
        }
        fps_tp[fps_id%fps_len] = tp1; 
        fps_id++;
      }
      // 
      generate_image(image,tnow) ;
      image_timestamp = tnow ;
      // Draw in SDL Window
      SDL_Surface * surf = createSDLSurface(image);
      SDL_Rect src  = { 0,0,w,h } ;
      SDL_Rect dest = { std::max( 0, (win_surface->w - w)/2 ) ,
                        std::max( 0, (win_surface->h - h)/2 ) ,
                        0, 0
      } ;      
      SDL_BlitSurface(surf,
                      &src,
                      win_surface,
                      &dest);
      SDL_FreeSurface(surf);
      SDL_UpdateWindowSurface(win);   
    }

    // Consume all events. 
    SDL_Event ev;
    int wait_timeout = rtc.is_paused() ? 100 : 0 ;
    while( SDL_WaitEventTimeout(&ev, wait_timeout)) {

      switch( ev.type ){

      case SDL_QUIT:
        std::cerr << "Quit\n";
        app.quit = true;
        break;
        
      case SDL_WINDOWEVENT:
        if (ev.window.event == SDL_WINDOWEVENT_RESIZED) {
          win_surface = SDL_GetWindowSurface(win);
        }
        break ;
        
      case SDL_KEYDOWN:

        switch(ev.key.keysym.sym) {

        case SDLK_q:
        case SDLK_ESCAPE:
          app.quit = true;
          break;

        case SDLK_c:
          // Also quit on CTRL-C
          if ( (ev.key.keysym.mod & KMOD_CTRL) == ev.key.keysym.mod)
            app.quit = true ;
          break;
          
        case SDLK_LEFTBRACKET: // Slower
          {            
            double current = std::min( speeds[speed_count-1], rtc.speed() ) ;
            for ( size_t i=1; i<speed_count ; i++) {
              if ( current <= speeds[i] ) {
                rtc.playAction(speeds[i-1]);
                std::cerr << "speed = " << rtc.speed() << "\n";
                break; 
              }
            }
          }
          break;

        case SDLK_RIGHTBRACKET: // Faster
          {            
            double current = std::max( speeds[0], rtc.speed() ) ;
            for ( int i=speed_count-2 ; i>=0 ; i--) {
              if ( current >= speeds[i] ) {
                std::cerr << "@" << current << " " << i << "\n";
                rtc.playAction(speeds[i+1]);
                std::cerr << "speed = " << rtc.speed() << "\n";
                break; 
              }
            }
          }
          break;
          
        case SDLK_s: // Save a snapshot
          {

            char filename[30] ;
            sprintf(filename,"out-%.3f.bmp", image_timestamp) ;
            std::cout << "Snapshot " << filename << "\n";
            image.writeToFile(filename, bmp_codec) || bl_fatal ;
          }
          break;

        case SDLK_a: // Save a snapshot and open imvr
          {
            //            image.writeToFile("out.bmp", bmp_codec) || bl_fatal ;
            image.writeToFile("out.bmp", bmp_codec) || bl_fatal ;
            system("imvr -b ff00ff -u nearest_neighbour out.bmp");
          }
          break;


        case SDLK_f:
          {
            app.show_fps = ! app.show_fps ;
          }
          break ;
          
        case SDLK_p: // Play/Pause
          rtc.playAction( rtc.is_paused() ? 1.0 : 0.0 ) ;
          break;
          
        }
        break;
        
      case SDL_KEYUP:
        break;
        
      default:
        break;
      }
    }
  }
  
  SDL_DestroyWindow(win);
  SDL_Quit();
  return 0;
}
