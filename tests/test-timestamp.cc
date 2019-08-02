
#include <iostream>
#include <cstring>
#include <vex/Timestamp.h>

#define MAIN_NUM 1001
#define MAIN_DEN 30000

#define LOCAL_NUM 13
#define LOCAL_DEN 666

#define DUMP(t) std::cout  << #t << " = " << (t) << " = " <<  (t).str(true)  << " = " << (t).str(false) << " = " << (t).human() << "\n"


int
main(void)
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

