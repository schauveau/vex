#ifndef VEX_TIMESTAMP_H
#define VEX_TIMESTAMP_H

#include <cstdlib>
#include <cstdint>
#include <cmath>
#include <iostream>

extern "C" {
#include <libavutil/parseutils.h>
}

//
// The Timestamp class describe a point in time (or a duration) in a way that 
// prevents all rounding errors.
//
// The problematic of time keeping in vex is that we have to consider at least
// consider 3 ways to measure or express the time. 
// 
//  - The time in the output video stream.
//  - The time in the (multiple) input video(s) streams. 
//  - and the human-readable time in the timeline.  
//
// In FFmpeg, the time in each video stream is expressed by an AVRational time_base
// and a integer timestamp (e.g. pts, dts, ...). For the output video, the time_base
// is defined once according to the framerate chosen by the user. However, the time_base
// is likely to be different in each input video. For instance, an input may use
// 1/24 (PAL), another 1001/30000 (NTSC) and a third could be an animated gif at 1/10.
//
// In the timeline, the time is expressed in milli-seconds.
//
// Fortunately, the input video streams are mostly independant of each others.
// (except when synchrononizing one video with another) so the basic idea is to
// represent a time as the sum of the following 3 times:
//   
//  main_ts*main_tb + milli_ts*(1/1000) + local_ts*local_tb
//
//  main_ts and main_tb represent a time in the output stream (so the main timeline).
//  millis_ts is a time expressed in milliseconds
//  local_ts and local_tb represent a time in a local context (typically an input stream).
//
// main_tb and local_tb can be undefined (0/1) if their respective 'ts' factor is zero.
//
// Two timestamps can be compared, added or substracted as long as their tb are compatible.
// If this is not the case then an exception is triggered. Functions are also provided to round the
// local time component into the milli_ts component.
//
// 
//
// 
//
struct Timestamp {
public:

  struct {
    int64_t count;
    int     num;
    int     den;
  } main ;

  struct {
    int64_t count;
    int     num;
    int     den;
  } local ;
  
  struct {
    int64_t count ;
    // int num = 1 ;
    // int den = 1000 ;
  } milli;
  
public:

  inline Timestamp() :
    main{0,0,0},
    local{0,0,0},
    milli{0}
  {
  }

  inline Timestamp(int64_t main_count, int main_num, int main_den,
            int64_t local_count, int local_num, int local_den, 
            int64_t milliseconds
            ) :
    main{main_count,main_num,main_den},
    local{local_count,local_num,local_den},
    milli{milliseconds}
  {    
  }

  // For convenience, allows double precisions to descibe time
  // but be aware that 
  inline Timestamp(double s) :
    main{0,0,0},
    local{0,0,0},
    milli{int64_t(s*1000)}
  {    
  }
  
  static inline Timestamp make_main(int64_t count, int num, int den) {
    return Timestamp( count, num, den,
                      0, 0, 0,
                      0 );
  }

  static inline Timestamp make_local(int64_t count, int num, int den) {
    return Timestamp( 0, 0, 0,
                      count, num, den,
                      0 );
  }
  
  static inline Timestamp make_ms(int64_t count) {
    return Timestamp( 0, 0, 0,
                      0, 0, 0,
                      count);
  }

  //
  // A simple parser to generate a timestamp from a duration description
  // of the form
  //    [-][HH:]MM:SS[.m...]
  // or
  //    [-]S+[.m...]
  //
  // This is internally using av_parse_time() except that the precison is
  // limited to 1ms (instead of 1us)
  //
  // See also the literal operator "..."_t below
  //
  static bool parse(const char *str, Timestamp &ts) ;  

  // Parse a timestamp string using the format described in Timestamp::human()
  static bool parse_human(const char *str, Timestamp &ts) ;  
  
  // Indicate if the main num/den is valid (i.e. non 0/0)
  // Remark: A timestamp with a valid main num/den can have a zero main.count
  bool inline has_main() const {
    return !(main.num==0 && main.den==0) ;
  }

  // Indicate if the local num/den is valid (i.e. non 0/0).
    // Remark: A timestamp with a valid main num/den can have a zero main.count
  bool inline  has_local() const {
    return !(local.num==0 && local.den==0) ;
  }

  // Initialize main.num, main.den, local.num and local.den from the
  // specified timestamp.
  inline void init_num_den(const Timestamp &a)
  {
    this->main.num = a.main.num;
    this->main.den = a.main.den;
    this->local.num = a.local.num;
    this->local.den = a.local.den;
  }
  
