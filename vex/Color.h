#ifndef VEX_COLOR_H
#define VEX_COLOR_H

#include <iostream>
#include <iomanip>

// The class Color represents a color. 
//
// The constructor
// 

class Color {
public:

  // The actual type used to store the color components.
  // Warning: The storage type may change in future versions
  // (e.g. 16 bit vs 8bit per component)

#if 0
  // Store 8 bits per component
  typedef uint32_t storage_type ;
  typedef uint8_t  component_type ;
#else
  // Store 16 bits per component
  typedef uint64_t storage_type ;
  typedef uint16_t component_type ;
#endif
  
private:

  template <typename T>
  static constexpr inline T clamp(T a, T b, T c) {
    return std::max(a, std::min(b,c) ) ; 
  } 
  
private: 
  
  storage_type storage;

  static constexpr bool storage_is_argb8  = sizeof(storage_type) == 4 ;
  static constexpr bool storage_is_argb16 = sizeof(storage_type) == 8 ;
    
  // Number of bits per storage components (8 or 16)
  static constexpr unsigned CBIT = sizeof(component_type) * 8 ;

  // Maximum value of storage components (0xFF or 0xFFFF)
  static constexpr unsigned CMAX = (1<<CBIT)-1 ; 
 
  static constexpr storage_type SHIFT_A = 3*CBIT; 
  static constexpr storage_type SHIFT_R = 2*CBIT; 
  static constexpr storage_type SHIFT_G = 1*CBIT; 
  static constexpr storage_type SHIFT_B = 0*CBIT; 
   
  static constexpr storage_type MASK_A = storage_type(CMAX) << SHIFT_A ;
  static constexpr storage_type MASK_R = storage_type(CMAX) << SHIFT_R ;
  static constexpr storage_type MASK_G = storage_type(CMAX) << SHIFT_G ;
  static constexpr storage_type MASK_B = storage_type(CMAX) << SHIFT_B ;
  
private:

  // Pack R, G,B and A components to the storage type.
  static inline constexpr
  storage_type
  pack_rgba( component_type r,
             component_type g,
             component_type b,
             component_type a)
    {    
      return
        ( storage_type(a) << SHIFT_A) |
        ( storage_type(r) << SHIFT_R) |
        ( storage_type(g) << SHIFT_G) |
        ( storage_type(b) << SHIFT_B) ;      
    }

  // Pack 8-bit R, G, B and A components to the storage type
  static inline constexpr
  storage_type
  pack_rgba_8( uint8_t r,
               uint8_t g,
               uint8_t b,
               uint8_t a)
    {
      if constexpr ( CBIT==8 ) {
        return pack_rgba(r,g,b,a); 
      } else {
        return pack_rgba(r*0x0101,
                         g*0x0101,
                         b*0x0101,
                         a*0x0101); 
      }      
    }

  // Pack 16-bit R, G, B and A components to the storage type
  static inline constexpr
  storage_type
  pack_rgba_16( uint16_t r,
                uint16_t g,
                uint16_t b,
                uint16_t a)
    {
      if constexpr ( CBIT==16 ) {
        return pack_rgba(r,g,b,a); 
      } else {
        return pack_rgba(r>>8,
                         g>>8,
                         b>>8,
                         a>>8);
      }      
    }
  
public:

  inline Color() noexcept =default ; 
  constexpr Color(const Color &) noexcept =default ; 

  // First constructor: Define the color as 0xAARRGGBB  
  constexpr inline
  Color( uint32_t argb ) noexcept :
    storage(0) 
  {
    if ( storage_is_argb8 )
      storage = argb;
    else
      storage = pack_rgba_8( argb>>16 , argb>>8, argb, argb>>24 );
  }  

  // Second constructor: Define the color as percentages of Red, Green, Blue
  // and Alpha.
  constexpr inline
  Color ( double r_pc, double g_pc, double b_pc, double a_pc=100.0 ) noexcept :
    // Remark: Using *2.55 instead if *255/100 does not work well because of
    // rounding errors: An input value of 100.0 would produce 254 instead of 255.
    // The division in the current implementation are probably a bit expensive
    // but that should not matter much since that constructor shall be mostly
    // used in constexpr contexts. 
    storage( pack_rgba( (clamp(0.0,r_pc,100.0)*CMAX)/100,
                        (clamp(0.0,g_pc,100.0)*CMAX)/100,
                        (clamp(0.0,b_pc,100.0)*CMAX)/100,
                        (clamp(0.0,a_pc,100.0)*CMAX)/100))
    {
    }    
  
  // Convenient constructor when using Blend2D.
  constexpr inline
  Color( const BLRgba32 &col ) noexcept :
    storage(0)
  {
    // NOTE: Assuming that BLRgba32 storage is argb8 
    uint32_t argb = col.value;
    if ( storage_is_argb8 )
      storage = argb;
    else
      storage = pack_rgba_8( argb>>16 , argb>>8, argb, argb>>24 );
  }  
  
  // Convenient constructor when using Blend2D.
  constexpr inline
  Color( const BLRgba64 &col ) noexcept :
    storage(0)
  {
    // NOTE: Assuming that BLRgba64 storage is argb16
    uint64_t argb = col.value;
    if ( storage_is_argb16 )
      storage = argb;
    else
      storage = pack_rgba_16( argb>>32 , argb>>16, argb, argb>>48 );
  }  

  
  constexpr inline
  operator BLRgba32() const {
    return BLRgba32( this->argb8() );
  }

  
  // The Blend2d API often allows both BLRgba32 and BLRgba64.
  // Allowing both casts to be implicit would cause ambiguities
  // so one of them must be explicit.
  explicit constexpr inline
  operator BLRgba64() const {
    return BLRgba64( this->argb16() );
  }

  constexpr bool isOpaque() const noexcept {
    return (storage & MASK_A) == MASK_A ;
  }

  constexpr bool isTransparent() const noexcept {
    return (storage & MASK_A) == 0 ;
  }

  // Return the 8bit A, R, G, and B components encoded in a
  // 32 unsigned storage as 0xAARRGGBB.     
  constexpr uint32_t argb8() const noexcept {
    if (storage_is_argb8) 
      return storage ; 
    uint32_t v = a8() ;
    v <<= 8 ;
    v |= r8() ;
    v <<= 8 ;
    v |= g8() ;
    v <<= 8 ;
    v |= b8() ;
    return v;
  }
  
    
  // Return the 16bit A, R, G, and B components encoded in a
  // 64 unsigned storage as 0xAAAARRRRGGGGBBBB.     
  constexpr uint64_t argb16() const noexcept {
    if (storage_is_argb16)
      return storage ;     
    uint64_t v = a16() ;
    v <<= 16 ;
    v |= r16() ;
    v <<= 16 ;
    v |= g16() ;
    v <<= 16 ;
    v |= b16() ;
    return v;
  }

  // Build a Color from 8bit R, G, B and A components
  static constexpr inline Color 
  rgba8(uint8_t r, uint8_t g, uint8_t b, uint8_t a=0xFF ) {
    return Color::makeStorage( pack_rgba_8(r,g,b,a) ) ;      
  }  

  // Build a Color from R, G, B and A components in range [0,1] 
  static constexpr inline Color 
  rgba1( double r, double g, double b, double a=1.0 ) {
    return Color::makeStorage( pack_rgba( (clamp(0.0,r,1.0) * CMAX),
                                          (clamp(0.0,g,1.0) * CMAX),
                                          (clamp(0.0,b,1.0) * CMAX),
                                          (clamp(0.0,a,1.0) * CMAX)));
  }  

    
  // Return the 8bit R, G, and B components encoded in a
  // 32 unsigned storage as 0x00RRGGBB.     
  constexpr uint32_t rgb8() const noexcept {
    if constexpr (storage_is_argb8) {
      return storage && ~MASK_A ;
    } else {
      uint32_t v = 0;
      v <<= 8 ;
      v |= r8() ;
      v <<= 8 ;
      v |= g8() ;
      v <<= 8 ;
      v |= b8() ;
      return v;
    }
  }

  constexpr uint64_t rgb16() const noexcept {
    if constexpr (storage_is_argb16) {
      return storage && ~MASK_A ;
    } else { 
      uint64_t v = 0;
      v <<= 16 ;
      v |= r16() ;
      v <<= 16 ;
      v |= g16() ;
      v <<= 16 ;
      v |= b16() ;
      return v;
    }
  }

  // Provide the actual storage.
  inline constexpr storage_type getStorage() const {
    return storage;
  }

  // Build from a storage value. 
  static inline constexpr
  Color makeStorage( storage_type v) {
    Color c(0) ;
    c.storage = v;
    return c; 
  }

  
private:

  // Get the component in either 8 ot 16 bit format
  constexpr inline component_type getA() const { return storage >> SHIFT_A; }
  constexpr inline component_type getR() const { return storage >> SHIFT_R; }
  constexpr inline component_type getG() const { return storage >> SHIFT_G; }
  constexpr inline component_type getB() const { return storage >> SHIFT_B; }  
  
  // Provide conversion between 8 bit and 16bit components
  static constexpr inline uint8_t  convert8(uint8_t v)   { return v; }
  static constexpr inline uint8_t  convert8(uint16_t v)  { return v>>8; }
  static constexpr inline uint16_t convert16(uint8_t v)  { return v*0x0101; }
  static constexpr inline uint16_t convert16(uint16_t v) { return v; }
  
public:
  constexpr inline uint8_t a8() const { return convert8( this->getA() ) ; }
  constexpr inline uint8_t r8() const { return convert8( this->getR() ) ; }
  constexpr inline uint8_t g8() const { return convert8( this->getG() ) ; }
  constexpr inline uint8_t b8() const { return convert8( this->getB() ) ; }
  
  constexpr inline uint16_t a16() const { return convert16( this->getA() ) ; }
  constexpr inline uint16_t r16() const { return convert16( this->getA() ) ; }
  constexpr inline uint16_t g16() const { return convert16( this->getA() ) ; }
  constexpr inline uint16_t b16() const { return convert16( this->getA() ) ; }

  constexpr inline float a1() const { return float(getA()) / float(CMAX); }
  constexpr inline float r1() const { return float(getR()) / float(CMAX); }
  constexpr inline float g1() const { return float(getG()) / float(CMAX); }
  constexpr inline float b1() const { return float(getB()) / float(CMAX); }  


  // Convert RGB Color to HSL 
  //
  // Ouput:
  //  - hue is in the range [0..6)
  //  - sat is in the range [0..1]
  //  - lum is in the range [0..1]
  // 
  constexpr inline void
  toHSL6( double &hue, double  &sat, double &lum) const
  {
    // See https://en.wikipedia.org/wiki/HSL_and_HSV#From_RGB

    double r = r1();
    double g = g1();
    double b = b1();
    
    double min = std::min(r,std::min(g,b)) ;
    double max = std::max(r,std::max(g,b)) ;
    
    lum = (max+min)/2;
    
    double delta = max-min ; 
    if (delta==0) {
      hue = sat = 0; // gray 
    } else {    
      if (max==r) {
        hue = (g-b) / delta;  // [-1,1]
        if (hue<0)
          hue += 6 ; // -> [0,1] + [5,6)
      } else if (max==g) {
        hue = 2+(b-r) / delta; // [1,3]
      } else {
        hue = 4+(r-g) / delta; // [3,5] 
      }
      sat = (max-lum) / std::min(lum,1-lum) ; 
    }    
  }


  // Build a Color from HSL components 
  //
  // Inputs:
  //  - hue in [0..6)
  //  - sat in [0..1]
  //  - lum in [0..1]
  //  - alpha in [0..1]
  //  
  static constexpr inline Color
  fromHSL6(double hue, double  sat, double lum, double alpha=1.0) {
    // See https://en.wikipedia.org/wiki/HSL_and_HSV#HSL_to_RGB

    hue   = clamp(0.0,hue,6.0);
    sat   = clamp(0.0,sat,1.0);
    lum   = clamp(0.0,lum,1.0);
    alpha = clamp(0.0,alpha,1.0);
    
    // if (hue==6.0)
    //   hue=0.0;
    // 
    // Remark: When hue==0.0 or hue=6.0, we get hmod2=0 and x=0
    //         which means that hue=6.0 can be handled by either
    //         the case '0' or the default case below.
    //    
    
    double hmod2 = hue - 2*int(hue/2); // because fmod(hue,2.0) is not constexpr 
    
    double c = (1 - std::abs(2*lum-1)) * sat;
    double x = c * (1-std::abs(hmod2-1)) ;
    double rr=0, gg=0, bb=0 ;
    switch(int(hue)) {
    case 0:   rr = c ; gg = x ; bb = 0 ; break;
    case 1:   rr = x ; gg = c ; bb = 0 ; break;
    case 2:   rr = 0 ; gg = c ; bb = x ; break;
    case 3:   rr = 0 ; gg = x ; bb = c ; break;
    case 4:   rr = x ; gg = 0 ; bb = c ; break;
    default:  rr = c ; gg = 0 ; bb = x ; break; // 5 but also 6 when hue==6.0 
    }
    float m = lum - c/2 ;
    return Color::rgba1( m+rr, m+gg, m+bb, alpha ); 
  }

  
  // Porter-Duff: S over D 
  //
  // S is a transparent layer over the background D. 
  //
  // Formula: 
  //    R = S + D*(1 - Sa)  
  //
  constexpr inline Color
  over(Color D) const
  {
    const Color &S = *this; 
    if ( S.isOpaque() ) {
      // Fully opaque source
      return S; 
    } else if ( S.isTransparent() ) {
      // Fully transparent source
      return D;  
    } else if ( D.isOpaque() ) {
      // Fully opaque destination
      float Sa = S.a1() ; // normalized S alpha [0..1]
      float Rr = ( (S.r1()-D.r1())*Sa + D.r1() ) ;
      float Rg = ( (S.g1()-D.g1())*Sa + D.g1() ) ;
      float Rb = ( (S.b1()-D.b1())*Sa + D.b1() ) ;      
      return Color::rgba1( Rr, Rg, Rb, 1.0 );
    } else {      
      float Sa = S.a1(); // normalized S alpha [0..1]
      float Da = D.a1(); // normalized D alpha [0..1]          
      float Ra = Sa + Da*(1-Sa) ; // normalized R alpha [0..1]
      float Rr = ( (S.r1()-D.r1()*Da)*Sa + D.r1()*Da ) / Ra ;
      float Rg = ( (S.g1()-D.g1()*Da)*Sa + D.g1()*Da ) / Ra ;
      float Rb = ( (S.b1()-D.b1()*Da)*Sa + D.b1()*Da ) / Ra ;
      return Color::rgba1( Rr, Rg, Rb, Ra );     
    }    
  }
  

  
  constexpr inline Color
  desaturate(double coef=0.0 ) const {
    coef = clamp(0.0,coef,1.0) ;
    double hue=0;
    double sat=0;
    double lum=0;
    this->toHSL6(hue,sat,lum);
    return Color::fromHSL6(hue, sat*coef, lum, this->a1() ) ;
  }
  
  constexpr inline Color
  adjustLuminance(double coef=0.0 ) const {
    coef = clamp(-1.0,coef,1.0) ;
    double hue=0;
    double sat=0;
    double lum=0;
    this->toHSL6(hue,sat,lum);
    if (coef < 0.0) {
      lum = lum*(coef+1) ;       // Darken
    } else {
      lum = lum+((1-lum)*coef) ; // Brighten 
    } 
    return Color::fromHSL6( hue, sat, lum, this->a1() ) ;
  }

  constexpr inline Color
  invertLuminance() const {
    double hue=0;
    double sat=0;
    double lum=0;
    this->toHSL6(hue,sat,lum);
    return Color::fromHSL6( hue, sat, 1.0-lum, this->a1() ) ;
  }

  constexpr inline Color
  setAlpha1(double alpha1) const {
    storage_type a = clamp(0.0,alpha1,1.0) * CMAX;
    return Color::makeStorage( 
      (this->getStorage() & ~MASK_A) | ( a << SHIFT_A)
      ) ;
  }
  
 
  //
  // Change the alpha using A = A + (1-A)*coef   
  //
  // With a coef value of 0.0, the original alpha is preserved. 
  // With a coef value of 1.0, the alpha is 1.0 (fully opaque). 
  //
  constexpr inline Color
  opacify(double coef) const {
    return this->setAlpha1(coef*this->a1()) ;
  }

  //
  // Provide a Color with the alpha component multiplied by coef. 
  //
  // With a coef value of 0.0, the alpha is 0.0 (fully transparent). 
  // With a coef value of 1.0, the original alpha is preserved.
  //
  constexpr inline Color
  fade(double coef) const {
    return this->setAlpha1(coef*this->a1()) ;
  }
    
};

