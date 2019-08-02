#include <iostream>
#include <cstring>
#include "Timestamp.h"

Timestamp
operator ""_t(const char *str, std::size_t len)
{
  Timestamp out;
  if ( Timestamp::parse(str,out) )
    return out ;
  else if ( Timestamp::parse_human(str,out) )
    return out ;
  std::cerr << "Failed to parse time '" << str << "'\n";
  exit(1);
}

Timestamp
operator ""_d(const char *str, std::size_t len)
{
  Timestamp out;
  if ( Timestamp::parse(str,out) )
    return out ;
  else if ( Timestamp::parse_human(str,out) )
    return out ;
  std::cerr << "Failed to parse duration '" << str << "'\n";
  exit(1);
}

std::ostream & operator<<( std::ostream &os, const Timestamp &v) {
  os << "{" ;
  if ( v.has_main() ) {
    os << v.main.count << "*" << v.main.num << "/" << v.main.den ; 
  } else {
    os << "0";
  }
  if ( v.has_local() ) {
    os << std::showpos << v.local.count << std::noshowpos << "*" << v.local.num << "/" << v.local.den  ; 
  } else {
    os << "+0";
  }
  os << std::showpos << (v.milli.count/1000.0) << std::noshowpos ;
  os << "}" ;
  return os ;
}

std::string
Timestamp::human()
{
  char buffer[100] ;
  
  int64_t val = int64_t(round(this->eval()*1000)) ;

  if (val>0) {
    buffer[0] = '\0' ;
  } else {
    buffer[0] = '-' ;
    buffer[1] = '\0' ;
    val = - val;
  }

  // val is a number of milliseconds.
  // Decompose into hours, minutes, seconds and milliseconds.
  
  int ms = val % 1000;
  val /= 1000;
  int s = val % 60;
  val /= 60;
  int m = val % 60;
  val /= 60;
  int h = val;
 
  if (h>0) {
    sprintf(buffer+strlen(buffer), "%dh", h);
  }

  // Reminder: Insert show '0m' between hours and seconds.
  //           For example, we want '1h0m5s' instead of
  //           '1h5s' 
  //          
  if (m>0 || (h>0 && (s>0 || ms>0)) ) {
    sprintf(buffer+strlen(buffer),"%dm",m);
  }
 
  if (s>0 || ms>0) {

    sprintf(buffer+strlen(buffer),"%d",s);
   
    if (ms>0) {
      // Remove trailing zeros in the decimal part
      int decimals = ms ; //  so from 000 to 999
      // Get rid of the trailing zeros.
      while (decimals%10==0) {
        decimals /= 10 ;
      }
      sprintf( buffer+strlen(buffer) , ".%d", decimals ) ;
    }
    
    // Only add the final 's' if there was no hours and no minutes.
    // For example, we want 3m12 instead of 3m12s
    //   
    sprintf( buffer+strlen(buffer) , "s" ) ;
  }
  
  return buffer ;
}

std::string
Timestamp::str(bool full)
{
  char buffer[100] ;
  
  int64_t val = int64_t(round(this->eval()*1000)) ;

  const char * sign = full ? "+" : "" ;
  if (val<0) {
    sign = "-" ;
    val = -val ;
  }
  // Decompose into hours, minutes, seconds and milliseconds.
  int ms = val % 1000;
  val /= 1000;
  int s = val % 60;
  val /= 60;
  int m = val % 60;
  val /= 60;
  int h = val;

  if (full) {    
    sprintf(buffer,"%s%02d:%02d:%02d.%03d", sign,h,m,s,ms);
    return buffer;
  }

 
  if (h>0) {
    sprintf(buffer,"%s%d:%02d:%02d", sign,h,m,s);
  } else if (m>0) {
    sprintf(buffer,"%s%d:%02d", sign,m,s);
  } else {
    sprintf(buffer,"%s%d", sign,s);
  }

  if (ms>0) {
    // Remove trailing zeros
    while (ms%10==0) {
      ms /= 10 ;
    }
    // and print decimal part (up to 3 digits)
    sprintf( buffer+strlen(buffer) , ".%d", ms ) ;
  }
  
  return buffer ;
}

