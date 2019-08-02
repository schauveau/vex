#ifndef VEX_TEXT_BOX_H
#define VEX_TEXT_BOX_H

#include <vector>
#include <list>
#include <string>

#include <iostream>

#include "parse_utils.h"    

namespace btb
{
  // Register a Blend2D font to the given alias.
  void registerFont(std::string alias, const BLFont &font);

  // Create and register a Blend2D font to the given alias. 
  void registerFont(std::string alias, const BLFontFace &face, float size);

  // Register an alias for a previously registered font.
  //
  // So this is an alias for
  //   registerFont( alias, getFont(other) )
  //
  void registerFont(std::string alias, std::string other);


  // Create a face from a FontConfig pattern.
  //
  // If a charset is specified then the pattern is extended to match all codepoints
  // found in charset.
  //
  // This function can fail (with an internal fatal error) but in most cases 
  // it shall return    shall be noted that this function is not really expected to fail.
  // 
  //
  // Example:
  //
  //    // Get a Monospace Bold face.
  //    BLFontFace face_mono;
  //    createFaceFromFcPattern(face_mono, "mono:style=bold") ;
  //
  //    // Get a face that supports some symbols. 
  //    BLFontFace face_symb;
  //    createFaceFromFcPattern(face_mono, ":") ;
  //
  // See also man fc-match(1)
  //
  //
  void createFaceFromFcPattern(BLFontFace &face, std::string pattern, const char32_t *charset=0) ;
  
  
  // Get the font previously registered to the the given alias.
  // If the alias is not defined then if strict is false then the
  // "fallback" font will be used instead. Otherwise a fatal error
  // occurs.
  const BLFont & getFont(std::string alias, bool strict=false);
  
  enum align_t : uint32_t 
  {
    align_left,          // Align to the left
    align_right,         // Align to the right
    align_center         // Align to the left
  };

  // Horizontal gravity (origin of x coordinates) 
  enum xpoint_t : uint32_t 
  {
    xpoint_left,           // x0 = left of text area
    xpoint_center,         // x0 = center of text area
    xpoint_right,          // x0 = right of text area
    xpoint_box_left,       // x0 = left of bounding box
    xpoint_box_center,     // x0 = center of bounding box
    xpoint_box_right,      // x0 = right of bounding box
  };
  
  // Horizontal gravity (origin of y coordinates) 
  enum ypoint_t : uint32_t 
  {
    ypoint_top,               // y0 = top of text area
    ypoint_center,            // y0 = center of text area 
    ypoint_bottom,            // y0 = bottom of text area 
    ypoint_box_top,           // y0 = top of bounding box 
    ypoint_box_center,        // y0 = center of bounding box  
    ypoint_box_bottom,        // y0 = bottom of bounding box 
    ypoint_top_baseline,      // y0 = baseline of first line of text
    ypoint_bottom_baseline    // y0 = baseline of last line of text
  };
  
  
  // The base class to implement a text box.
  // 
  //
  class TextBoxBase {
  protected:

    align_t align         = align_left;

    xpoint_t xgrav        = xpoint_left;
    ypoint_t ygrav        = ypoint_top;
    
    double  border_top    = 0;
    double  border_bottom = 0;
    double  border_left   = 0;
    double  border_right  = 0;
        
    double text_width ;   // width of the text area (once finalized)
    double text_height ;  // height of the text area (once finalized)

    double x0;  // if applied_xgrav then contains the x-origin relative to 'xpoint_left'
    double y0;  // if applied_ygrav then contains the y-origin relative to 'ygrav_top'

    bool finalized = false;

    std::vector<char32_t> text ;

    char escape_char = 0; 

    typedef void (TextBoxBase::* escape_cb) (const char *text) ;    
    
    // The base struct to describe a block of text.
    // That structure is not to be used directly.
    // Instead, the actual implementation must provide a derived version
    // with text attributes. 
    struct BlockBase {
      std::string     text;
      double          x;
      double          y;
      BLTextMetrics   metrics;
      BLGlyphBuffer   glyphs; 
    };
    
    
    // Describe a single line of text (composed of at least one block)
    // 
    struct Line {
      inline Line(align_t a) : align(a) , cr(false) { } 
      double x; // Can it be computed on the fly?
      double y; // Can it be computed on the fly?
      align_t align;
      bool cr;  // Carriage Return (\r) vs New Line (\n)
      // The combined metrics for all blocks in the line (so potentially using
      // different fonts). 
      struct {
        double width;    // Sum of all 'blocks[*].metrics.advance'
        double ascent;   // Max of all block font ascents
        double descent;  // Max of all block font descents
        double lineGap;  // Max of all block font lineGaps?
      } metrics;
      std::list<BlockBase*> blocks;
    } ;
    
