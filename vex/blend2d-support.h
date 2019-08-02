
namespace bl
{
  
  const char * error_str(int code)
  {
    static char buffer[30] ;
    switch(code) {
    case BL_SUCCESS:                            return "BL_SUCCESS";
    case BL_ERROR_OUT_OF_MEMORY:                return "BL_ERROR_OUT_OF_MEMORY";
    case BL_ERROR_INVALID_VALUE:                return "BL_ERROR_INVALID_VALUE";
    case BL_ERROR_INVALID_STATE:                return "BL_ERROR_INVALID_STATE";
    case BL_ERROR_INVALID_HANDLE:               return "BL_ERROR_INVALID_HANDLE";
    case BL_ERROR_VALUE_TOO_LARGE:              return "BL_ERROR_VALUE_TOO_LARGE";
    case BL_ERROR_NOT_INITIALIZED:              return "BL_ERROR_NOT_INITIALIZED";
    case BL_ERROR_NOT_IMPLEMENTED:              return "BL_ERROR_NOT_IMPLEMENTED";
    case BL_ERROR_NOT_PERMITTED:                return "BL_ERROR_NOT_PERMITTED";
    case BL_ERROR_IO:                           return "BL_ERROR_IO";
    case BL_ERROR_BUSY:                         return "BL_ERROR_BUSY";
    case BL_ERROR_INTERRUPTED:                  return "BL_ERROR_INTERRUPTED";
    case BL_ERROR_TRY_AGAIN:                    return "BL_ERROR_TRY_AGAIN";
    case BL_ERROR_TIMED_OUT:                    return "BL_ERROR_TIMED_OUT";
    case BL_ERROR_BROKEN_PIPE:                  return "BL_ERROR_BROKEN_PIPE";
    case BL_ERROR_INVALID_SEEK:                 return "BL_ERROR_INVALID_SEEK";
    case BL_ERROR_SYMLINK_LOOP:                 return "BL_ERROR_SYMLINK_LOOP";
    case BL_ERROR_FILE_TOO_LARGE:               return "BL_ERROR_FILE_TOO_LARGE";
    case BL_ERROR_ALREADY_EXISTS:               return "BL_ERROR_ALREADY_EXISTS";
    case BL_ERROR_ACCESS_DENIED:                return "BL_ERROR_ACCESS_DENIED";
    case BL_ERROR_MEDIA_CHANGED:                return "BL_ERROR_MEDIA_CHANGED";
    case BL_ERROR_READ_ONLY_FS:                 return "BL_ERROR_READ_ONLY_FS";
    case BL_ERROR_NO_DEVICE:                    return "BL_ERROR_NO_DEVICE";
    case BL_ERROR_NO_ENTRY:                     return "BL_ERROR_NO_ENTRY";
    case BL_ERROR_NO_MEDIA:                     return "BL_ERROR_NO_MEDIA";
    case BL_ERROR_NO_MORE_DATA:                 return "BL_ERROR_NO_MORE_DATA";
    case BL_ERROR_NO_MORE_FILES:                return "BL_ERROR_NO_MORE_FILES";
    case BL_ERROR_NO_SPACE_LEFT:                return "BL_ERROR_NO_SPACE_LEFT";
    case BL_ERROR_NOT_EMPTY:                    return "BL_ERROR_NOT_EMPTY";
    case BL_ERROR_NOT_FILE:                     return "BL_ERROR_NOT_FILE";
    case BL_ERROR_NOT_DIRECTORY:                return "BL_ERROR_NOT_DIRECTORY";
    case BL_ERROR_NOT_SAME_DEVICE:              return "BL_ERROR_NOT_SAME_DEVICE";
    case BL_ERROR_NOT_BLOCK_DEVICE:             return "BL_ERROR_NOT_BLOCK_DEVICE";
    case BL_ERROR_INVALID_FILE_NAME:            return "BL_ERROR_INVALID_FILE_NAME";
    case BL_ERROR_FILE_NAME_TOO_LONG:           return "BL_ERROR_FILE_NAME_TOO_LONG";
    case BL_ERROR_TOO_MANY_OPEN_FILES:          return "BL_ERROR_TOO_MANY_OPEN_FILES";
    case BL_ERROR_TOO_MANY_OPEN_FILES_BY_OS:    return "BL_ERROR_TOO_MANY_OPEN_FILES_BY_OS";
    case BL_ERROR_TOO_MANY_LINKS:               return "BL_ERROR_TOO_MANY_LINKS";
    case BL_ERROR_TOO_MANY_THREADS:             return "BL_ERROR_TOO_MANY_THREADS";
    case BL_ERROR_FILE_EMPTY:                   return "BL_ERROR_FILE_EMPTY";
    case BL_ERROR_OPEN_FAILED:                  return "BL_ERROR_OPEN_FAILED";
    case BL_ERROR_NOT_ROOT_DEVICE:              return "BL_ERROR_NOT_ROOT_DEVICE";
    case BL_ERROR_UNKNOWN_SYSTEM_ERROR:         return "BL_ERROR_UNKNOWN_SYSTEM_ERROR";
    case BL_ERROR_INVALID_ALIGNMENT:            return "BL_ERROR_INVALID_ALIGNMENT";
    case BL_ERROR_INVALID_SIGNATURE:            return "BL_ERROR_INVALID_SIGNATURE";
    case BL_ERROR_INVALID_DATA:                 return "BL_ERROR_INVALID_DATA";
    case BL_ERROR_INVALID_STRING:               return "BL_ERROR_INVALID_STRING";
    case BL_ERROR_DATA_TRUNCATED:               return "BL_ERROR_DATA_TRUNCATED";
    case BL_ERROR_DATA_TOO_LARGE:               return "BL_ERROR_DATA_TOO_LARGE";
    case BL_ERROR_DECOMPRESSION_FAILED:         return "BL_ERROR_DECOMPRESSION_FAILED";
    case BL_ERROR_INVALID_GEOMETRY:             return "BL_ERROR_INVALID_GEOMETRY";
    case BL_ERROR_NO_MATCHING_VERTEX:           return "BL_ERROR_NO_MATCHING_VERTEX";
    case BL_ERROR_NO_MATCHING_COOKIE:           return "BL_ERROR_NO_MATCHING_COOKIE";
    case BL_ERROR_NO_STATES_TO_RESTORE:         return "BL_ERROR_NO_STATES_TO_RESTORE";
    case BL_ERROR_IMAGE_TOO_LARGE:              return "BL_ERROR_IMAGE_TOO_LARGE";
    case BL_ERROR_IMAGE_NO_MATCHING_CODEC:      return "BL_ERROR_IMAGE_NO_MATCHING_CODEC";
    case BL_ERROR_IMAGE_UNKNOWN_FILE_FORMAT:    return "BL_ERROR_IMAGE_UNKNOWN_FILE_FORMAT";
    case BL_ERROR_IMAGE_DECODER_NOT_PROVIDED:   return "BL_ERROR_IMAGE_DECODER_NOT_PROVIDED";
    case BL_ERROR_IMAGE_ENCODER_NOT_PROVIDED:   return "BL_ERROR_IMAGE_ENCODER_NOT_PROVIDED";
    case BL_ERROR_PNG_MULTIPLE_IHDR:            return "BL_ERROR_PNG_MULTIPLE_IHDR";
    case BL_ERROR_PNG_INVALID_IDAT:             return "BL_ERROR_PNG_INVALID_IDAT";
    case BL_ERROR_PNG_INVALID_IEND:             return "BL_ERROR_PNG_INVALID_IEND";
    case BL_ERROR_PNG_INVALID_PLTE:             return "BL_ERROR_PNG_INVALID_PLTE";
    case BL_ERROR_PNG_INVALID_TRNS:             return "BL_ERROR_PNG_INVALID_TRNS";
    case BL_ERROR_PNG_INVALID_FILTER:           return "BL_ERROR_PNG_INVALID_FILTER";
    case BL_ERROR_JPEG_UNSUPPORTED_FEATURE:     return "BL_ERROR_JPEG_UNSUPPORTED_FEATURE";
    case BL_ERROR_JPEG_INVALID_SOS:             return "BL_ERROR_JPEG_INVALID_SOS";
    case BL_ERROR_JPEG_INVALID_SOF:             return "BL_ERROR_JPEG_INVALID_SOF";
    case BL_ERROR_JPEG_MULTIPLE_SOF:            return "BL_ERROR_JPEG_MULTIPLE_SOF";
    case BL_ERROR_JPEG_UNSUPPORTED_SOF:         return "BL_ERROR_JPEG_UNSUPPORTED_SOF";
    case BL_ERROR_FONT_NO_CHARACTER_MAPPING:    return "BL_ERROR_FONT_NO_CHARACTER_MAPPING";
    case BL_ERROR_FONT_MISSING_IMPORTANT_TABLE: return "BL_ERROR_FONT_MISSING_IMPORTANT_TABLE";
    case BL_ERROR_FONT_FEATURE_NOT_AVAILABLE:   return "BL_ERROR_FONT_FEATURE_NOT_AVAILABLE";
    case BL_ERROR_FONT_CFF_INVALID_DATA:        return "BL_ERROR_FONT_CFF_INVALID_DATA";
    case BL_ERROR_FONT_PROGRAM_TERMINATED:      return "BL_ERROR_FONT_PROGRAM_TERMINATED";
    case BL_ERROR_INVALID_GLYPH:                return "BL_ERROR_INVALID_GLYPH";
    default:
      sprintf(buffer,"BL_ERROR(%d)",code) ;
      return buffer;
    }
  }
  