bool
Timestamp::parse_human(const char *str, Timestamp &out)
{
  const char *ptr = str;
  int n;
  int h = 0 ;
  int m = 0 ;
  int s = 0 ;
  int ms = 0 ;
  int pos;
  unsigned val;
  char suffix;    
  int sign = 1  ;
  if (*ptr=='-') {
    sign = -1 ;
    ptr++;
  } else if (*ptr=='+') {
    sign = +1 ;
    ptr++;
  }
  
  n = sscanf(ptr, "%u%c%n", &val, &suffix, &pos);
  if (n!=2) goto error;
  ptr += pos ;
  if (suffix=='h' || suffix=='H') goto hours ;
  if (suffix=='m' || suffix=='M') goto minutes ;
  if (suffix=='s' || suffix=='S') goto seconds ;
  if (suffix=='.' ) goto decimals ;
  goto error;
  
 hours: // found '{NUMBER}h'
  h = val;
  // std::cerr << "TRACE[" << __LINE__ << "] '" << ptr  << "'\n" ;
  if ( *ptr == '\0' ) goto success ;
  n = sscanf(ptr, "%u%c%n", &val, &suffix, &pos);
  if (n!=2) goto error;
  ptr += pos ;
  if (suffix=='m' || suffix=='M') goto minutes ;
  if (suffix=='s' || suffix=='S') goto seconds ;
  if (suffix=='.' ) goto decimals ;
  goto error;
  
 minutes: // found '{NUMBER}m'
  if (val >= 60) goto error ; // 0..59 minutes
  m = val; 
  if ( *ptr == '\0' ) goto success ;
  n = sscanf(ptr, "%u%c%n", &val, &suffix, &pos);
  if (n!=2) goto error;
  ptr += pos ;  
  if (suffix=='s' || suffix=='S') goto seconds ;
  if (suffix=='.' ) goto decimals ;
  goto error;
    
 seconds: // found '{NUMBER}s'
  if (val >= 60) goto error ; // 0..59 seconds
  s = val; 
  if ( *ptr == '\0' ) goto success ;
  goto error;
    
 decimals: // found '{NUMBER}.'
  // what follows is a decimal part.
  if (val >= 60) goto error ; // 0..59 seconds
  s = val;
  {
    int dec=1000 ;
    // read up to 4 digits
    while ( '0' <= *ptr && *ptr <= '9' ) {
      ms  += dec*(*ptr-'0') ;
      dec /= 10 ;
      ptr++;
    }
    // and round to milliseconds according to the last digit 
    ms = (ms+5)/10;      
  }
  suffix = *ptr ;
  if (suffix=='s' || suffix=='S') goto success;
  goto error;

 success:
  out = Timestamp::make_ms( sign*(((((h*60)+m)*60)+s)*1000+ms) );
  return true;
  
 error:
  return false;
}

  
bool
Timestamp::parse(const char *str, Timestamp &out)
{
  int64_t us;
  if ( av_parse_time(&us, str, 1)  < 0 ) {
    return false;
  }    
  // av_parse_time gives a duration in micro-seconds
  // so round to milliseconds
  int64_t ms = (us+((us<0)?-500:+500))/1000;    
  out = Timestamp::make_ms(ms);
  return true;
}

Timestamp
Timestamp::add(const Timestamp &b) const
{
  const Timestamp &a = *this ;
  Timestamp out ;
  out.init_num_den(a,b);  
  out.main.count  = a.main.count  + b.main.count;
  out.local.count = a.local.count + b.local.count;
  out.milli.count = a.milli.count + b.milli.count;
  return out;
}


Timestamp
Timestamp::sub(const Timestamp &b) const
{
  const Timestamp &a = *this ;
  Timestamp out ;
  out.init_num_den(a,b);  
  out.main.count  = a.main.count  - b.main.count;
  out.local.count = a.local.count - b.local.count;
  out.milli.count = a.milli.count - b.milli.count;
  return out;
}
  
  
Timestamp
Timestamp::neg() const
{
  const Timestamp &a = *this ;
  Timestamp out ;
  out.init_num_den(a);    
  out.main.count  = - a.main.count;
  out.local.count = - a.local.count;
  out.milli.count = - a.milli.count;
  return out;
}