    std::vector<Line> lines;
    
  public:
    
    TextBoxBase()  {
      lines.emplace_back(this->align);
    }            

    virtual ~TextBoxBase()  {      
    }            

    // Get the font from the block.
    virtual const BLFont &block_font(BlockBase &block) =0 ;
   
    // Return true if any of the current text attribute
    // was modified. The implementation may either track
    // all attribute changes or may compare the current
    // attributes to those of the specified last block of
    // text.
    virtual bool modified_attributes(BlockBase &last_block) =0;
   
    // Create a new empty block using the current attributes.
    virtual BlockBase *new_block(void) = 0;

    // Delete a block
    virtual void delete_block(BlockBase *block) =0 ;
   
    // Remove all blocks of text thus clearing the whole textbox.    
    void clear() {
      for ( Line & line : this->lines ) {
        for ( BlockBase *b : line.blocks ) {
          this->delete_block(b);
        }
      }
      this->lines.clear();
      // Start with one empty line
      this->align = align_left;
      this->lines.emplace_back(this->align);
      finalized = false;
    }
    
    // Return the current block or, if some text attributes are
    // changed, create a new current block.
    // In both cases, the new block is suitable for appending text.
    BlockBase & current_block() {
      bool need_new = false ;
      auto & blocks = this->lines.back().blocks;
      if (blocks.empty() || this->modified_attributes(*blocks.back())) {
        blocks.emplace_back( this->new_block() );
      }
      return *blocks.back();      
    }

    void assert_not_finalized() {
      if (finalized) {
        std::cerr << "ERROR: text box is already finalized\n";
        exit(1);
      }
    }

    inline void auto_finalize() {
      if (!finalized) {
        this->finalize();
      }
    }

    inline TextBoxBase &
    append_raw(const char *text, size_t len)
    {
      if (len>0) {        
        current_block().text.append(text,len);
      }
      return *this;      
    }

    // Parse and execute an escape sequence.
    //
    // The first character of text is always the escape_char.
    //
    // The text is not terminal by '\0' but its length is
    // indicated by the len argument
    //
    // That function is expected to change the `current` attributes
    // but it is also possible to change the 
    //
    // Return: The number of characters consumed by the
    //         escape sequence (including the escape character).
    //         A return value of 0 indicates that the escape character
    //         shall be consumed as a regular character (so an
    //         invalid escape sequence)
    //
    virtual size_t
    escape( const char *text, size_t len) {
      // The default behavior is to ignore escape sequences
      return 0 ;
    }
    
    TextBoxBase &
    append(const char *text, size_t len)
    {
      assert_not_finalized();
      size_t start = 0; // Start of the current raw sequence of character
      size_t pos = 0;  
      while(pos<len) {
        char c = text[pos] ;
        if ( c=='\n' || c=='\r' ) {
          // Flush pending raw sequence of characters
          append_raw(text+start, pos-start) ;
          if (c=='\r') 
            this->lines.back().cr = true;
          // Insert newline
          this->lines.emplace_back(this->align);
          // and consume the '\n'
          pos++;
          start=pos;
        } else if (escape_char && c==escape_char) {
          append_raw(text+start, pos-start) ;
          size_t n = this->escape(text+pos,len-pos) ;
          n = std::min(len-pos,n) ; // Be user that we not overflow
          pos += n;
          start = pos;
          // If the escape character was not consumed by 
          // the escape() member then treat is a 
          if (n==0) {
            pos++; 
          }
        } else {
          // A non-special character.
          pos++ ;
        }
      }
      // Flush the last raw sequence of characters
      append_raw(text+start, pos-start);      
      return *this;
    }
    
    inline TextBoxBase &
    append(const std::string &text)
    {
      return append(text.data(),text.size());
    }  