  // Initialize main.num, main.den, local.num and local.den from two compatible
  // time stamps. A fatal error will occur if they are not compatible.
  inline void init_num_den(const Timestamp &a, const Timestamp &b)
  {
    // Initialize main_num_den
    if (a.has_main()) {
      if (b.has_main()) {
        if ( a.main.num != b.main.num || a.main.den != b.main.den ) {          
          std::cerr << "Incompatible main_num_den\n" ; // unlikely unless the user did something very wrong.
          exit(1); 
        }
      }
      this->main.num = a.main.num;
      this->main.den = a.main.den;    
    } else if (b.has_main() ) {
      this->main.num = b.main.num;
      this->main.den = b.main.den;
    } else {
      this->main.num = 0;
      this->main.den = 0;
    }

    if (a.has_local()) {
      if (b.has_local()) {
        if ( a.local.num != b.local.num || a.local.den != b.local.den ) {          
          std::cerr << "Incompatible local_num_den\n" ; // unlikely unless the user did something very wrong.
          exit(1); 
        }
      }
      this->local.num = a.local.num;
      this->local.den = a.local.den;    
    } else if (b.has_local() ) {
      this->local.num = b.local.num;
      this->local.den = b.local.den;
    } else {
      this->local.num = 0;
      this->local.den = 0;
    }
  }

 
  inline Timestamp round_to_ms() {
    double v = 0 ;
    if ( has_main() ) {
      v += (double(this->main.count)*double(this->main.num))/double(this->main.den) ;
    }
    if ( has_local() ) {
      v += (double(this->local.count)*double(this->local.num))/double(this->local.den) ;
    }
    return Timestamp( 0,0,0,
                      0,0,0,
                      this->milli.count + int64_t(round(v*1000.0))
                      );
  }
  
  inline Timestamp round_main_to_ms() {
    double v = 0 ;
    if ( has_main() ) {
      v += (double(this->main.count)*double(this->main.num))/double(this->main.den) ;
    }
    return Timestamp( 0,0,0,
                      this->local.count, this->local.num, this->local.den,
                      this->milli.count + int64_t(round(v*1000.0))
                      );
  }
  
  inline Timestamp round_local_to_ms() {
    double v = 0 ;
    if ( has_local() ) {
      v += (double(this->local.count)*double(this->local.num))/double(this->local.den) ;
    }
    return Timestamp( this->main.count, this->main.num, this->main.den,
                      0,0,0,
                      this->milli.count + int64_t(round(v*1000.0))
                      );
  }

  inline Timestamp ceil_to_ms() {
    double v = 0 ;
    if ( has_main() ) {
      v += (double(this->main.count)*double(this->main.num))/double(this->main.den) ;
    }
    if ( has_local() ) {
      v += (double(this->local.count)*double(this->local.num))/double(this->local.den) ;
    }
    return Timestamp( 0,0,0,
                      0,0,0,
                      this->milli.count + int64_t(ceil(v*1000.0))
                      );
  }
  
  inline Timestamp ceil_main_to_ms() {
    double v = 0 ;
    if ( has_main() ) {
      v += (double(this->main.count)*double(this->main.num))/double(this->main.den) ;
    }
    return Timestamp( 0,0,0,
                      this->local.count, this->local.num, this->local.den,
                      this->milli.count + int64_t(ceil(v*1000.0))
                      );
  }
  
  inline Timestamp ceil_local_to_ms() {
    double v = 0 ;
    if ( has_local() ) {
      v += (double(this->local.count)*double(this->local.num))/double(this->local.den) ;
    }
    return Timestamp( this->main.count, this->main.num, this->main.den,
                      0,0,0,
                      this->milli.count + int64_t(ceil(v*1000.0))
                      );
  }

  inline Timestamp floor_to_ms() {
    double v = 0 ;
    if ( has_main() ) {
      v += (double(this->main.count)*double(this->main.num))/double(this->main.den) ;
    }
    if ( has_local() ) {
      v += (double(this->local.count)*double(this->local.num))/double(this->local.den) ;
    }
    return Timestamp( 0,0,0,
                      0,0,0,
                      this->milli.count + int64_t(floor(v*1000.0))
                      );
  }
  
  inline Timestamp floor_main_to_ms() {
    double v = 0 ;
    if ( has_main() ) {
      v += (double(this->main.count)*double(this->main.num))/double(this->main.den) ;
    }
    return Timestamp( 0,0,0,
                      this->local.count, this->local.num, this->local.den,
                      this->milli.count + int64_t(floor(v*1000.0))
                      );
  }
  
  inline Timestamp floor_local_to_ms() {
    double v = 0 ;
    if ( has_local() ) {
      v += (double(this->local.count)*double(this->local.num))/double(this->local.den) ;
    }
    return Timestamp( this->main.count, this->main.num, this->main.den,
                      0,0,0,
                      this->milli.count + int64_t(floor(v*1000.0))
                      );
  }
  

