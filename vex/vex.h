#include <iostream>
#include <chrono>
#include <SDL2/SDL.h>
#include <blend2d.h>

#include "blend2d-support.h"

#include "Color.h"

// Provide named colors from X11
// #include "bl_colors.h"

#include <type_traits>


#if (defined(__ARMEB__)) || (defined(__MIPSEB__)) ||                    \
    (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__))
#define VEX_LITTLE_ENDIAN 0
#define VEX_BIG_ENDIAN 1
#else
#define VEX_LITTLE_ENDIAN 1
#define VEX_BIG_ENDIAN 0
#endif


//
// in C++20, std::endian is an enum that describes endianness
//
// Provide a compatible implementation for C++17.
//
#if __cplusplus > 201703L
using std::endian;
#else
enum class endian
{
  little = 11,  // The actual values do not matter.
  big    = 22,
#if VEX_BIG_ENDIAN
  native = big
#elif VEX_LITTLE_ENDIAN
  native = little  
#endif
};
#endif



using Timer = std::chrono::high_resolution_clock ;
using std::chrono::duration_cast ;
using std::chrono::duration ;


// Convert a duration into nanoseconds (10E-9 seconds)
//
// Timer::time_point T1 = Timer::now() ;
// ...
// Timer::time_point T2 = Timer::now() ; 
// std::cout << "Elapsed time = " << elapsed_ns(T2-T1) << " nanoseconds" ;
// 
inline double elapsed_ns( const Timer::duration &d )
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>(d).count();
}

// Convert a duration into microseconds (10E-6 seconds)
inline double elapsed_us( const Timer::duration &d )
{
  return elapsed_ns(d) * 1E-3 ;
}

// Convert a duration into milliseconds (10E-3 seconds)
inline double elapsed_ms( const Timer::duration &d )
{
  return elapsed_ns(d) * 1E-6 ;
}

// Convert a duration into seconds
inline double elapsed_s( const Timer::duration &d )
{
  return elapsed_ns(d) * 1E-9 ;
}

template <typename T> inline T clamp(T vmin, T v, T vmax) {
  return std::min(std::max(vmin,v),vmax) ;
}

// Provide common actions on (real) timer (play, pause, ...)
class RealTimeControler
{
private:  
  Timer::time_point m_ref_point ;  // The real time point reference
  double            m_ref_time ;   // The local time at m_ref_point
  double            m_speed ;      // The speed of the local time
  double            m_end_time ;   // The end of time 
  double            m_mark ;       // The time marker set by the last action 

  inline double clamp_time(double at) {
    return clamp(0.0, at, m_end_time) ;
  }

  inline double time_point_to_local( const Timer::time_point &tp)
  {
    // Precision to up to 1 microsecond 
    double dt = elapsed_us(tp - m_ref_point) * 1e-6 ; 
    // double dt = elapsed_ns(tp - m_ref_point) * 1e-9 ; 
    return clamp_time( m_ref_time + m_speed * dt ) ; 
  }
  
public:
  RealTimeControler() :
    m_ref_time(0),
    m_speed(0),
    m_end_time(1e100),
    m_mark(0)
  {
  }

  inline RealTimeControler &
  setEndTime(double at) {
    m_end_time = std::max(at,0.0) ;
    return *this ;
  }

  inline RealTimeControler &
  playAction(double speed = 1.0) {
    if ( speed == m_speed ) {
      dummyAction() ;
    } else {
      Timer::time_point tp = Timer::now() ;
      double tlocal = this->time_point_to_local(tp) ;
      m_ref_time  = tlocal ;
      m_ref_point = tp ;
      m_speed     = speed ;
      m_mark      = tlocal ;
    }
    return *this;
  }

  inline RealTimeControler &
  resetAction(double at = 0.0, double speed=0.0) {
    m_ref_point = Timer::now();
    m_ref_time  = this->clamp_time(at);  
    m_speed     = 0 ;
    m_mark      = m_ref_time ;
    return *this;
  }

  // Perform a jump relative to the current local time. 
  inline RealTimeControler &
  jumpAction(double dist) {
    m_ref_time = clamp_time(m_ref_time+dist) ;
    this->dummyAction() ; 
    return *this;
  }

  // A dummy action to can be used to set the time
  // returned by last().
  inline RealTimeControler &
  dummyAction() {
    m_mark  = this->now() ;
    return *this ;
  }
  
  inline RealTimeControler &
  pauseAction() {
    return this->playAction(1.0);
  }
  
  double
  speed() {
    return m_speed ;
  } 

  bool
  is_paused() {
    return m_speed == 0.0 ;
  } 

  // Provide the current local time
  double now()
  {
    return this->time_point_to_local( Timer::now() ) ;
  }

  // Provide the local time of the previous action.
  double last() {
    return m_mark ;
  }
  
} ; 

class AutoTimer {
private:
  Timer::time_point t0;
public:
  inline AutoTimer() { t0 = Timer::now() ; }

  inline void start() { t0 = Timer::now() ; }

  inline void click(const char *name = "?")
  {
    click_ms(name);
  }

  inline void click_ms(const char *name = "?")
  {
    Timer::time_point t1 = Timer::now() ;
    std::cout << "Duration " << name << " = " << elapsed_ms(t1-t0) << " milliseconds\n";
    t0=Timer::now() ;
  }

  inline void click_us(const char *name = "?")
  {
    Timer::time_point t1 = Timer::now() ;
    std::cout << "Duration " << name << " = " << elapsed_ms(t1-t0) << " microseconds\n";
    t0=Timer::now() ;
  }

};



double operator "" _deg(long double v)        { return v*(M_PI/180.0) ; }
double operator "" _deg(unsigned long long v) { return v*(M_PI/180.0) ; }



 

// Create an SDL surface from a BLImage.
// Be aware that the data is still owned by the BLImage. 
SDL_Surface *
createSDLSurface(BLImage &img)
{
  BL_FATAL_DECLARE;
  BLImageData img_data; 

  // Hopefully, that should work regardless of the system endianness. 
  uint32_t SDL_MASK_R = 0x00FF0000u;
  uint32_t SDL_MASK_G = 0x0000FF00u;
  uint32_t SDL_MASK_B = 0x000000FFu;
  uint32_t SDL_MASK_A = 0xFF000000u;

  uint32_t mask_alpha = 0;
  
  img.getData(&img_data) || bl_fatal ;

  switch( img_data.format ) {
  case BL_FORMAT_PRGB32:
    mask_alpha = SDL_MASK_A  ;
    break;
  case BL_FORMAT_XRGB32:
    mask_alpha = 0 ; 
    break;
  default:
    std::cerr << "ERROR: Unsupported BL image format for SDL conversion " << img_data.format << "\n" ;
    abort();
  }
  
  SDL_Surface * surf = SDL_CreateRGBSurfaceFrom(img_data.pixelData,
                                                img.width() ,
                                                img.height(),
                                                32,
                                                img_data.stride,
                                                SDL_MASK_R,
                                                SDL_MASK_G,
                                                SDL_MASK_B,
                                                mask_alpha
                                                );
  return surf ;
}

// An abstract representation of an image (32 bit RGBA). 
class Image {
public:
  int w ;
  int h ; 
  BLContext bl ;
  BLImage * blimg ;
} ;


// A 
//
class Animation {
} ;