    // (re)compute the position of all blocks and update
    // metrics accordingly. 
    TextBoxBase & finalize() {
      assert_not_finalized();   
      this->text_width = 0; 
      double y = 0;
      double gap = 0;

      // mcr values hold the combined vertical metrics for consecutive 'cr' lines. 
      double mcr_ascent=0;
      double mcr_descent=0;
      double mcr_gap=0;

      double kcr = 0; // first k index of the current sequence of 'cr' lines
      size_t nl = this->lines.size() ;
      for ( size_t k=0 ; k<nl ; k++) {
        Line &line = this->lines[k];

        line.x = 0; 
        line.metrics.ascent=0; 
        line.metrics.descent=0; 
        line.metrics.lineGap=0; 
        double x = 0 ;
        for (BlockBase *block_p : line.blocks) {
          BlockBase & block = *block_p ;

          block.x = x;
          block.y = 0;   // reserved from future height adjustments (e.g. superscript, subscript)

          // block.glyphs.setUtf8Text(block.text.c_str());
          block.glyphs.setUtf8Text( block.text.data() , block.text.size()  );
          
          auto &font = this->block_font(block);
          font.shape(block.glyphs);
          int err = font.getTextMetrics(block.glyphs, block.metrics);

          x += block.metrics.advance.x;
          // Apply the font metrics to the line metrics
          const BLFontMetrics &fm = font.metrics();
          line.metrics.ascent  = std::max( line.metrics.ascent, double(fm.ascent)) ;
          // Let's maximize the total descent (so descent+linegap) instead
          // of always maximizing the linegap.   
          double total_descent = std::max( line.metrics.descent + line.metrics.lineGap,
                                           double(fm.descent) + double(fm.lineGap) );
          line.metrics.descent = std::max( line.metrics.descent , double(fm.descent) ) ;          
          line.metrics.lineGap = total_descent - line.metrics.descent ;
        }
        line.metrics.width = x;            
        this->text_width = std::max(this->text_width, line.metrics.width);

        // Apply the current line metrics to  the combined cr metrics  
        
        mcr_ascent  = std::max(mcr_ascent,  line.metrics.ascent) ;
        double mcr_total_descent = std::max( line.metrics.descent + line.metrics.lineGap,
                                             mcr_descent + mcr_gap );
        mcr_descent = std::max(mcr_descent, line.metrics.descent) ;
        mcr_gap     = mcr_total_descent - mcr_descent;
        
        if ( !line.cr || k==nl-1 ) {
          // Found a newline or reached the bottom. 
          // Apply the current combined cr metrics. 
          y += gap + mcr_ascent;
          for ( ; kcr <= k ; kcr++ ) {
            this->lines[kcr].y = y ;
          }
          y  += mcr_descent; 
          gap = mcr_gap;
          // And reset the cr metrics
          mcr_ascent = 0;
          mcr_descent = 0;
          mcr_gap = 0;
        }
        
      }
      this->text_height = y ;

#if 0
      if (this->align==align_right) {
        for (Line &line : this->lines) {
          line.x = this->text_width - line.metrics.width;
        }
      } else if (this->align==align_center) {
        for (Line &line : this->lines) {
          line.x = (this->text_width - line.metrics.width)/2;
        }
      }
#else
      for (Line &line : this->lines) {
        switch (line.align) {       
        case align_right:
          line.x = this->text_width - line.metrics.width;
          break;
        case align_center:
          line.x = (this->text_width - line.metrics.width)/2;
        case align_left:
        default:
          line.x = line.x; 
          break;
        }
      }
#endif
      
      finalized = true;
      return *this;
    }

    // Compute the width of the whole text area so excluding borders.
    double width() {
      auto_finalize();
      return this->text_width;
    }

    // Compute the height of the whole text area so excluding borders.
    double height() {
      auto_finalize();
      return this->text_height;
    }

    void setAlign(align_t a) {
      assert_not_finalized();
      if (a != this->align) {
        this->lines.back().align = a;
        this->align = a;
      }
    }
    
    // Compute the absolute X coordinate for the specified x-point.
    // By convention, xpoint_left has absolute coordinate 0. 
    double getAbsPointX(xpoint_t xp) 
    {
      auto_finalize();
      double x=0;
      switch(xp) {
      default:   // should not happen       
      case xpoint_left:
        // x0 = left of text area
        x = 0;
        break;
      case xpoint_center:
        // x0 = center of text area
        x = this->width() / 2.0 ;
        break;
      case xpoint_right:
        // x0 = right of text area
        x = this->width() ;
        break;
      case xpoint_box_left:
        // x0 = left of bounding box
        x = -border_left;
        break;
      case xpoint_box_center:
        // x0 = center of bounding box
        x = -border_left + (border_left + this->width() + border_right) / 2.0; 
        break;
      case xpoint_box_right:
        // x0 = right of bounding box
        x = this->width() + border_right; 
        break;
      }
      return x ;
    }
    