  // Evaluate the timestamp as a 'double'.
  // Rounding may occur.
  inline double eval()
  {
    double v = 0 ;
    if ( has_main() ) {
      v += (double(this->main.count)*double(this->main.num))/double(this->main.den) ;
    }
    if ( has_local() ) {
      v += (double(this->local.count)*double(this->local.num))/double(this->local.den) ;
    }
    v += this->milli.count/1000.0 ;
    return v; 
  }

  // Convert to a human readable string.
  //
  // If full=true then the ouput uses the format
  // '[+-]HH:MM:SS.mmm' (13 characters if smaller
  // for any time smaller 100 hours):
  // 
  //   +01:30:45.500     # 1 hour, 30 minutes, 45 seconds and 500 ms
  //   +00:00:01.000     # 1 second  
  //   -00:00:01.200     # negative 1 second and 200 milliseconds 
  //   +00:00:01.234     # 1 second and 234 milliseconds 
  //   +00:03:00.000     # 3 minues  
  //   +01:30:00.000     # 1 hour and 30 minutes  
  //
  std::string str(bool full=false) ;  

  
  // Similar to Timestamp::str() but produces a more readable 
  // output for normal humans. 
  //
  //   1h30m45.5         # 1 hour, 30 minutes, 45 seconds and 500 ms
  //   1s                # 1 second  
  //   -1.2s             # negative 1 second and 200 milliseconds 
  //   1.234s            # 1 second and 234 milliseconds 
  //   3m                # 3 minutes  
  //   1h30m             # 1 hour and 30 minutes
  // 
  //
  // Remark: that output is NOT valid for Timestamp::parse()
  //
  // TODO: write a Timestamp::human_parse() function?
  //
  std::string human() ;  

public: // Arithmetic operations on timestamps 

  
  Timestamp add(const Timestamp &b) const ;
  Timestamp sub(const Timestamp &b) const ;   
  Timestamp neg() const ;

  // Return -1 if the timestamp is negative, 0 if null and +1 if positive
  int sign() const ;

} ;


std::ostream & operator<<( std::ostream &os, const Timestamp &v) ;


//
// 
//
//

inline Timestamp operator+( const Timestamp &a, const Timestamp &b ) {
  return a.add(b) ;
}

inline Timestamp operator-( const Timestamp &a, const Timestamp &b ) {
  return a.sub(b) ;
}

inline Timestamp operator-( const Timestamp &a ) {
  return a.neg() ;
}

inline bool operator==( const Timestamp &a, const Timestamp &b ) {
  return (a-b).sign() == 0 ;
}

inline bool operator!=( const Timestamp &a, const Timestamp &b ) {
  return (a-b).sign() != 0 ;
}

inline bool operator<( const Timestamp &a, const Timestamp &b ) {
  return (a-b).sign() < 0 ;
}

inline bool operator<=( const Timestamp &a, const Timestamp &b ) {
  return (a-b).sign() <= 0 ;
}

inline bool operator>( const Timestamp &a, const Timestamp &b ) {
  return (a-b).sign() > 0 ;
}

inline bool operator>=( const Timestamp &a, const Timestamp &b ) {
  return (a-b).sign() >= 0 ;
}

inline Timestamp max(const Timestamp &a, const Timestamp &b) {
  return (a>b) ? a : b ;
}

inline Timestamp min(const Timestamp &a, const Timestamp &b) {
  return (a<b) ? a : b ;
}

inline Timestamp operator ""_s(long double v) { return Timestamp::make_ms(int64_t(v*1000)) ; }
inline Timestamp operator ""_s(unsigned long long v) { return Timestamp::make_ms(int64_t(v*1000)) ; }
inline Timestamp operator ""_ms(unsigned long long v) { return Timestamp::make_ms(int64_t(v)) ; }


// User defined string literal generate timestamps.
//
// Examples using the Timestamp::parse() format 
//
//     "12.123"_t      = 12 seconds and 123 milliseconds
//     "1:56:33"_t     = 1 hour, 56 minutes and 33 seconds
//     "-2:00"_t       = minus 2 minutes
//     "1:23.4"_t      = 1 minute, 23 seconds and 400 milliseconds
//     "1:23.456789"_t = 1 minute, 23 seconds and 457 milliseconds (rounded to nearest ms) 
//
// Equivalent using the Timestamp::parse_human() format
//
//     "12.123s"_t      = 12 seconds and 123 milliseconds
//     "1h56m33s"_t     = 1 hour, 56 minutes and 33 seconds
//     "-2m"_t          = minus 2 minutes
//     "1m23.4s"_t      = 1 minute, 23 seconds and 400 milliseconds
//     "1m23.456789s"_t  = 1 minute, 23 seconds and 457 milliseconds (rounded to nearest ms) 
//
Timestamp operator ""_t(const char *str, std::size_t len);

// The user defined string literal "..."_d is stricty equivalent to "..."_t.
// It is only provided to help differentiate times from durations when 
// reading the code.
Timestamp operator ""_d(const char *str, std::size_t len) ;


#endif