  inline void check(int code, const char *filename=0, int line=0)
  {
    if (code!=BL_SUCCESS) {
      if (filename && line)
        std::cerr << filename << ":" << line << ": Fatal Error: " << bl::error_str(code) << "\n";
      else
        std::cerr << "Error [Blend2D] " << bl::error_str(code) << "\n";
      abort();
    }
  }

  class BLResultFatal
  {
  private:
    const char *file; 
    int line;
  public:
    
    inline BLResultFatal & operator<<(BLResult code) {
      if (code!=BL_SUCCESS) {
        if (line)
          std::cerr << file << ":" << line << ": Fatal Error: " << bl::error_str(code) << "\n";
        else
          std::cerr << "Error [Blend2D] " << bl::error_str(code) << "\n";
        abort();
      }
      return *this;
    }
    
    inline BLResultFatal & operator=(BLResult code) {
      *this << code ;
      return *this;
    }
    
    
    inline BLResultFatal & location(const char *f, int ln) {
      file = f ;
      line = ln ;
      return *this ;
    }
  } ;
  
  inline BLResultFatal & operator||(BLResult code, BLResultFatal &fatal) {
    fatal << code ;
    return fatal; 
  }

  
  // bl_fatal is basically bl_fatal_impl but with
  // attached location information. 
#define bl_fatal bl_fatal_impl.location(__FILE__,__LINE__)
  
// Provide the actual implementation of 'bl_fatal'.
// That macro should be used once in any function using bl_fatal.
// Note: Using a global declaration is technically possible
//       but suboptimal since the location may be incorrect
//       if 'bl_fatal' was also used by a callee. A global
//       would also not be thread safe.  
#define BL_FATAL_DECLARE bl::BLResultFatal bl_fatal_impl ;


// Check an error code (and fail if different from BL_SUCCESS)
#define BL_CHECK(expr) bl::check(expr,__FILE__,__LINE__)