#if 1
namespace colors
{
  static constexpr Color Transparent(0,0,0,0) ;
  static constexpr Color AliceBlue            = Color::rgba8(240,248,255);
  static constexpr Color AntiqueWhite         = Color::rgba8(250,235,215);
  static constexpr Color AntiqueWhite1        = Color::rgba8(255,239,219);
  static constexpr Color AntiqueWhite2        = Color::rgba8(238,223,204);
  static constexpr Color AntiqueWhite3        = Color::rgba8(205,192,176);
  static constexpr Color AntiqueWhite4        = Color::rgba8(139,131,120);
  static constexpr Color Aquamarine           = Color::rgba8(127,255,212);
  static constexpr Color Aquamarine1          = Color::rgba8(127,255,212);
  static constexpr Color Aquamarine2          = Color::rgba8(118,238,198);
  static constexpr Color Aquamarine3          = Color::rgba8(102,205,170);
  static constexpr Color Aquamarine4          = Color::rgba8(69,139,116);
  static constexpr Color Azure                = Color::rgba8(240,255,255);
  static constexpr Color Azure1               = Color::rgba8(240,255,255);
  static constexpr Color Azure2               = Color::rgba8(224,238,238); 
  static constexpr Color Azure3               = Color::rgba8(193,205,205);
  static constexpr Color Azure4               = Color::rgba8(131,139,139);
  static constexpr Color Beige                = Color::rgba8(245,245,220);
  static constexpr Color Bisque               = Color::rgba8(255,228,196);
  static constexpr Color Bisque1              = Color::rgba8(255,228,196);
  static constexpr Color Bisque2              = Color::rgba8(238,213,183);
  static constexpr Color Bisque3              = Color::rgba8(205,183,158);
  static constexpr Color Bisque4              = Color::rgba8(139,125,107);
  static constexpr Color Black                = Color::rgba8(0,0,0);
  static constexpr Color BlanchedAlmond       = Color::rgba8(255,235,205);
  static constexpr Color Blue                 = Color::rgba8(0,0,255);
  static constexpr Color Blue1                = Color::rgba8(0,0,255);
  static constexpr Color Blue2                = Color::rgba8(0,0,238);
  static constexpr Color Blue3                = Color::rgba8(0,0,205);
  static constexpr Color Blue4                = Color::rgba8(0,0,139);
  static constexpr Color BlueViolet           = Color::rgba8(138,43,226);
  static constexpr Color Brown                = Color::rgba8(165,42,42);
  static constexpr Color Brown1               = Color::rgba8(255,64,64);
  static constexpr Color Brown2               = Color::rgba8(238,59,59);
  static constexpr Color Brown3               = Color::rgba8(205,51,51);
  static constexpr Color Brown4               = Color::rgba8(139,35,35);
  static constexpr Color Burlywood            = Color::rgba8(222,184,135);
  static constexpr Color Burlywood1           = Color::rgba8(255,211,155);
  static constexpr Color Burlywood2           = Color::rgba8(238,197,145);
  static constexpr Color Burlywood3           = Color::rgba8(205,170,125);
  static constexpr Color Burlywood4           = Color::rgba8(139,115,85);
  static constexpr Color CadetBlue            = Color::rgba8(95,158,160);
  static constexpr Color CadetBlue1           = Color::rgba8(152,245,255);
  static constexpr Color CadetBlue2           = Color::rgba8(142,229,238);
  static constexpr Color CadetBlue3           = Color::rgba8(122,197,205);
  static constexpr Color CadetBlue4           = Color::rgba8(83,134,139);
  static constexpr Color Chartreuse           = Color::rgba8(127,255,0);
  static constexpr Color Chartreuse1          = Color::rgba8(127,255,0);
  static constexpr Color Chartreuse2          = Color::rgba8(118,238,0);
  static constexpr Color Chartreuse3          = Color::rgba8(102,205,0);
  static constexpr Color Chartreuse4          = Color::rgba8(69,139,0);
  static constexpr Color Chocolate            = Color::rgba8(210,105,30);
  static constexpr Color Chocolate1           = Color::rgba8(255,127,36);
  static constexpr Color Chocolate2           = Color::rgba8(238,118,33);
  static constexpr Color Chocolate3           = Color::rgba8(205,102,29);
  static constexpr Color Chocolate4           = Color::rgba8(139,69,19);
  static constexpr Color Coral                = Color::rgba8(255,127,80);
  static constexpr Color Coral1               = Color::rgba8(255,114,86);
  static constexpr Color Coral2               = Color::rgba8(238,106,80);
  static constexpr Color Coral3               = Color::rgba8(205,91,69);
  static constexpr Color Coral4               = Color::rgba8(139,62,47);
  static constexpr Color CornflowerBlue       = Color::rgba8(100,149,237);
  static constexpr Color Cornsilk             = Color::rgba8(255,248,220);
  static constexpr Color Cornsilk1            = Color::rgba8(255,248,220);
  static constexpr Color Cornsilk2            = Color::rgba8(238,232,205);
  static constexpr Color Cornsilk3            = Color::rgba8(205,200,177);
  static constexpr Color Cornsilk4            = Color::rgba8(139,136,120);
  static constexpr Color Cyan                 = Color::rgba8(0,255,255);
  static constexpr Color Cyan1                = Color::rgba8(0,255,255);
  static constexpr Color Cyan2                = Color::rgba8(0,238,238);
  static constexpr Color Cyan3                = Color::rgba8(0,205,205);
  static constexpr Color Cyan4                = Color::rgba8(0,139,139);
  static constexpr Color DarkBlue             = Color::rgba8(0,0,139);
  static constexpr Color DarkCyan             = Color::rgba8(0,139,139);
  static constexpr Color DarkGoldenrod        = Color::rgba8(184,134,11);
  static constexpr Color DarkGoldenrod1       = Color::rgba8(255,185,15);
  static constexpr Color DarkGoldenrod2       = Color::rgba8(238,173,14);
  static constexpr Color DarkGoldenrod3       = Color::rgba8(205,149,12);
  static constexpr Color DarkGoldenrod4       = Color::rgba8(139,101,8);
  static constexpr Color DarkGray             = Color::rgba8(169,169,169);
  static constexpr Color DarkGreen            = Color::rgba8(0,100,0);
  static constexpr Color DarkGrey             = Color::rgba8(169,169,169);
  static constexpr Color DarkKhaki            = Color::rgba8(189,183,107);
  static constexpr Color DarkMagenta          = Color::rgba8(139,0,139);
  static constexpr Color DarkOliveGreen       = Color::rgba8(85,107,47);
  static constexpr Color DarkOliveGreen1      = Color::rgba8(202,255,112);
  static constexpr Color DarkOliveGreen2      = Color::rgba8(188,238,104);
  static constexpr Color DarkOliveGreen3      = Color::rgba8(162,205,90);
  static constexpr Color DarkOliveGreen4      = Color::rgba8(110,139,61);
  static constexpr Color DarkOrange           = Color::rgba8(255,140,0);
  static constexpr Color DarkOrange1          = Color::rgba8(255,127,0);
  static constexpr Color DarkOrange2          = Color::rgba8(238,118,0);
  static constexpr Color DarkOrange3          = Color::rgba8(205,102,0);
  static constexpr Color DarkOrange4          = Color::rgba8(139,69,0);
  static constexpr Color DarkOrchid           = Color::rgba8(153,50,204);
  static constexpr Color DarkOrchid1          = Color::rgba8(191,62,255);
  static constexpr Color DarkOrchid2          = Color::rgba8(178,58,238);
  static constexpr Color DarkOrchid3          = Color::rgba8(154,50,205);
  static constexpr Color DarkOrchid4          = Color::rgba8(104,34,139);
  static constexpr Color DarkRed              = Color::rgba8(139,0,0);
  static constexpr Color DarkSalmon           = Color::rgba8(233,150,122);
  static constexpr Color DarkSeaGreen         = Color::rgba8(143,188,143);
  static constexpr Color DarkSeaGreen1        = Color::rgba8(193,255,193);
  static constexpr Color DarkSeaGreen2        = Color::rgba8(180,238,180);
  static constexpr Color DarkSeaGreen3        = Color::rgba8(155,205,155);
  static constexpr Color DarkSeaGreen4        = Color::rgba8(105,139,105);
  static constexpr Color DarkSlateBlue        = Color::rgba8(72,61,139);
  static constexpr Color DarkSlateGray        = Color::rgba8(47,79,79);
  static constexpr Color DarkSlateGray1       = Color::rgba8(151,255,255);
  static constexpr Color DarkSlateGray2       = Color::rgba8(141,238,238);
  static constexpr Color DarkSlateGray3       = Color::rgba8(121,205,205);
  static constexpr Color DarkSlateGray4       = Color::rgba8(82,139,139);
  static constexpr Color DarkSlateGrey        = Color::rgba8(47,79,79);
  static constexpr Color DarkTurquoise        = Color::rgba8(0,206,209);
  static constexpr Color DarkViolet           = Color::rgba8(148,0,211);
  static constexpr Color DebianRed            = Color::rgba8(215,7,81);
  static constexpr Color DeepPink             = Color::rgba8(255,20,147);
  static constexpr Color DeepPink1            = Color::rgba8(255,20,147);
  static constexpr Color DeepPink2            = Color::rgba8(238,18,137);
  static constexpr Color DeepPink3            = Color::rgba8(205,16,118);
  static constexpr Color DeepPink4            = Color::rgba8(139,10,80);
  static constexpr Color DeepSkyBlue          = Color::rgba8(0,191,255);
  static constexpr Color DeepSkyBlue1         = Color::rgba8(0,191,255);
  static constexpr Color DeepSkyBlue2         = Color::rgba8(0,178,238);
  static constexpr Color DeepSkyBlue3         = Color::rgba8(0,154,205);
  static constexpr Color DeepSkyBlue4         = Color::rgba8(0,104,139);
  static constexpr Color DimGray              = Color::rgba8(105,105,105);
  static constexpr Color DimGrey              = Color::rgba8(105,105,105);
  static constexpr Color DodgerBlue           = Color::rgba8(30,144,255);
  static constexpr Color DodgerBlue1          = Color::rgba8(30,144,255);
  static constexpr Color DodgerBlue2          = Color::rgba8(28,134,238);
  static constexpr Color DodgerBlue3          = Color::rgba8(24,116,205);
  static constexpr Color DodgerBlue4          = Color::rgba8(16,78,139);
  static constexpr Color Firebrick            = Color::rgba8(178,34,34);
  static constexpr Color Firebrick1           = Color::rgba8(255,48,48);
  static constexpr Color Firebrick2           = Color::rgba8(238,44,44);
  static constexpr Color Firebrick3           = Color::rgba8(205,38,38);
  static constexpr Color Firebrick4           = Color::rgba8(139,26,26);
  static constexpr Color FloralWhite          = Color::rgba8(255,250,240);
  static constexpr Color ForestGreen          = Color::rgba8(34,139,34);
  static constexpr Color Gainsboro            = Color::rgba8(220,220,220);
  static constexpr Color GhostWhite           = Color::rgba8(248,248,255);
  static constexpr Color Gold                 = Color::rgba8(255,215,0);
  static constexpr Color Gold1                = Color::rgba8(255,215,0);
  static constexpr Color Gold2                = Color::rgba8(238,201,0);
  static constexpr Color Gold3                = Color::rgba8(205,173,0);
  static constexpr Color Gold4                = Color::rgba8(139,117,0);
  static constexpr Color Goldenrod            = Color::rgba8(218,165,32);
  static constexpr Color Goldenrod1           = Color::rgba8(255,193,37);
  static constexpr Color Goldenrod2           = Color::rgba8(238,180,34);
  static constexpr Color Goldenrod3           = Color::rgba8(205,155,29);
  static constexpr Color Goldenrod4           = Color::rgba8(139,105,20);
  static constexpr Color Gray                 = Color::rgba8(190,190,190);
  static constexpr Color Gray0                = Color::rgba8(0,0,0);
  static constexpr Color Gray1                = Color::rgba8(3,3,3);
  static constexpr Color Gray10               = Color::rgba8(26,26,26);
  static constexpr Color Gray100              = Color::rgba8(255,255,255);
  static constexpr Color Gray11               = Color::rgba8(28,28,28);
  static constexpr Color Gray12               = Color::rgba8(31,31,31);
  static constexpr Color Gray13               = Color::rgba8(33,33,33);
  static constexpr Color Gray14               = Color::rgba8(36,36,36);
  static constexpr Color Gray15               = Color::rgba8(38,38,38);
  static constexpr Color Gray16               = Color::rgba8(41,41,41);
  static constexpr Color Gray17               = Color::rgba8(43,43,43);
  static constexpr Color Gray18               = Color::rgba8(46,46,46);
  static constexpr Color Gray19               = Color::rgba8(48,48,48);
  static constexpr Color Gray2                = Color::rgba8(5,5,5);
  static constexpr Color Gray20               = Color::rgba8(51,51,51);
  static constexpr Color Gray21               = Color::rgba8(54,54,54);
  static constexpr Color Gray22               = Color::rgba8(56,56,56);
  static constexpr Color Gray23               = Color::rgba8(59,59,59);
  static constexpr Color Gray24               = Color::rgba8(61,61,61);
  static constexpr Color Gray25               = Color::rgba8(64,64,64);
  static constexpr Color Gray26               = Color::rgba8(66,66,66);
  static constexpr Color Gray27               = Color::rgba8(69,69,69);
  static constexpr Color Gray28               = Color::rgba8(71,71,71);
  static constexpr Color Gray29               = Color::rgba8(74,74,74);
  static constexpr Color Gray3                = Color::rgba8(8,8,8);
  static constexpr Color Gray30               = Color::rgba8(77,77,77);
  static constexpr Color Gray31               = Color::rgba8(79,79,79);
  static constexpr Color Gray32               = Color::rgba8(82,82,82);
  static constexpr Color Gray33               = Color::rgba8(84,84,84);
  static constexpr Color Gray34               = Color::rgba8(87,87,87);
  static constexpr Color Gray35               = Color::rgba8(89,89,89);
  static constexpr Color Gray36               = Color::rgba8(92,92,92);
  static constexpr Color Gray37               = Color::rgba8(94,94,94);
  static constexpr Color Gray38               = Color::rgba8(97,97,97);
  static constexpr Color Gray39               = Color::rgba8(99,99,99);
  static constexpr Color Gray4                = Color::rgba8(10,10,10);
  static constexpr Color Gray40               = Color::rgba8(102,102,102);
  static constexpr Color Gray41               = Color::rgba8(105,105,105);
  static constexpr Color Gray42               = Color::rgba8(107,107,107);
  static constexpr Color Gray43               = Color::rgba8(110,110,110);
  static constexpr Color Gray44               = Color::rgba8(112,112,112);
  static constexpr Color Gray45               = Color::rgba8(115,115,115);
  static constexpr Color Gray46               = Color::rgba8(117,117,117);
  static constexpr Color Gray47               = Color::rgba8(120,120,120);
  static constexpr Color Gray48               = Color::rgba8(122,122,122);
  static constexpr Color Gray49               = Color::rgba8(125,125,125);
  static constexpr Color Gray5                = Color::rgba8(13,13,13);
  static constexpr Color Gray50               = Color::rgba8(127,127,127);
  static constexpr Color Gray51               = Color::rgba8(130,130,130);
  static constexpr Color Gray52               = Color::rgba8(133,133,133);
  static constexpr Color Gray53               = Color::rgba8(135,135,135);
  static constexpr Color Gray54               = Color::rgba8(138,138,138);
  static constexpr Color Gray55               = Color::rgba8(140,140,140);
  static constexpr Color Gray56               = Color::rgba8(143,143,143);
  static constexpr Color Gray57               = Color::rgba8(145,145,145);
  static constexpr Color Gray58               = Color::rgba8(148,148,148);
  static constexpr Color Gray59               = Color::rgba8(150,150,150);
  static constexpr Color Gray6                = Color::rgba8(15,15,15);
  static constexpr Color Gray60               = Color::rgba8(153,153,153);
  static constexpr Color Gray61               = Color::rgba8(156,156,156);
  static constexpr Color Gray62               = Color::rgba8(158,158,158);
  static constexpr Color Gray63               = Color::rgba8(161,161,161);
  static constexpr Color Gray64               = Color::rgba8(163,163,163);
  static constexpr Color Gray65               = Color::rgba8(166,166,166);
  static constexpr Color Gray66               = Color::rgba8(168,168,168);
  static constexpr Color Gray67               = Color::rgba8(171,171,171);
  static constexpr Color Gray68               = Color::rgba8(173,173,173);
  static constexpr Color Gray69               = Color::rgba8(176,176,176);
  static constexpr Color Gray7                = Color::rgba8(18,18,18);
  static constexpr Color Gray70               = Color::rgba8(179,179,179);
  static constexpr Color Gray71               = Color::rgba8(181,181,181);
  static constexpr Color Gray72               = Color::rgba8(184,184,184);
  static constexpr Color Gray73               = Color::rgba8(186,186,186);
  static constexpr Color Gray74               = Color::rgba8(189,189,189);
  static constexpr Color Gray75               = Color::rgba8(191,191,191);
  static constexpr Color Gray76               = Color::rgba8(194,194,194);
  static constexpr Color Gray77               = Color::rgba8(196,196,196);
  static constexpr Color Gray78               = Color::rgba8(199,199,199);
  static constexpr Color Gray79               = Color::rgba8(201,201,201);
  static constexpr Color Gray8                = Color::rgba8(20,20,20);
  static constexpr Color Gray80               = Color::rgba8(204,204,204);
  static constexpr Color Gray81               = Color::rgba8(207,207,207);
  static constexpr Color Gray82               = Color::rgba8(209,209,209);
  static constexpr Color Gray83               = Color::rgba8(212,212,212);
  static constexpr Color Gray84               = Color::rgba8(214,214,214);
  static constexpr Color Gray85               = Color::rgba8(217,217,217);
  static constexpr Color Gray86               = Color::rgba8(219,219,219);
  static constexpr Color Gray87               = Color::rgba8(222,222,222);
  static constexpr Color Gray88               = Color::rgba8(224,224,224);
  static constexpr Color Gray89               = Color::rgba8(227,227,227);
  static constexpr Color Gray9                = Color::rgba8(23,23,23);
  static constexpr Color Gray90               = Color::rgba8(229,229,229);
  static constexpr Color Gray91               = Color::rgba8(232,232,232);
  static constexpr Color Gray92               = Color::rgba8(235,235,235);
  static constexpr Color Gray93               = Color::rgba8(237,237,237);
  static constexpr Color Gray94               = Color::rgba8(240,240,240);
  static constexpr Color Gray95               = Color::rgba8(242,242,242);
  static constexpr Color Gray96               = Color::rgba8(245,245,245);
  static constexpr Color Gray97               = Color::rgba8(247,247,247);
  static constexpr Color Gray98               = Color::rgba8(250,250,250);
  static constexpr Color Gray99               = Color::rgba8(252,252,252);
  static constexpr Color Green                = Color::rgba8(0,255,0);
  static constexpr Color Green1               = Color::rgba8(0,255,0);
  static constexpr Color Green2               = Color::rgba8(0,238,0);
  static constexpr Color Green3               = Color::rgba8(0,205,0);
  static constexpr Color Green4               = Color::rgba8(0,139,0);
  static constexpr Color GreenYellow          = Color::rgba8(173,255,47);
  static constexpr Color Grey                 = Color::rgba8(190,190,190);
  static constexpr Color Grey0                = Color::rgba8(0,0,0);
  static constexpr Color Grey1                = Color::rgba8(3,3,3);
  static constexpr Color Grey10               = Color::rgba8(26,26,26);
  static constexpr Color Grey100              = Color::rgba8(255,255,255);
  static constexpr Color Grey11               = Color::rgba8(28,28,28);
  static constexpr Color Grey12               = Color::rgba8(31,31,31);
  static constexpr Color Grey13               = Color::rgba8(33,33,33);
  static constexpr Color Grey14               = Color::rgba8(36,36,36);
  static constexpr Color Grey15               = Color::rgba8(38,38,38);
  static constexpr Color Grey16               = Color::rgba8(41,41,41);
  static constexpr Color Grey17               = Color::rgba8(43,43,43);
  static constexpr Color Grey18               = Color::rgba8(46,46,46);
  static constexpr Color Grey19               = Color::rgba8(48,48,48);
  static constexpr Color Grey2                = Color::rgba8(5,5,5);
  static constexpr Color Grey20               = Color::rgba8(51,51,51);
  static constexpr Color Grey21               = Color::rgba8(54,54,54);
  static constexpr Color Grey22               = Color::rgba8(56,56,56);
  static constexpr Color Grey23               = Color::rgba8(59,59,59);
  static constexpr Color Grey24               = Color::rgba8(61,61,61);
  static constexpr Color Grey25               = Color::rgba8(64,64,64);
  static constexpr Color Grey26               = Color::rgba8(66,66,66);
  static constexpr Color Grey27               = Color::rgba8(69,69,69);
  static constexpr Color Grey28               = Color::rgba8(71,71,71);
  static constexpr Color Grey29               = Color::rgba8(74,74,74);
  static constexpr Color Grey3                = Color::rgba8(8,8,8);
  static constexpr Color Grey30               = Color::rgba8(77,77,77);
  static constexpr Color Grey31               = Color::rgba8(79,79,79);
  static constexpr Color Grey32               = Color::rgba8(82,82,82);
  static constexpr Color Grey33               = Color::rgba8(84,84,84);
  static constexpr Color Grey34               = Color::rgba8(87,87,87);
  static constexpr Color Grey35               = Color::rgba8(89,89,89);
  static constexpr Color Grey36               = Color::rgba8(92,92,92);
  static constexpr Color Grey37               = Color::rgba8(94,94,94);
  static constexpr Color Grey38               = Color::rgba8(97,97,97);
  static constexpr Color Grey39               = Color::rgba8(99,99,99);
  static constexpr Color Grey4                = Color::rgba8(10,10,10);
  static constexpr Color Grey40               = Color::rgba8(102,102,102);
  static constexpr Color Grey41               = Color::rgba8(105,105,105);
  static constexpr Color Grey42               = Color::rgba8(107,107,107);
  static constexpr Color Grey43               = Color::rgba8(110,110,110);
  static constexpr Color Grey44               = Color::rgba8(112,112,112);
  static constexpr Color Grey45               = Color::rgba8(115,115,115);
  static constexpr Color Grey46               = Color::rgba8(117,117,117);
  static constexpr Color Grey47               = Color::rgba8(120,120,120);
  static constexpr Color Grey48               = Color::rgba8(122,122,122);
  static constexpr Color Grey49               = Color::rgba8(125,125,125);
  static constexpr Color Grey5                = Color::rgba8(13,13,13);
  static constexpr Color Grey50               = Color::rgba8(127,127,127);
  static constexpr Color Grey51               = Color::rgba8(130,130,130);
  static constexpr Color Grey52               = Color::rgba8(133,133,133);
  static constexpr Color Grey53               = Color::rgba8(135,135,135);
  static constexpr Color Grey54               = Color::rgba8(138,138,138);
  static constexpr Color Grey55               = Color::rgba8(140,140,140);
  static constexpr Color Grey56               = Color::rgba8(143,143,143);
  static constexpr Color Grey57               = Color::rgba8(145,145,145);
  static constexpr Color Grey58               = Color::rgba8(148,148,148);
  static constexpr Color Grey59               = Color::rgba8(150,150,150);
  static constexpr Color Grey6                = Color::rgba8(15,15,15);
  static constexpr Color Grey60               = Color::rgba8(153,153,153);
  static constexpr Color Grey61               = Color::rgba8(156,156,156);
  static constexpr Color Grey62               = Color::rgba8(158,158,158);
  static constexpr Color Grey63               = Color::rgba8(161,161,161);
  static constexpr Color Grey64               = Color::rgba8(163,163,163);
  static constexpr Color Grey65               = Color::rgba8(166,166,166);
  static constexpr Color Grey66               = Color::rgba8(168,168,168);
  static constexpr Color Grey67               = Color::rgba8(171,171,171);
  static constexpr Color Grey68               = Color::rgba8(173,173,173);
  static constexpr Color Grey69               = Color::rgba8(176,176,176);
  static constexpr Color Grey7                = Color::rgba8(18,18,18);
  static constexpr Color Grey70               = Color::rgba8(179,179,179);
  static constexpr Color Grey71               = Color::rgba8(181,181,181);
  static constexpr Color Grey72               = Color::rgba8(184,184,184);
  static constexpr Color Grey73               = Color::rgba8(186,186,186);
  static constexpr Color Grey74               = Color::rgba8(189,189,189);
  static constexpr Color Grey75               = Color::rgba8(191,191,191);
  static constexpr Color Grey76               = Color::rgba8(194,194,194);
  static constexpr Color Grey77               = Color::rgba8(196,196,196);
  static constexpr Color Grey78               = Color::rgba8(199,199,199);
  static constexpr Color Grey79               = Color::rgba8(201,201,201);
  static constexpr Color Grey8                = Color::rgba8(20,20,20);
  static constexpr Color Grey80               = Color::rgba8(204,204,204);
  static constexpr Color Grey81               = Color::rgba8(207,207,207);
  static constexpr Color Grey82               = Color::rgba8(209,209,209);
  static constexpr Color Grey83               = Color::rgba8(212,212,212);
  static constexpr Color Grey84               = Color::rgba8(214,214,214);
  static constexpr Color Grey85               = Color::rgba8(217,217,217);
  static constexpr Color Grey86               = Color::rgba8(219,219,219);
  static constexpr Color Grey87               = Color::rgba8(222,222,222);
  static constexpr Color Grey88               = Color::rgba8(224,224,224);
  static constexpr Color Grey89               = Color::rgba8(227,227,227);
  static constexpr Color Grey9                = Color::rgba8(23,23,23);
  static constexpr Color Grey90               = Color::rgba8(229,229,229);
  static constexpr Color Grey91               = Color::rgba8(232,232,232);
  static constexpr Color Grey92               = Color::rgba8(235,235,235);
  static constexpr Color Grey93               = Color::rgba8(237,237,237);
  static constexpr Color Grey94               = Color::rgba8(240,240,240);
  static constexpr Color Grey95               = Color::rgba8(242,242,242);
  static constexpr Color Grey96               = Color::rgba8(245,245,245);
  static constexpr Color Grey97               = Color::rgba8(247,247,247);
  static constexpr Color Grey98               = Color::rgba8(250,250,250);
  static constexpr Color Grey99               = Color::rgba8(252,252,252);
  static constexpr Color Honeydew             = Color::rgba8(240,255,240);
  static constexpr Color Honeydew1            = Color::rgba8(240,255,240);
  static constexpr Color Honeydew2            = Color::rgba8(224,238,224);
  static constexpr Color Honeydew3            = Color::rgba8(193,205,193);
  static constexpr Color Honeydew4            = Color::rgba8(131,139,131);
  static constexpr Color HotPink              = Color::rgba8(255,105,180);
  static constexpr Color HotPink1             = Color::rgba8(255,110,180);
  static constexpr Color HotPink2             = Color::rgba8(238,106,167);
  static constexpr Color HotPink3             = Color::rgba8(205,96,144);
  static constexpr Color HotPink4             = Color::rgba8(139,58,98);
  static constexpr Color IndianRed            = Color::rgba8(205,92,92);
  static constexpr Color IndianRed1           = Color::rgba8(255,106,106);
  static constexpr Color IndianRed2           = Color::rgba8(238,99,99);
  static constexpr Color IndianRed3           = Color::rgba8(205,85,85);
  static constexpr Color IndianRed4           = Color::rgba8(139,58,58);
  static constexpr Color Ivory                = Color::rgba8(255,255,240);
  static constexpr Color Ivory1               = Color::rgba8(255,255,240);
  static constexpr Color Ivory2               = Color::rgba8(238,238,224);
  static constexpr Color Ivory3               = Color::rgba8(205,205,193);
  static constexpr Color Ivory4               = Color::rgba8(139,139,131);
  static constexpr Color Khaki                = Color::rgba8(240,230,140);
  static constexpr Color Khaki1               = Color::rgba8(255,246,143);
  static constexpr Color Khaki2               = Color::rgba8(238,230,133);
  static constexpr Color Khaki3               = Color::rgba8(205,198,115);
  static constexpr Color Khaki4               = Color::rgba8(139,134,78);
  static constexpr Color Lavender             = Color::rgba8(230,230,250);
  static constexpr Color LavenderBlush        = Color::rgba8(255,240,245);
  static constexpr Color LavenderBlush1       = Color::rgba8(255,240,245);
  static constexpr Color LavenderBlush2       = Color::rgba8(238,224,229);
  static constexpr Color LavenderBlush3       = Color::rgba8(205,193,197);
  static constexpr Color LavenderBlush4       = Color::rgba8(139,131,134);
  static constexpr Color LawnGreen            = Color::rgba8(124,252,0);
  static constexpr Color LemonChiffon         = Color::rgba8(255,250,205);
  static constexpr Color LemonChiffon1        = Color::rgba8(255,250,205);
  static constexpr Color LemonChiffon2        = Color::rgba8(238,233,191);
  static constexpr Color LemonChiffon3        = Color::rgba8(205,201,165);
  static constexpr Color LemonChiffon4        = Color::rgba8(139,137,112);
  static constexpr Color LightBlue            = Color::rgba8(173,216,230);
  static constexpr Color LightBlue1           = Color::rgba8(191,239,255);
  static constexpr Color LightBlue2           = Color::rgba8(178,223,238);
  static constexpr Color LightBlue3           = Color::rgba8(154,192,205);
  static constexpr Color LightBlue4           = Color::rgba8(104,131,139);
  static constexpr Color LightCoral           = Color::rgba8(240,128,128);
  static constexpr Color LightCyan            = Color::rgba8(224,255,255);
  static constexpr Color LightCyan1           = Color::rgba8(224,255,255);
  static constexpr Color LightCyan2           = Color::rgba8(209,238,238);
  static constexpr Color LightCyan3           = Color::rgba8(180,205,205);
  static constexpr Color LightCyan4           = Color::rgba8(122,139,139);
  static constexpr Color LightGoldenrod       = Color::rgba8(238,221,130);
  static constexpr Color LightGoldenrod1      = Color::rgba8(255,236,139);
  static constexpr Color LightGoldenrod2      = Color::rgba8(238,220,130);
  static constexpr Color LightGoldenrod3      = Color::rgba8(205,190,112);
  static constexpr Color LightGoldenrod4      = Color::rgba8(139,129,76);
  static constexpr Color LightGoldenrodYellow = Color::rgba8(250,250,210);
  static constexpr Color LightGray            = Color::rgba8(211,211,211);
  static constexpr Color LightGreen           = Color::rgba8(144,238,144);
  static constexpr Color LightGrey            = Color::rgba8(211,211,211);
  static constexpr Color LightPink            = Color::rgba8(255,182,193);
  static constexpr Color LightPink1           = Color::rgba8(255,174,185);
  static constexpr Color LightPink2           = Color::rgba8(238,162,173);
  static constexpr Color LightPink3           = Color::rgba8(205,140,149);
  static constexpr Color LightPink4           = Color::rgba8(139,95,101);
  static constexpr Color LightSalmon          = Color::rgba8(255,160,122);
  static constexpr Color LightSalmon1         = Color::rgba8(255,160,122);
  static constexpr Color LightSalmon2         = Color::rgba8(238,149,114);
  static constexpr Color LightSalmon3         = Color::rgba8(205,129,98);
  static constexpr Color LightSalmon4         = Color::rgba8(139,87,66);
  static constexpr Color LightSeaGreen        = Color::rgba8(32,178,170);
  static constexpr Color LightSkyBlue         = Color::rgba8(135,206,250);
  static constexpr Color LightSkyBlue1        = Color::rgba8(176,226,255);
  static constexpr Color LightSkyBlue2        = Color::rgba8(164,211,238);
  static constexpr Color LightSkyBlue3        = Color::rgba8(141,182,205);
  static constexpr Color LightSkyBlue4        = Color::rgba8(96,123,139);
  static constexpr Color LightSlateBlue       = Color::rgba8(132,112,255);
  static constexpr Color LightSlateGray       = Color::rgba8(119,136,153);
  static constexpr Color LightSlateGrey       = Color::rgba8(119,136,153);
  static constexpr Color LightSteelBlue       = Color::rgba8(176,196,222);
  static constexpr Color LightSteelBlue1      = Color::rgba8(202,225,255);
  static constexpr Color LightSteelBlue2      = Color::rgba8(188,210,238);
  static constexpr Color LightSteelBlue3      = Color::rgba8(162,181,205);
  static constexpr Color LightSteelBlue4      = Color::rgba8(110,123,139);
  static constexpr Color LightYellow          = Color::rgba8(255,255,224);
  static constexpr Color LightYellow1         = Color::rgba8(255,255,224);
  static constexpr Color LightYellow2         = Color::rgba8(238,238,209);
  static constexpr Color LightYellow3         = Color::rgba8(205,205,180);
  static constexpr Color LightYellow4         = Color::rgba8(139,139,122);
  static constexpr Color LimeGreen            = Color::rgba8(50,205,50);
  static constexpr Color Linen                = Color::rgba8(250,240,230);
  static constexpr Color Magenta              = Color::rgba8(255,0,255);
  static constexpr Color Magenta1             = Color::rgba8(255,0,255);
  static constexpr Color Magenta2             = Color::rgba8(238,0,238);
  static constexpr Color Magenta3             = Color::rgba8(205,0,205);
  static constexpr Color Magenta4             = Color::rgba8(139,0,139);
  static constexpr Color Maroon               = Color::rgba8(176,48,96);
  static constexpr Color Maroon1              = Color::rgba8(255,52,179);
  static constexpr Color Maroon2              = Color::rgba8(238,48,167);
  static constexpr Color Maroon3              = Color::rgba8(205,41,144);
  static constexpr Color Maroon4              = Color::rgba8(139,28,98);
  static constexpr Color MediumAquamarine     = Color::rgba8(102,205,170);
  static constexpr Color MediumBlue           = Color::rgba8(0,0,205);
  static constexpr Color MediumOrchid         = Color::rgba8(186,85,211);
  static constexpr Color MediumOrchid1        = Color::rgba8(224,102,255);
  static constexpr Color MediumOrchid2        = Color::rgba8(209,95,238);
  static constexpr Color MediumOrchid3        = Color::rgba8(180,82,205);
  static constexpr Color MediumOrchid4        = Color::rgba8(122,55,139);
  static constexpr Color MediumPurple         = Color::rgba8(147,112,219);
  static constexpr Color MediumPurple1        = Color::rgba8(171,130,255);
  static constexpr Color MediumPurple2        = Color::rgba8(159,121,238);
  static constexpr Color MediumPurple3        = Color::rgba8(137,104,205);
  static constexpr Color MediumPurple4        = Color::rgba8(93,71,139);
  static constexpr Color MediumSeaGreen       = Color::rgba8(60,179,113);
  static constexpr Color MediumSlateBlue      = Color::rgba8(123,104,238);
  static constexpr Color MediumSpringGreen    = Color::rgba8(0,250,154);
  static constexpr Color MediumTurquoise      = Color::rgba8(72,209,204);
  static constexpr Color MediumVioletRed      = Color::rgba8(199,21,133);
  static constexpr Color MidnightBlue         = Color::rgba8(25,25,112);
  static constexpr Color MintCream            = Color::rgba8(245,255,250);
  static constexpr Color MistyRose            = Color::rgba8(255,228,225);
  static constexpr Color MistyRose1           = Color::rgba8(255,228,225);
  static constexpr Color MistyRose2           = Color::rgba8(238,213,210);
  static constexpr Color MistyRose3           = Color::rgba8(205,183,181);
  static constexpr Color MistyRose4           = Color::rgba8(139,125,123);
  static constexpr Color Moccasin             = Color::rgba8(255,228,181);
  static constexpr Color NavajoWhite          = Color::rgba8(255,222,173);
  static constexpr Color NavajoWhite1         = Color::rgba8(255,222,173);
  static constexpr Color NavajoWhite2         = Color::rgba8(238,207,161);
  static constexpr Color NavajoWhite3         = Color::rgba8(205,179,139);
  static constexpr Color NavajoWhite4         = Color::rgba8(139,121,94);
  static constexpr Color Navy                 = Color::rgba8(0,0,128);
  static constexpr Color NavyBlue             = Color::rgba8(0,0,128);
  static constexpr Color OldLace              = Color::rgba8(253,245,230);
  static constexpr Color OliveDrab            = Color::rgba8(107,142,35);
  static constexpr Color OliveDrab1           = Color::rgba8(192,255,62);
  static constexpr Color OliveDrab2           = Color::rgba8(179,238,58);
  static constexpr Color OliveDrab3           = Color::rgba8(154,205,50);
  static constexpr Color OliveDrab4           = Color::rgba8(105,139,34);
  static constexpr Color Orange               = Color::rgba8(255,165,0);
  static constexpr Color Orange1              = Color::rgba8(255,165,0);
  static constexpr Color Orange2              = Color::rgba8(238,154,0);
  static constexpr Color Orange3              = Color::rgba8(205,133,0);
  static constexpr Color Orange4              = Color::rgba8(139,90,0);
  static constexpr Color OrangeRed            = Color::rgba8(255,69,0);
  static constexpr Color OrangeRed1           = Color::rgba8(255,69,0);
  static constexpr Color OrangeRed2           = Color::rgba8(238,64,0);
  static constexpr Color OrangeRed3           = Color::rgba8(205,55,0);
  static constexpr Color OrangeRed4           = Color::rgba8(139,37,0);
  static constexpr Color Orchid               = Color::rgba8(218,112,214);
  static constexpr Color Orchid1              = Color::rgba8(255,131,250);
  static constexpr Color Orchid2              = Color::rgba8(238,122,233);
  static constexpr Color Orchid3              = Color::rgba8(205,105,201);
  static constexpr Color Orchid4              = Color::rgba8(139,71,137);
  static constexpr Color PaleGoldenrod        = Color::rgba8(238,232,170);
  static constexpr Color PaleGreen            = Color::rgba8(152,251,152);
  static constexpr Color PaleGreen1           = Color::rgba8(154,255,154);
  static constexpr Color PaleGreen2           = Color::rgba8(144,238,144);
  static constexpr Color PaleGreen3           = Color::rgba8(124,205,124);
  static constexpr Color PaleGreen4           = Color::rgba8(84,139,84);
  static constexpr Color PaleTurquoise        = Color::rgba8(175,238,238);
  static constexpr Color PaleTurquoise1       = Color::rgba8(187,255,255);
  static constexpr Color PaleTurquoise2       = Color::rgba8(174,238,238);
  static constexpr Color PaleTurquoise3       = Color::rgba8(150,205,205);
  static constexpr Color PaleTurquoise4       = Color::rgba8(102,139,139);
  static constexpr Color PaleVioletRed        = Color::rgba8(219,112,147);
  static constexpr Color PaleVioletRed1       = Color::rgba8(255,130,171);
  static constexpr Color PaleVioletRed2       = Color::rgba8(238,121,159);
  static constexpr Color PaleVioletRed3       = Color::rgba8(205,104,137);
  static constexpr Color PaleVioletRed4       = Color::rgba8(139,71,93);
  static constexpr Color PapayaWhip           = Color::rgba8(255,239,213);
  static constexpr Color PeachPuff            = Color::rgba8(255,218,185);
  static constexpr Color PeachPuff1           = Color::rgba8(255,218,185);
  static constexpr Color PeachPuff2           = Color::rgba8(238,203,173);
  static constexpr Color PeachPuff3           = Color::rgba8(205,175,149);
  static constexpr Color PeachPuff4           = Color::rgba8(139,119,101);
  static constexpr Color Peru                 = Color::rgba8(205,133,63);
  static constexpr Color Pink                 = Color::rgba8(255,192,203);
  static constexpr Color Pink1                = Color::rgba8(255,181,197);
  static constexpr Color Pink2                = Color::rgba8(238,169,184);
  static constexpr Color Pink3                = Color::rgba8(205,145,158);
  static constexpr Color Pink4                = Color::rgba8(139,99,108);
  static constexpr Color Plum                 = Color::rgba8(221,160,221);
  static constexpr Color Plum1                = Color::rgba8(255,187,255);
  static constexpr Color Plum2                = Color::rgba8(238,174,238);
  static constexpr Color Plum3                = Color::rgba8(205,150,205);
  static constexpr Color Plum4                = Color::rgba8(139,102,139);
  static constexpr Color PowderBlue           = Color::rgba8(176,224,230);
  static constexpr Color Purple               = Color::rgba8(160,32,240);
  static constexpr Color Purple1              = Color::rgba8(155,48,255);
  static constexpr Color Purple2              = Color::rgba8(145,44,238);
  static constexpr Color Purple3              = Color::rgba8(125,38,205);
  static constexpr Color Purple4              = Color::rgba8(85,26,139);
  static constexpr Color Red                  = Color::rgba8(255,0,0);
  static constexpr Color Red1                 = Color::rgba8(255,0,0);
  static constexpr Color Red2                 = Color::rgba8(238,0,0);
  static constexpr Color Red3                 = Color::rgba8(205,0,0);
  static constexpr Color Red4                 = Color::rgba8(139,0,0);
  static constexpr Color RosyBrown            = Color::rgba8(188,143,143);
  static constexpr Color RosyBrown1           = Color::rgba8(255,193,193);
  static constexpr Color RosyBrown2           = Color::rgba8(238,180,180);
  static constexpr Color RosyBrown3           = Color::rgba8(205,155,155);
  static constexpr Color RosyBrown4           = Color::rgba8(139,105,105);
  static constexpr Color RoyalBlue            = Color::rgba8(65,105,225);
  static constexpr Color RoyalBlue1           = Color::rgba8(72,118,255);
  static constexpr Color RoyalBlue2           = Color::rgba8(67,110,238);
  static constexpr Color RoyalBlue3           = Color::rgba8(58,95,205);
  static constexpr Color RoyalBlue4           = Color::rgba8(39,64,139);
  static constexpr Color SaddleBrown          = Color::rgba8(139,69,19);
  static constexpr Color Salmon               = Color::rgba8(250,128,114);
  static constexpr Color Salmon1              = Color::rgba8(255,140,105);
  static constexpr Color Salmon2              = Color::rgba8(238,130,98);
  static constexpr Color Salmon3              = Color::rgba8(205,112,84);
  static constexpr Color Salmon4              = Color::rgba8(139,76,57);
  static constexpr Color SandyBrown           = Color::rgba8(244,164,96);
  static constexpr Color SeaGreen             = Color::rgba8(46,139,87);
  static constexpr Color SeaGreen1            = Color::rgba8(84,255,159);
  static constexpr Color SeaGreen2            = Color::rgba8(78,238,148);
  static constexpr Color SeaGreen3            = Color::rgba8(67,205,128);
  static constexpr Color SeaGreen4            = Color::rgba8(46,139,87);
  static constexpr Color Seashell             = Color::rgba8(255,245,238);
  static constexpr Color Seashell1            = Color::rgba8(255,245,238);
  static constexpr Color Seashell2            = Color::rgba8(238,229,222);
  static constexpr Color Seashell3            = Color::rgba8(205,197,191);
  static constexpr Color Seashell4            = Color::rgba8(139,134,130);
  static constexpr Color Sienna               = Color::rgba8(160,82,45);
  static constexpr Color Sienna1              = Color::rgba8(255,130,71);
  static constexpr Color Sienna2              = Color::rgba8(238,121,66);
  static constexpr Color Sienna3              = Color::rgba8(205,104,57);
  static constexpr Color Sienna4              = Color::rgba8(139,71,38);
  static constexpr Color SkyBlue              = Color::rgba8(135,206,235);
  static constexpr Color SkyBlue1             = Color::rgba8(135,206,255);
  static constexpr Color SkyBlue2             = Color::rgba8(126,192,238);
  static constexpr Color SkyBlue3             = Color::rgba8(108,166,205);
  static constexpr Color SkyBlue4             = Color::rgba8(74,112,139);
  static constexpr Color SlateBlue            = Color::rgba8(106,90,205);
  static constexpr Color SlateBlue1           = Color::rgba8(131,111,255);
  static constexpr Color SlateBlue2           = Color::rgba8(122,103,238);
  static constexpr Color SlateBlue3           = Color::rgba8(105,89,205);
  static constexpr Color SlateBlue4           = Color::rgba8(71,60,139);
  static constexpr Color SlateGray            = Color::rgba8(112,128,144);
  static constexpr Color SlateGray1           = Color::rgba8(198,226,255);
  static constexpr Color SlateGray2           = Color::rgba8(185,211,238);
  static constexpr Color SlateGray3           = Color::rgba8(159,182,205);
  static constexpr Color SlateGray4           = Color::rgba8(108,123,139);
  static constexpr Color SlateGrey            = Color::rgba8(112,128,144);
  static constexpr Color Snow                 = Color::rgba8(255,250,250);
  static constexpr Color Snow1                = Color::rgba8(255,250,250);
  static constexpr Color Snow2                = Color::rgba8(238,233,233);
  static constexpr Color Snow3                = Color::rgba8(205,201,201);
  static constexpr Color Snow4                = Color::rgba8(139,137,137);
  static constexpr Color SpringGreen          = Color::rgba8(0,255,127);
  static constexpr Color SpringGreen1         = Color::rgba8(0,255,127);
  static constexpr Color SpringGreen2         = Color::rgba8(0,238,118);
  static constexpr Color SpringGreen3         = Color::rgba8(0,205,102);
  static constexpr Color SpringGreen4         = Color::rgba8(0,139,69);
  static constexpr Color SteelBlue            = Color::rgba8(70,130,180);
  static constexpr Color SteelBlue1           = Color::rgba8(99,184,255);
  static constexpr Color SteelBlue2           = Color::rgba8(92,172,238);
  static constexpr Color SteelBlue3           = Color::rgba8(79,148,205);
  static constexpr Color SteelBlue4           = Color::rgba8(54,100,139);
  static constexpr Color Tan                  = Color::rgba8(210,180,140);
  static constexpr Color Tan1                 = Color::rgba8(255,165,79);
  static constexpr Color Tan2                 = Color::rgba8(238,154,73);
  static constexpr Color Tan3                 = Color::rgba8(205,133,63);
  static constexpr Color Tan4                 = Color::rgba8(139,90,43);
  static constexpr Color Thistle              = Color::rgba8(216,191,216);
  static constexpr Color Thistle1             = Color::rgba8(255,225,255);
  static constexpr Color Thistle2             = Color::rgba8(238,210,238);
  static constexpr Color Thistle3             = Color::rgba8(205,181,205);
  static constexpr Color Thistle4             = Color::rgba8(139,123,139);
  static constexpr Color Tomato               = Color::rgba8(255,99,71);
  static constexpr Color Tomato1              = Color::rgba8(255,99,71);
  static constexpr Color Tomato2              = Color::rgba8(238,92,66);
  static constexpr Color Tomato3              = Color::rgba8(205,79,57);
  static constexpr Color Tomato4              = Color::rgba8(139,54,38);
  static constexpr Color Turquoise            = Color::rgba8(64,224,208);
  static constexpr Color Turquoise1           = Color::rgba8(0,245,255);
  static constexpr Color Turquoise2           = Color::rgba8(0,229,238);
  static constexpr Color Turquoise3           = Color::rgba8(0,197,205);
  static constexpr Color Turquoise4           = Color::rgba8(0,134,139);
  static constexpr Color Violet               = Color::rgba8(238,130,238);
  static constexpr Color VioletRed            = Color::rgba8(208,32,144);
  static constexpr Color VioletRed1           = Color::rgba8(255,62,150);
  static constexpr Color VioletRed2           = Color::rgba8(238,58,140);
  static constexpr Color VioletRed3           = Color::rgba8(205,50,120);
  static constexpr Color VioletRed4           = Color::rgba8(139,34,82);
  static constexpr Color Wheat                = Color::rgba8(245,222,179);
  static constexpr Color Wheat1               = Color::rgba8(255,231,186);
  static constexpr Color Wheat2               = Color::rgba8(238,216,174);
  static constexpr Color Wheat3               = Color::rgba8(205,186,150);
  static constexpr Color Wheat4               = Color::rgba8(139,126,102);
  static constexpr Color White                = Color::rgba8(255,255,255);
  static constexpr Color WhiteSmoke           = Color::rgba8(245,245,245);
  static constexpr Color Yellow               = Color::rgba8(255,255,0);
  static constexpr Color Yellow1              = Color::rgba8(255,255,0);
  static constexpr Color Yellow2              = Color::rgba8(238,238,0);
  static constexpr Color Yellow3              = Color::rgba8(205,205,0);
  static constexpr Color Yellow4              = Color::rgba8(139,139,0);
  static constexpr Color YellowGreen          = Color::rgba8(154,205,50);
    
};

