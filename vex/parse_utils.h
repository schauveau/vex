#ifndef VEX_PARSE_UTILS_H
#define VEX_PARSE_UTILS_H

#include <blend2d.h>
#include <string_view>

//
// Parse a color specified as 3, 4, 6 or 8 hexadecimal characters
// 
//   rgb
//   argb
//   rrggbb
//   aarrggbb
//
// Return: In case of success true else false.
//
// Remark: argb was used instead of rgba because 
//         Blend2d allows BLRgba32(0xAARRGGBB)
//    
inline bool
parse_argb(std::string_view arg, BLRgba32 &col ) {
  if ( arg.size() > 8 )
    return false ;
  int val[8] ;
  size_t i;
  
  for (i=0;i<arg.size();i++) {
    char c = arg[i];
    if ( '0' <= c && c <='9' )
      val[i] = c-'0';
    else if ( 'A' <= c && c <='F' )
      val[i] = c-'A'+0xA;
    else if ( 'a' <= c && c <='f' )
      val[i] = c-'a'+0xA;
    else
      break;
  }
  
  if (i!=arg.size()) {
    return false;
  }
  
  int a,r,g,b;
  switch(arg.size()) {
  case 3: // rgb
    a = 0xFF;
    r = (val[0] << 4) + val[0] ; 
    g = (val[1] << 4) + val[1] ;  
    b = (val[2] << 4) + val[2] ; 
    break;
  case 4: // argb
    a = (val[0] << 4) + val[0] ;
    r = (val[1] << 4) + val[1] ; 
    g = (val[2] << 4) + val[2] ;  
    b = (val[3] << 4) + val[3] ; 
    break;
  case 6: // rrggbb
    a = 0xFF;
    r = (val[0] << 4) + val[1] ; 
    g = (val[2] << 4) + val[3] ;  
    b = (val[4] << 4) + val[5] ; 
    break;
  case 8: // aarrggbb
    a = (val[0] << 4) + val[1] ;
    r = (val[2] << 4) + val[3] ; 
    g = (val[4] << 4) + val[5] ;  
    b = (val[6] << 4) + val[7] ; 
    break;
  default:
    return false; 
  }
  col = BLRgba32(r,g,b,a) ;
  return true; 
}

// Parse a string quoted by c1 and c2. 
// 
// Return true in case of success.
//
// Example:
//  parse_quoted( "{abc}" , '{', out, '}' )
//     -> true and out='abc'
//  parse_quoted( "{abc" , '{', out, '}' )
//     -> false
//  
inline bool
parse_quoted(std::string_view text, char c1, std::string_view &out, char c2) {
  if (text.size() >= 2 && text[0] == c1) {
    auto n = text.find(c2,1);
    if (n != std::string_view::npos) {
      out = text.substr(1,n-1);
      return true;
    }
  }
  return false;
}

//
// Parse a color surrounded by c1 and c2 starting at text[pos].
//
// In case of success, return true and set pos to the position after c2.
//
// Example:
//   pos = 6 ;
//   parse_quoted_color( "color={34af56},date=now", pos, '{', color, '}' )
//      -> return true and set pos=14 
//
inline bool
parse_quoted_hexcolor( std::string_view text, size_t &pos, char c1, BLRgba32 &color, char c2 ) {
  std::string_view arg; 
  if ( parse_quoted(text.substr(pos),c1,arg,c2) ) {
    if ( parse_argb(arg,color) ) {
      pos += arg.size() + 2 ;
      return true;
    }
  }
  return false; 
}

#endif
