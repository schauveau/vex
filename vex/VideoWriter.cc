#include <cassert>

#include <sstream>
#include <iomanip>

#include <vex/config.h>
#include <vex/VideoWriter.h>

std::string VideoWriter::default_video_encoder = VEX_DATA_DIR "/video-encoder";

// Escape an argument for 'sh'
// This is not perfect.
// TODO: Get rid of intermediate sh in popen() using pipe/fork/dup2/execl
static std::string
escape(const std::string &str)
{
  if ( str.find('`') != std::string::npos )  {
    std::cerr << "Cannot espace '`' character in ffmpeg command\n";
    std::exit(1);
  } else if ( str.find('$') != std::string::npos )  {
    std::cerr << "Cannot espace '$' character in ffmpeg command\n";
    std::exit(1);
  } else if ( str.find_first_of(" '\"\\") != std::string::npos )  {
    std::stringstream ss;
    ss << std::quoted(str,'\"','\\') ;
    return ss.str() ;
  } else {
    return str;
  }
}

void VideoWriter::open(std::string preset, int w, int h, AVPixelFormat fmt, AVRational framerate, std::string filename)                                   
{    
  assert(pipe==NULL);

  width  = w; 
  height = h;
  pixfmt = fmt;
  
  // For now, we do not need to support multiple planes 
  assert(av_pix_fmt_count_planes(pixfmt)==1);  
  
  std::stringstream cmd ;
  
  cmd << this->video_encoder ;
  cmd << " " << escape(preset) ;
  cmd << " " << width << "x" << height ;
  cmd << " " << av_get_pix_fmt_name(pixfmt) ;
  cmd << " " << framerate;
  cmd << " " << escape(filename);
  
  std::string fullcmd = cmd.str() ;
  
  std::cout << "CMD: " << fullcmd << "\n";
  pipe = popen(fullcmd.c_str() , "w") ;
    
  if (!pipe) {
    std::cerr << "ERROR: Failed to execute command\n" ;
    std::exit(1);
  }    
  
}
    
void
VideoWriter::add_frame(uint8_t *data, int stride)
{
  size_t n = av_image_get_linesize(pixfmt, width, 0); 
  for (int y=0;y<height;y++) {
    size_t res = fwrite(data, n, 1, pipe) ;
    if (res != 1) {
      std::cerr << "ERROR: Failed to write frame to encoder process\n";
      std::exit(1);
    }
    data += stride ;
  }
}

void
VideoWriter::close() {
  assert(pipe!=NULL);
  int n = pclose(pipe);
  std::cerr << "ffmpeg command terminate with " << n << "\n";
  pipe = NULL;
}



#ifdef TEST

const int WIDTH   = 1024;
const int HEIGHT  = 768;
const AVRational fps  = FRAMERATE_PAL;
const int nframes = (30.0*fps.num)/fps.den ;

struct color_t {
  color_t() {}
  color_t(uint8_t r,uint8_t g,uint8_t b,uint8_t a=0) :
    b(b),g(g),r(r),a(a)
  {    
  }
  uint8_t b,g,r,a ;
};

color_t red(255,0,0) ;
color_t green(0,255,0) ;
color_t blue(0,0,255) ;
color_t black(0,0,0) ;

color_t img[HEIGHT][WIDTH] ;

void fillbox( int x, int y,
              int w, int h,
              color_t color
              )
{
  if (x<0) { w+=x; x=0; }
  if (y<0) { h+=y; y=0; }
  
  w = std::min(w,WIDTH-x) ;
  h = std::min(h,HEIGHT-y) ;

  //  printf("== %d %d %d %d\n", x,y,w,h);
  for (int j=0;j<h;j++) {
    for (int i=0;i<w;i++) {
      img[y+j][x+i] = color ;
    }
  }
}
          

void box( int x, int y,
          int w, int h,
          color_t color
          )
{
  int s = 2 ;
  fillbox(x,y,w,s,color);
  fillbox(x,y,s,h,color);
  fillbox(x+w-s,y,s,h,color);
  fillbox(x,y+h-s,w,s,color);
}

int main(void)
{ 
  av_log_set_level(true ? AV_LOG_DEBUG : AV_LOG_ERROR);

  VideoWriter writer ;
  
  writer.open( "fast", WIDTH, HEIGHT, AV_PIX_FMT_BGRA, FRAMERATE_PAL, "out.mkv" );
  
  struct Info {
    int x,y,w,h ;
    double speed ;
    int r;
    color_t c ;
  } ;

  const int nb = 20 ;
  Info boxes[nb] ;

  for (int i=0;i<nb;i++) {
    boxes[i].x = rand() % WIDTH ;
    boxes[i].y = rand() % HEIGHT ;
    boxes[i].w = 10 + rand() % WIDTH/3 ;
    boxes[i].h = 10 + rand() % HEIGHT/3 ;
    boxes[i].speed = 10 + rand() % 40 ;
    boxes[i].r = 10 + rand() % HEIGHT/2  ;
    boxes[i].c.r = rand() % 0xFF ;
    boxes[i].c.g = rand() % 0xFF ;
    boxes[i].c.b = rand() % 0xFF ;
    boxes[i].c.a = 0xFF ;
  }
  
  for (int iframe = 0; iframe < nframes; iframe++)
    {
      fillbox(0,0,WIDTH,HEIGHT,black) ;

      for (int i=0;i<nb;i++) {
        Info &b = boxes[i] ;
        if (i%2) {
          fillbox( b.x + b.r*sin(iframe/b.speed),
                   b.y + b.r*cos(iframe/b.speed),
                   b.w,
                   b.h,
                   b.c) ;                   
        } else {
          box( b.x + b.r*sin(iframe/b.speed),
               b.y + b.r*cos(iframe/b.speed),
               b.w,
               b.h,
               b.c) ;                   
        }
      }

      writer.add_frame( (uint8_t*)img , WIDTH*sizeof(color_t) ) ;
    }

  writer.close();
}

#endif