#endif


//
// Operator || provides the Porter-Duff SRC_OVER operation. 
//
inline Color
operator||(const Color &S, const Color &D)
{
  return S.over(D);
}



// Use operator * to multiply the alpha component
// of a color.
//
// For example "Color(10,20,30,44) * 0.25" is equivalent
// to "Color(10,20,30,11)".
// 
inline Color
operator*(const Color &in, float coef)
{
  return in.fade(coef);
}

inline Color
operator*(float coef, const Color &in)
{
  return in.fade(coef);
}


// Use operator % to set the alpha of a color.
//
// For example "Color(45,23,11) % 0.25" is equivalent
// to "Color(45,23,11,25)".
//
inline Color operator%(const Color &in, float alpha1)
{
  return in.setAlpha1(alpha1);
}

// Use operator % to set the alpha of a color.
//
// For example "Color(45,23,11) % 0.25" is equivalent
// to "Color(45,23,11,25)".
//
inline Color operator%(float alpha1, const Color &in)
{
  return in.setAlpha1(alpha1);
}


// Operator / provides desaturation

inline Color operator/(const Color &in, float coef)
{
  return in.desaturate(coef);
}

// Operators + and - provide luminance adjustment on colors
//
// The floating-point coefficient must be between -1.0 and +1.0
//

inline Color operator+(const Color &in, float coef)
{
  return in.adjustLuminance(coef);
}

inline Color operator+(float coef, const Color &in)
{
  return in.adjustLuminance(coef);
}

inline Color operator-(const Color &in, float coef)
{
  return in.adjustLuminance(-coef);
}

// Invert Luminance but keep Hue, Saturation and Alpha.

inline Color operator!(const Color &in)
{
  return in.invertLuminance();
}

// Invert R, G & B components but keep Alpha.

inline Color operator~(const Color &in)
{
  return in;
  // TODO: return in.invert();
}

std::ostream & operator<<( std::ostream &s, const Color c) {  
  s << "#" << std::hex << std::setfill('0') 
    << std::setw(8) << c.argb8() 
    << std::setfill(' ');
  return s;
}

#endif