    // Compute the absolute Y coordinate for the specified y-point.
    // By convention, ypoint_left has absolute coordinate 0. 
    double getAbsPointY(ypoint_t yp) 
    {
      auto_finalize();
      double y=0; 
      switch(yp) {
      case ypoint_top:
        // Top of the text area.
        // Always zero in absolute coordinates.
        y = 0;
        break;        
      case ypoint_center:
        y = this->height() / 2 ; 
        break;
      case ypoint_bottom:
        y = this->height();
        break; 
      case ypoint_box_top:
        y = -border_top;
        break ;
      case ypoint_box_center:
        y = -border_top + (border_top + this->height() + border_bottom) / 2.0; 
        break;
      case ypoint_box_bottom:
        y = this->height() + border_bottom;
        break;
      case ypoint_top_baseline:
        y = this->lines.front().y ;
        break;
      case ypoint_bottom_baseline:
        y = this->lines.back().y ;
        break;
      }
      return y ;
    }

    // Compute the X coordinate for the specified X-point relative
    // to the current X-gravity.
    double getPointX(xpoint_t xp) {
      if ( xp == this->xgrav ) 
        return 0.0 ;
      else
        return getAbsPointX(xp) - getAbsPointX(this->xgrav) ;
    }

    // Compute the Y coordinate for the specified X-point relative
    // to the current Y-gravity.
    double getPointY(ypoint_t yp) {
      if ( yp == this->ygrav ) 
        return 0.0 ;
      else
        return getAbsPointY(yp) - getAbsPointY(this->ygrav) ;
    }
       
    // Get the bounding box relative to the selected gravity point.
    inline void
    getBox( double &x0, double &y0, double &x1, double &y1 )
    {
      x0 = this->getPointX(xpoint_box_left) ;
      x1 = this->getPointX(xpoint_box_right) ;
      y0 = this->getPointY(ypoint_box_top) ;
      y1 = this->getPointY(ypoint_box_bottom) ;
    }

    // Get the bounding box 
    inline void
    getBox( BLBox &box ) {
      getBox( box.x0, box.y0, box.x1, box.y1 ) ;
    }

    inline BLBox
    getBoxAt( double x, double y ) {
      BLBox out; 
      this->getBox(out) ;
      out.x0 += x ;
      out.y0 += y ;
      out.x1 += x ;
      out.y1 += y ;
      return out;
    }

    inline BLRect
    getRectAt( double x, double y ) {
      BLBox box;
      this->getBox(box) ;
      return BLRect( x+box.x0,
                     y+box.x0,
                     box.x1-box.x0,
                     box.y1-box.y0 ) ;
    }
    
  protected:


    // Draw the glyphs of a block of text.
    virtual void drawBlock(BLContext &ctx, BlockBase &block, double x, double y)
    {
      BLPoint pos( x+block.x, y+block.y ) ;
      ctx.fillGlyphRun( pos, this->block_font(block), block.glyphs.glyphRun() );
    }

  public:

    void setEscape(char c) {
      this->escape_char = c; 
    }
    
    void setBorder(double border) {
      this->border_left   = border;
      this->border_top    = border;
      this->border_right  = border;
      this->border_bottom = border;
    }

    // Set all 4 border sizes.
    // The arguments are given in the same order than for a BLBox
    void setBorder(double left, double top, double right, double bottom) {
      this->border_left   = left;
      this->border_top    = top;
      this->border_right  = right;
      this->border_bottom = bottom;
    }
    
    // Draw the box around the text.
    // The default implementation is to draw 
    // using the current fill style and 20% alpha
    virtual void
    drawBox(BLContext &ctx, double x, double y)
    {
      BLBox box = this->getBoxAt(x,y);      
      ctx.save();
      {
        ctx.setFillAlpha( ctx.fillAlpha() * 0.1 );
        ctx.fillBox(box);
      }
      ctx.restore();
    }

