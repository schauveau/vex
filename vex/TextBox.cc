
#include "TextBox.h"

#include <map>
#include <string>
#include <fontconfig/fontconfig.h>

namespace btb
{

  static FcConfig * fc_config = NULL ;
  
  void
  createFaceFromFcPattern(BLFontFace &face, std::string p, const char32_t *charset) {   
    FcPattern * fc_pattern = NULL;
    FcPattern * fc_font = NULL;
    FcResult    result;
      
    if ( !fc_config ) {
      fc_config = FcInitLoadConfigAndFonts();    
    } 
    
    fc_pattern = FcNameParse( (const FcChar8*) p.c_str() );
    if (!fc_pattern) {
      std::cerr << "Failed to parse font pattern: '" << p << "'\n"; 
      exit(1);
    }

    FcConfigSubstitute(fc_config, fc_pattern, FcMatchPattern);
    FcDefaultSubstitute(fc_pattern);
    
    FcFontSet * fc_fontset = FcFontSort(fc_config, fc_pattern, FcFalse, 0, &result);
    if (!fc_fontset || fc_fontset->nfont == 0) {
      std::cerr << "No fonts installed on the system\n";
      exit(1);
    }

#if 0
    // Dump the first alternatives 
    std::cerr << "###### " << p << "\n";
    for (int k=0;k<10;k++) {
      if (k >= fc_fontset->nfont)
        break ;
      FcChar8 *filename = NULL;
      fc_font = FcFontRenderPrepare(fc_config, fc_pattern, fc_fontset->fonts[k]);
      FcPatternGetString(fc_font, FC_FILE, 0, &filename);
      std::cerr << "#    " << ((const char*)filename)  << "\n";
    }
    if (k==10) {
      std::cerr << "#    ...\n";
    }
#endif
    
    // Blend2d only support a subset of the possible font
    // formats that can be returned by fontconfig.
    // So try all matches until one can be loaded. 
    for (int i=0; i<fc_fontset->nfont ; i++) {
      FcChar8 *filename = NULL;
      int index = 0;
      fc_font = FcFontRenderPrepare(fc_config, fc_pattern, fc_fontset->fonts[i]);
      
      if (FcPatternGetString(fc_font, FC_FILE, 0, &filename)  != FcResultMatch)
        continue;
      if (FcPatternGetInteger(fc_font, FC_INDEX, 0, &index)  != FcResultMatch)
        continue;
      
      BLFontLoader loader;
      uint32_t flags =  BL_FILE_READ_MMAP_ENABLED | BL_FILE_READ_MMAP_AVOID_SMALL;
      if ( loader.createFromFile((const char*)filename, flags) != BL_SUCCESS ) {        
        std::cerr << "INFO: Failed to load font file " << filename << "\n";
        continue;
      }
      
      if ( face.createFromLoader(loader, 0) != BL_SUCCESS ) {
        std::cerr << "INFO: Failed to create face from " << filename << "\n";
        continue;
      }

      // Success

      std::cerr << "INFO: Using `" << filename << "` for font pattern '" << p << "'\n";
      
      if (fc_fontset)
        FcFontSetSortDestroy(fc_fontset);
      if (fc_pattern)
        FcPatternDestroy(fc_pattern);
      if (fc_font)
        FcPatternDestroy(fc_font);
      return;
      
    }

    std::cerr << "Failed to get font from pattern: '" << p << "'\n"; 
    exit(1);
     
  }

  // A global font map
  std::map<std::string,BLFont> font_aliases;

  void
  registerFont(std::string alias, const BLFont &font)
  {
    font_aliases[alias] = font; 
  }

  void
  registerFont(std::string alias, std::string other)
  {
    font_aliases[alias] = getFont(other,true); 
  }
  
  void
  registerFont(std::string alias, const BLFontFace &face, float size)
  {
    font_aliases[alias].createFromFace(face,size); 
  }
  
  const BLFont &
  getFont(std::string alias, bool strict)
  {
    static const std::string fallback("fallback"); 
    auto it = font_aliases.find(alias) ;
    if (it != font_aliases.end()) {
      return it->second;
    }
    if (!strict && alias!=fallback) {
      it = font_aliases.find(fallback) ;
      if (it != font_aliases.end()) {
        std::cerr << "Warning: Using fallback font instead of '" << alias << "'\n";
        return it->second;
      }
    }
    std::cerr << "Unknown font alias '" << alias << "'\n";
    exit(1);
  }
    
};