  template <typename T>
  constexpr inline T clamp( T vmin, T v, T vmax) {
    return std::max(vmin,std::min(v,vmax)) ;
  }

  
  inline BLRect toRect(const BLBox &box) {
    // TODO: Are w and h supposed to be positive?
    return BLRect( box.x0,
                   box.y0,
                   box.x1 - box.x0,
                   box.y1 - box.y0 );
  }
  
  // Shrink a BLRect by the specified amount
  inline BLRect shrink( const BLRect & rect , double dx, double dy ) {
    return BLRect( rect.x+dx , rect.y+dy , rect.w-2*dx , rect.h-2*dy) ;
  }
  
  // Grow a BLRect by the specified amounts.
  inline BLRect grow( const BLRect & rect , double dx, double dy ) {
    return BLRect( rect.x-dx , rect.y-dy , rect.w+2*dx , rect.h+2*dy) ;
  }


  // A simple helper to save the context and insure that it is properly
  // restored  when leaving the current scope.
  // 
  // Example:
  //    void my_draw_rect(BLContext &ctx, const BLRect &rect, bool stroke) {
  //       bl::ContextRestorer rest(ctx);  // ctx.save() happens here
  //       ctx.setFillStyle(fill_color);
  //       ctx.fillRect(rect);
  //       if (!stroke)
  //          return;   // ctx.restore() happens here
  //       ctx.setStrokeStyle(stroke_color);
  //       ctx.strokeRect(rect);
  //       // ctx.restore() happens here
  //   } 
  //
  // TODO: Provide a similar 'OptContextRestorer' that
  //       does the save/restore conditionally. 
  //

  class ContextRestorer {
  private:
    BLContext &ctx ;
  public:

    ContextRestorer() =delete ;

    ContextRestorer(const ContextRestorer &) =delete ;

    inline ContextRestorer(BLContext &c) : ctx(c) {
        ctx.save() ;
    }    

    inline ~ContextRestorer() {
        ctx.restore() ;
    }
  };

  //
  // Similar to OptContextRestorer but the context save/restore is optional.
  // 
  // The context save happens the first time the save() member is called.
  //
  // The context restore happens during the destructor but only if save()
  // was called at least once. 
  //
  // Example:
  //    void my_draw_rect(BLContext &ctx, const BLRect &rect, unsigned op) {
  //       bl::OptContextRestorer ocr(ctx);  
  //       if (op & SET_FILL_COLOR) {
  //          ocr.save();
  //          ctx.setFillStyle(fill_color);
  //       }
  //       if (op & SET_STROKE_COLOR) {
  //         ocr.save();
  //         ctx.setStrokeStyle(stroke_color);
  //       } 
  //       ctx.fillRect(rect);
  //       ctx.strokeRect(rect);
  //   } 
  //  
  class OptContextRestorer {
  private:
    BLContext &ctx ;
    bool saved;
  public:

    OptContextRestorer() =delete ;

    OptContextRestorer(const ContextRestorer &) =delete ;

    inline OptContextRestorer(BLContext &c) : ctx(c) , saved(false) {
    }   
    
    inline ~OptContextRestorer() {
      if (saved)
        ctx.restore() ;
    }
    
    // Save the context at first call.
    void save() {
      if (!saved) {
        saved=true;
        ctx.save();
      }
    }
  } ; 

  
}; // of namespace bl


std::ostream &operator<<( std::ostream &s, const BLBox &b) {
  s << "[x0=" << b.x0
    << ",y0=" << b.y0
    << ",x1=" << b.x1
    << ",y1=" << b.y1
    << "]";
  return s;
}

std::ostream &operator<<( std::ostream &s, const BLRect &b) {
  s << "[x=" << b.x
    << ",y=" << b.y
    << ",w=" << b.w
    << ",h=" << b.h
    << "]";
  return s;
}