    virtual void
    draw(BLContext &ctx, double x, double y)
    {
      auto_finalize();
      for ( Line &line : this->lines) {
        for ( BlockBase *block_p : line.blocks) {        
          this->drawBlock(ctx, *block_p, x+line.x, y+line.y);
        }
      }
    }

   
  }; // of class TextBoxBase


  
  class SimpleTextBox : public TextBoxBase
  {
  public:
      
    SimpleTextBox(const std::string &fontname) {
      this->default_font = getFont(fontname);
      this->setFont(this->default_font);
      this->setEscape('^');
    }

    virtual ~SimpleTextBox() {
    }

  protected:

    enum FillStyleEnum : unsigned {
      FILL_STYLE_NONE,     // Do not stroke
      FILL_STYLE_CONTEXT,  // Fill using the current BlContext style
      FILL_STYLE_COLOR,    // Fill using solid color
      FILL_STYLE_COUNT,    // Number of fill styles      
    };

    enum StokeStyleEnum : unsigned {
      STROKE_STYLE_NONE,     // Do not stroke
      STROKE_STYLE_CONTEXT,  // Stroke using the current BLContext style 
      STROKE_STYLE_COLOR,    // Stoke using a fill color 
      STROKE_STYLE_COUNT,    // Number of stroke styles
    };
    
    struct Attributes 
    {
      // The font member is mandatory in Attributes.
      BLFont font;
      
      struct {
        FillStyleEnum style;
        union {
          BLRgba32 color;
        };
      } fill ;

      struct {
        StokeStyleEnum style;
        union {
          BLRgba32 color;
        };
      } stroke ;        
   
      // A comparison operator to detect any change in the attributes.
      bool operator==( const Attributes &a) {

        if ( this->font != a.font )
          return false;
        
        // Compare the fill styles.
        if ( this->fill.style != a.fill.style ) {
          return false;
        } else {
          switch(this->fill.style) {
          case FILL_STYLE_NONE:
          case FILL_STYLE_CONTEXT:
            break;
          case FILL_STYLE_COLOR:
            if (this->fill.color !=  a.fill.color)
              return false; 
            break;          
          default: // Should not happen ... yet. 
            return false; 
          }
        }

        // Compare the stroke styles.
        if ( this->stroke.style != a.stroke.style ) {
          return false;
        } else {
          switch(this->stroke.style) {
          case STROKE_STYLE_NONE:
          case STROKE_STYLE_CONTEXT:
            break;
          case STROKE_STYLE_COLOR:
            if (this->stroke.color !=  a.stroke.color)
              return false; 
            break;          
          default: // Should not happen ... yet. 
            return false; 
          }
        }
        
        return true;
      }
      
    };
    
    struct Block : public BlockBase {
      inline Block(Attributes &a) : BlockBase(), attribs(a) { } 
      Attributes  attribs;
    } ;
    
  private:

    Attributes current ;    

    BLRgba32 box_fill{0} ;

    BLFont   default_font; 
    
  protected:

    virtual const BLFont & block_font(BlockBase &block_) override {
      Block &block = static_cast<Block&>(block_);
      return block.attribs.font;
    }

    virtual bool modified_attributes(BlockBase &last_block_) override {
      Block &last_block  = static_cast<Block&>(last_block_) ;
      return !( last_block.attribs == current ) ;
    }

    virtual BlockBase *new_block(void) override {
      return new Block(current); 
    }

    virtual void delete_block(BlockBase *block) override {
      delete (Block*) block;
    }
    
  public: 

    void setFont(const BLFont &font) {      
      this->current.font = font ;
    }

    void setFont(const std::string &fontname) {
      this->current.font = getFont(fontname) ;
    }
    
    void setStrokeNone()  {
      this->current.stroke.style = STROKE_STYLE_NONE;
    } 

    void setStrokeContext()  {
      this->current.stroke.style = STROKE_STYLE_CONTEXT;
    } 

    void setStrokeColor(BLRgba32 c)  {
      this->current.stroke.style = STROKE_STYLE_COLOR;
      this->current.stroke.color = c ;
    }
    
    void setStrokeColor(uint32_t r, uint32_t g, uint32_t b, uint32_t a = 0xFFu)  {
      this->current.stroke.style = STROKE_STYLE_COLOR;
      this->current.stroke.color = BLRgba32(r,g,b,a) ;
    }

    void setFillNone()  {
      this->current.fill.style = FILL_STYLE_NONE;
    }

    void setFillContext()  {
      this->current.fill.style = FILL_STYLE_CONTEXT;
    } 