int
Timestamp::sign() const
{
  int64_t main_count = 0 ; 
  int     main_num   = 1 ;
  int     main_den   = 1 ;
  if ( has_main() ) {
    main_count = main.count ; 
    main_num   = main.num ; 
    main_den   = main.den ; 
  }
  
  int64_t local_count = 0 ; 
  int     local_num   = 1 ;
  int     local_den   = 1 ;
  if ( has_local() ) {
    local_count = local.count ; 
    local_num   = local.num ; 
    local_den   = local.den ; 
  }
  
  int64_t ms_count = milli.count ;
  int     ms_num   = 1 ;
  int     ms_den   = 1000 ;
  
  // So we want the sign of
  //   main_count*main_num/main_den + local_count*local_num/local_den + ms_count*ms_num/ms_den
  // knowing that all 'den' are strictly positive and also assuming no overflow
  // when working with int64_t
  
  // --> sign_of(main_count*main_num/main_den + local_count*local_num/local_den + ms_count*ms_num/ms_den)
  
  // Normalize all denominators to main_den*local_den*ms_den
  
  // --> sign_of( ( main_count  * main_num  * local_den             * ms_den
  //              + local_count * local_num             * main_den  * ms_den
  //              + ms_count    * ms_num    * local_den * main_den           ) 
  //              / (main_den*local_den*ms_den)
  //            )
  // The denominator does not affect the sign because all den are strictly
  // positive so we only care about the numerator
  //
  // --> sign_of( main_count  * main_num  * local_den             * ms_den
  //            + local_count * local_num             * main_den  * ms_den
  //            + ms_count    * ms_num    * local_den * main_den
  //            )
  //
  //
  
  int64_t val =
      ((main_count * main_num) * local_den ) * ms_den
    + ( (local_count * local_num) * ms_den +
        (ms_count    * ms_num   ) * local_den
        ) * main_den
    ;
  
  if (val<0)
    return -1 ;
  else if ( val>0)
    return +1 ;
  else
    return 0;
  
}

#ifdef TEST

#define MAIN_NUM 1001
#define MAIN_DEN 30000

#define LOCAL_NUM 13
#define LOCAL_DEN 666

#define DUMP(t) std::cout  << #t << " = " << (t) << " = " <<  (t).str(true)  << " = " << (t).str(false) << " = " << (t).human() << "\n"


int main(void)
{
  Timestamp t1 = Timestamp::make_main(1,MAIN_NUM,MAIN_DEN) ;
  DUMP(t1);

  Timestamp t2 = Timestamp::make_local(33,LOCAL_NUM,LOCAL_DEN) ;
  DUMP(t2);

  DUMP((t2 + t2));
  DUMP((t1 + 1234.3_s));
  DUMP((t1 + "20:34.3"_t));

  DUMP((t1+t2).round_to_ms());
  DUMP((t1+t2).round_main_to_ms());
  DUMP((t1+t2).round_local_to_ms());

  DUMP(4.4_s);
  DUMP((-12_s));
  DUMP(4561_ms);

  std::cout  <<"==\n";
  DUMP("00:10.333499"_t);
  DUMP("00:10.333500"_t);
  DUMP("00:10.333501"_t);
  DUMP("-00:10.333499"_t);
  DUMP("-00:10.333500"_t);
  DUMP("-00:10.333501"_t);

  DUMP("-3:10:45.456"_t);

  DUMP( max( 4.4_s , 4561_ms ) ) ;

  DUMP("3h"_t) ;
  DUMP("3h4m"_t) ;
  DUMP("3h55m2s"_t) ;
  DUMP("3h11m2.4s"_t) ;
  DUMP("3h11m2.3334999s"_t) ;
  DUMP("3h11m2.3335000s"_t) ;
  DUMP("3h11m2.3335001s"_t) ;
  DUMP("3h"_t) ;
}
#endif