    void setFillColor(BLRgba32 c)  {
      this->current.fill.style = FILL_STYLE_COLOR;
      this->current.fill.color = c ;
    } 

    void setFillColor(uint32_t r, uint32_t g, uint32_t b, uint32_t a = 0xFFu)  {
      this->current.fill.style = FILL_STYLE_COLOR;
      this->current.fill.color = BLRgba32(r,g,b,a) ;
    } 

    void setBoxFillColor(BLRgba32 c)  {
      this->box_fill = c ;
    } 

    void setBoxFillColor(uint32_t r, uint32_t g, uint32_t b, uint32_t a = 0xFFu)  {
      this->box_fill = BLRgba32(r,g,b,a) ;
    } 

    
  protected:

    
    virtual void drawBlock(BLContext &ctx, BlockBase &block_, double x, double y) override 
    {
      Block &block = static_cast<Block&>(block_);
      
      BLPoint pos( x+block.x , y+block.y ) ;
      Attributes &attribs  = block.attribs ;
      auto &font = attribs.font ;
      ctx.save();
      
      if ( attribs.fill.style == FILL_STYLE_COLOR )
        ctx.setFillStyle( attribs.fill.color );
      if ( attribs.fill.style != FILL_STYLE_NONE )
        ctx.fillGlyphRun( pos,
                          font,
                          block.glyphs.glyphRun() );

      if ( attribs.stroke.style == STROKE_STYLE_COLOR ) 
        ctx.setStrokeStyle( attribs.stroke.color ) ;
      if ( attribs.stroke.style != STROKE_STYLE_NONE )
        ctx.strokeGlyphRun( pos,
                            font,
                            block.glyphs.glyphRun() );
              
      ctx.restore();
    }

    virtual size_t
    escape( const char *text, size_t len) override {

      std::string_view str(text,len);
      size_t pos ; // Position while parsing the arguments.
      BLRgba32 color ;
      std::string_view arg;
      
      // Need at least 2 characters
      if (len<2) {
        return 0;
      }
      
      if (str[1] == str[0]) {
        // The escape_char can be emited by using it twice.
        return 1;
      }

      pos=2; // Start offset of the arguments if any.
      
      switch(str[1]) {       
      case 'E': // Set the escape character
        if (len<3)
          return 0;
        setEscape(str[2]);
        return 3;

      case 'R': // Full reset: reset font, fill style and stroke style 
        this->setFont(this->default_font);
        this->setStrokeNone();
        this->setFillContext();
        return 2;        

      case 'r': // Weak reset: reset fill and stroke styles but keep font
        this->setStrokeNone();
        this->setFillContext();        
        return 2;

      case 'n': 
        this->setStrokeNone();
        return 2;
      case 'x': 
        this->setStrokeContext();
        return 2;
      case 'c':
        if ( parse_quoted_hexcolor(str,pos,'[',color,']') ) {
          this->setStrokeColor(color) ;
          return pos;
        } 
        return 0;

        
      case 'N': 
        this->setFillNone(); 
        return 2;
      case 'X': 
        this->setFillContext(); 
        return 2;
      case 'C': // set fill color
        if ( parse_quoted_hexcolor(str,pos,'[',color,']') ) {
          this->setFillColor(color) ;
          return pos;
        }          
        return 0;        

      case '<':
        this->setAlign(align_left) ;
        return 2;

      case '=':
        this->setAlign(align_center) ;
        return 2;

      case '>':
        this->setAlign(align_right) ;
        return 2;
                
      case 'F': // set font
        if ( parse_quoted( str.substr(2), '[', arg,']') ) {          
          setFont( std::string(arg) );
          return 3 + arg.size() + 1 ;
        }
        return 0;
      default:
        return 0;
      }

      // Failure 
      return 0;
    }
    
  public:

    // virtual void draw(BLContext &ctx, double x, double y) override
    // {
    //  this->TextBoxBase::draw(ctx,x,y);      
    // }
    
    virtual void drawBox(BLContext &ctx, double x, double y) override
    {
      if (box_fill.a) {
        BLBox box = this->getBoxAt(x,y);
        //std::cout << "Box " << box << "\n";
        ctx.save();
        {
          ctx.setFillStyle(BLRgba32(box_fill));
          ctx.fillBox(box);
        }
        ctx.restore();
      }
    }


  }; // of class SimpleTextBox


  
} // of namespace btb

#endif
