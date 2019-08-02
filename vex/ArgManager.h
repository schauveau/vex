#include<iostream>
#include<iomanip>
#include<cmath>
#include<cstdlib>

#include<string>
#include<vector>
#include<list>
#include<cerrno>

#include <functional>
#include <map>

using namespace std::literals;

const std::map<std::string, bool> bool_map = {
  { "0", false },
  { "n", false },
  { "no", false },
  { "F", false },
  { "false", false },

  { "1", true },
  { "y",  true },
  { "yes",  true },
  { "T", true },
  { "true",  true },
};

// A simple helper to create a lambda compatible with ArgManager::action_type. 
//
// arg is the name of the 'const char *' argument that contains the action 
// argument (may be NULL if the option does not support any argument).
//
// The optional remaining arguments are the capture specifiers.
//
#define ARG_LAMBDA_ACTION(arg,...) [ __VA_ARGS__ ](const char *arg) -> arg_error_t

// ARG_LAMBDA_PARSER(str,T,var,...) is a simple helper to create a
// lambda funcion that can be used as a parser (see also arg_parse) 
//
// Arguments:
//   - str is the name of the 'const char*' argument that
//     contains the input string.
//   - T is the output type
//   - var is the name of the output argument of type T &
//   - The remaining optional arguments specify the capture.
//
// Example:
//
// auto yy_parser = ARG_LAMBDA_PARSER(str,int,out) {
//    if (str == "ying"sv) {
//      out = 999;
//      return ArgSuccess;
//    } else if (str == "yang"sv) {
//      out = -999;
//      return ArgSuccess;
//    } else {
//      return ArgUnknownValueError;
//    }
//  };
//
#define ARG_LAMBDA_PARSER(str,T,var,...) [ __VA_ARGS__ ](const char *str, T &var) -> arg_error_t 
 
// Error codes for ArgManager actions and parsers.
enum arg_error_t {
  ArgSuccess,            // Success! Not an error. 
  ArgParseError,         // The argument could not be parsed.
  ArgInvalidRangeError,  // The argument is not in the valid range.        
  ArgEmptyArgumentError, // The argument is was empty
  ArgNoDefaultError,     // The action could not handle a missing argument (not a user error)
  ArgUnknownValueError,  // Similar to ParseError but for enumerations
  ArgOtherError,         // Another error occured
} ;  


// Base template for parsing arguments of type T in ArgManager
template <typename T> arg_error_t arg_parser(const char *arg, T &var) ;


// Provide a parser that only accepts values in the range (minval,maxval).
// 
template <typename T> 
auto arg_parser_between(T minval, T maxval) {
  return ARG_LAMBDA_PARSER(str, T, out, minval, maxval) {
    auto err = arg_parser(str,out);
    if ( err == ArgSuccess ) {
      if ( out < minval || out > maxval ) 
        return ArgInvalidRangeError;
    }
    return err; 
  };
}


class ArgManager {
public:

  ArgManager() =default ;
  ArgManager(const ArgManager &) =delete ;
  
  // Control the internal behavior of register_action. 
  enum register_mode_t {
    mode_user,   // An action created by the user 
    mode_set,    // An action that set a predefined value (no argument allowed)
    mode_parse,  // An action that parses a value (default is to require an argument)
  };

  // A callable object that implements an action when an option is encountered.
  // The argument is the option argument or NULL if that argument does not
  // support options.
  typedef std::function< arg_error_t (const char *)> action_type ;

  enum arg_kind_t {
    arg_unset,
    arg_none,
    arg_required,
    arg_optional,
  };
  
  struct Entry {
    std::string info; // The original info string
    std::vector<std::string> short_options;
    std::vector<std::string> long_options;
    std::string argname;
    action_type action;
    arg_kind_t  arg_kind = arg_unset;
    bool once = false; // that option must not be called more than once
    bool hidden = false; // if true then do not appear in help
    int count = 0; // number of times that option was set.
    std::string opt_value;
    bool use_opt_value = false; 
    std::vector<std::string> help_text ;
    
    Entry & help(std::string line) {
      help_text.push_back(line) ;
      return *this;
    }

    // Transform a required argument into an optional argument
    // by specifying a 'default' textual value. 
    Entry &optional(std::string value) {
      if ( this->arg_kind == arg_none || this->arg_kind == arg_unset ) {
        std::cerr << "Internal Error: Cannot make argument optional (" << info << ")\n"; 
        exit(1);
      }
      this->arg_kind  = arg_optional ;
      this->opt_value = value;
      return *this;
    }
  };

  std::vector<std::string> header_text ;
  std::vector<std::string> footer_text ;
  
  std::list<Entry> entries ;


  ArgManager & header(std::string text) {
    header_text.push_back(text);
    return *this ;
  }

  ArgManager & footer(std::string text) {
    footer_text.push_back(text);
    return *this ;
  }


  // A simple helper to retreive a value from a map.
  template<typename T>
  static inline arg_error_t
  get_from_map( const char *str,
                const std::map<std::string, T> &m,
                T &out ) {
    auto it = m.find(str) ;
    if (it != m.end() ) {
      out = it->second ;
      return ArgSuccess;
    } else {
      return ArgUnknownValueError;
    }    
  }

  
  Entry & register_action(const std::string &info,
                          register_mode_t mode,
                          action_type a)
  {
    size_t pos = 0 ;

    Entry & entry = entries.emplace_back() ;

    entry.arg_kind  = arg_unset;
    entry.argname   = "ARG"; // A good default for the argument name
  
    // Parse tbe information string
    std::string desc(info);
    while ( !desc.empty() ) {
      auto next = desc.find(' ');
      if (next==0) {
        // Skip blank
        desc = desc.substr(1);
        continue;
      }
      if (next == std::string::npos) {
        next = desc.size();
      }
      std::string item = desc.substr(0,next);
      desc = desc.substr(next);


      
      
      if ( item[0]=='-' ) {
        // Not all charaacters should be allowed in option names.
        static const char * allowed_chars =
          "abcdefghijklmnopqrstuvwxyz"
          "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
          "0123456789"
          "-@?_"
          ;
        if ( item.find_first_not_of(allowed_chars) != std::string::npos ) {
          std::cerr << "Internal Error: Malformed option name '" << item << "' in argument description\n";
          exit(1);
        }
        if ( item.size()>1 && item[1]=='-' )  {          
          entry.long_options.push_back(item);
        } else {
          entry.short_options.push_back(item);
        }
      } else if ( item[0] == '%' ) {
        if ( item == "%once" ) {
          entry.once = true ;
        } else if ( item == "%hidden" ) {
          entry.hidden = true ;
        }  else {
          std::cerr << "Internal Error: Unknown item '" << item << "' in argument description\n";
          exit(1);
        }
      } else if ( item[0] == '=' ) {
        if ( mode == mode_set ) {
          std::cerr << "Internal Error: Argument is not allowed here ('" << info << "')\n";          
          exit(1);
        }
        entry.argname = item.substr(1) ;
        entry.arg_kind = arg_required;
      } else if ( item[0] == '?' ) {
        if ( mode == mode_set ) {
          std::cerr << "Internal Error: Argument is not allowed here ('" << info << "')\n";          
          exit(1);
        }
        entry.argname = item.substr(1) ;
        entry.arg_kind = arg_optional;
      } else {
        std::cerr << "Unknown item '" << item << "' in argument description\n";
        exit(1);
      }      
    }
    
    entry.action = a ;

    if ( entry.arg_kind == arg_unset ) {
      switch(mode) {
      case mode_set:
        entry.arg_kind = arg_none ;
        break;
      case mode_user:
        entry.arg_kind = arg_none ;
        break;
      case mode_parse:
        entry.arg_kind = arg_required ;
        break;           
      default:
        entry.arg_kind = arg_none ;
        break;
      } 
    }
        
    return entry;
    
  }

  //
  // Add an option that performs the specified action.
  // 
  //   
  Entry & action(const std::string &info, action_type action)
  {
    return register_action( info, mode_user, action); 
  }

  //
  // Add an option that assigns the specfied var to the specified value. 
  //   
  template<typename T> 
  Entry & assign( const std::string &info, T &var, T value)  {
    return register_action(
      info,
      mode_set,
      ARG_LAMBDA_ACTION(str,&var,value) {
        var = value;
        return ArgSuccess;
      }
      ) ;    
  }

  //
  // Add an option that increments var by 1.
  // 
  template<typename T> 
  Entry &
  increment( const std::string &info, T &var)  {
    auto action = ARG_LAMBDA_ACTION(str,&var) {
      var = var+1 ;
      return ArgSuccess;
    }; 
    return register_action(info,mode_set,action);
  }
  
  //
  // Add an option that parses its argument value using the
  // default parser arg_parser<T>
  // 
  
  template<typename T>
  Entry &
  parse( const std::string &info, T &var)
  {
    return parse(info,var, arg_parser<T>) ;
  }

  //
  // Add an option that parses its argument using the specified parser.
  //
  // parser must be a callable object with arguments and result
  // compatible with arg_parser<T> or ARG_LAMBDA_PARSER.
  //
  // Simply speaking, parser should allow something like:
  //    int arg_error_t err = parser("hello",var); 
  //
  //  
  template<typename T, typename Parser>
  Entry &
  parse( const std::string &info, T &var, Parser parser )  {
    return register_action(
      info,
      mode_parse,
      ARG_LAMBDA_ACTION(str,&var,parser) {
        return parser(str, var) ;
      }) ;
  }

  
  // Add an option that selects a value from the specified map.
  template<typename T>
  Entry &select( const std::string &info,
                 T &var,
                 const std::map<std::string,T> & value_map
    ){
    return register_action(
      info,
      mode_parse,
      ARG_LAMBDA_ACTION(str,&var,&value_map) {        
        if (str==NULL) {
          return ArgNoDefaultError;
        } else {
          return ArgManager::get_from_map(str, value_map, var);
        }
      }
     ) ;
  }

  // Add a value that selects a value from the specified maps
  // and, in case of failure, parse the value using the specified parser.
  template<typename T, typename Parser>
  Entry &select_or_parse( const std::string &info,
                          T &var,
                          const std::map<std::string,T> & value_map,
                          Parser parser)
  {
    return register_action(
      info,
      mode_parse,
      ARG_LAMBDA_ACTION(str,&var,&value_map,parser) {        
        if (str==NULL) {
          return ArgNoDefaultError;
        } else {
          auto err = ArgManager::get_from_map(str, value_map, var);
          if (err==ArgSuccess)
            return ArgSuccess;
          else 
            return parser(str, var) ;
        }
      }
     ) ;
  }
  
  // Add a value that selects a value from the specified maps
  // and, in case of failure, parse the argument arg_parser<T>
  template<typename T>
  Entry &
  select_or_parse( const std::string &info,
                   T &var,
                   const std::map<std::string,T> & value_map)
  {
    return select_or_parse(info, var, value_map, arg_parser<T>) ;      
  }

  Entry *
  find_long_option(std::string &option)
  {
    for ( Entry & entry : entries ) {
      for ( const std::string & str : entry.long_options ) {
        if ( option == str ) {
          return &entry;
        }
      }
    }
    return NULL;
  }
  
  Entry *
  find_short_option(std::string &option)
  {
    for ( Entry & entry : entries ) {
      for ( const std::string & str : entry.short_options ) {
        if ( option == str ) {
          return &entry;
        }
      }
    }
    return NULL;
  }

  Entry * find_option(std::string &option) {
    Entry * e = find_short_option(option);
    if (e)
      return e;
    else
      return find_long_option(option);
  }

  
  void process(int argc, char **argv) {

    for (int i=1;i<argc; i++) {
      
      std::string option = argv[i] ;
      std::string argument ;
      bool argument_set = false; 
      
      if (option[0]=='-') {
        Entry * entry = NULL;        
        if (option[1]=='-') {
          // A long option.
          // Separate the option from its argument (e.g. --foobar=xxx )
          auto pos = option.find('=') ;
          if (pos!=std::string::npos) {
            argument = option.substr(pos+1); 
            option = option.substr(0,pos);
            argument_set = true;
          }
          // Find the entry that can handle that long argument
          entry = find_long_option(option);
        } else {
          // A short option.
          // Separate the option from its argument (e.g. -Fxxxx )
          if ( option.size() > 2 ) {
            argument = option.substr(2); 
            option = option.substr(0,2);
            argument_set = true;
          }
          entry = find_short_option(option);
        }

        if (!entry) {
          std::cerr << "ERROR: Unknown option " << option << "\n";
          exit(1);
        }

        if ( argument_set && entry->arg_kind ==  arg_none ) {
          std::cerr << "ERROR: Unexpected argument to option " << option << "\n";
          exit(1);
        }

        if ( !argument_set ) {
          if ( entry->arg_kind == arg_required ) {
            if ( i+1 == argc ) {
              std::cerr << "ERROR: Required argument after option " << option << "\n";
              exit(1);
            }            
            i++;
            argument = argv[i]; 
            argument_set = true;
          } else if ( entry->arg_kind == arg_optional ) {
            argument = entry->opt_value ;
            argument_set = true;  
          } else {
            argument = "" ;
          }
        }

        entry->count++ ;

        if (entry->once && entry->count>1 ) {
          std::cerr << "Error: Option " << option << " should only be used once\n";
          exit(1);          
        }
        
        const char * arg =  argument_set ? argument.c_str() : NULL ;

        error_t err = entry->action(arg);

        std::string fulloption = option;        
        if (argument_set)
          fulloption = fulloption + " '" + argument + "'";                       

        if ( err != ArgSuccess ) {
          switch( err ) {
          case ArgParseError:
            std::cerr << "Error: Parse error in option " << fulloption << "\n";
            break;
          case ArgInvalidRangeError:
            std::cerr << "Error: Invalid range in option " << fulloption << "\n";
            break;
          case ArgEmptyArgumentError:
            std::cerr << "Error: Illegal empty argument in option " << fulloption << "\n";
            break;
          case ArgNoDefaultError:
            std::cerr << "Internal Error: Cannot handle option " << fulloption << "\n";
            break;
          case ArgUnknownValueError:
            std::cerr << "Error: Unknown value in option " << fulloption << "\n";
            break;
          case ArgOtherError:            
          default:          
            std::cerr << "Error(: Unexpected error in option " << fulloption << "\n";
          }
          exit(1);
          
        }  
      } else {
        std::cerr << "Error: Found orphan argument '" << option << "'\n";
        exit(1);
      }
      
    }
  }

  void usage( std::ostream &out ) {
    size_t header_width = 0;

    
    for ( int pass=0;pass<2;pass++) {

      if ( pass==1) {
        for ( auto &str : header_text ) {
          out << str << std::endl ;
        }
      }
      
      for ( auto it = entries.begin() ; it!= entries.end() ; ++it ) {
        
        Entry & entry = *it ;

        if (entry.hidden)
          continue;
        
        std::string header;
        
        bool first=true ; 
        for ( auto &opt : entry.short_options ) {
          header += (first ? "  " : ", " );
          first = false;
          header += opt ;
        }
        if (first)
          header += "    ";
        for ( auto &opt : entry.long_options ) {
          header += (first ? "  " : ", " );
          first = false;
          header += opt ;
        }        
        if (first) {
          // No valid option for that entry! Ignore.
          continue ; 
        }

        switch (entry.arg_kind) {
        default:
        case arg_none:
          break;
        case arg_required:
          header += " " + entry.argname; 
          break;
        case arg_optional:
          if ( entry.long_options.empty() )
            header += "[" + entry.argname + "]";
          else
            header += "[=" + entry.argname + "]";
          break;          
        }
        
        if (pass==0) {
          header_width = std::max(header_width, header.size() ) ;
        } else {
          out << std::setw(header_width) << std::left << header ;
        }
          
        if (pass==1) {
          if ( entry.help_text.empty() ) {
            out << "\n";
          } else {
            out << "  ";          
            for (size_t k=0; k<entry.help_text.size(); k++) {
              if (k>0)
                out << std::setw(header_width+2) << "" ;
              out << entry.help_text[k] << "\n"; 
            }
          }
        } 
        
      }

      
      if ( pass==1) {
        for ( auto &str : footer_text ) {
          out << str << std::endl ;
        }
      }
      
      
    }
  }
    
} ;


template <> arg_error_t
arg_parser(const char *arg, std::string &var)
{
  if ( !arg )
    return ArgNoDefaultError; 
  var = arg;
  return ArgSuccess;
}

template <> arg_error_t
arg_parser(const char *arg, int &var)
{
  if ( !arg )
    return ArgNoDefaultError;
  char *endptr;
  errno = 0;
  long long int v = std::strtoll(arg, &endptr, 0);
  if (*endptr != '\0')
    return ArgParseError; // unprocessed characters at the end
  if ( errno == ERANGE )
    return ArgInvalidRangeError; // overflow or underflow on 'long int'
  
  // 'long long int' should be larger than 'int'.
  // So detect 'small' overflow but for convenience, 
  // allows up to the max of 'unsigned'.    
  if ( v < std::numeric_limits<int>::min() ||
       v > std::numeric_limits<unsigned>::max() ) {
    return ArgInvalidRangeError;
  }  
  // ArgSuccess
  var = v;
    return ArgSuccess;
}

template<> arg_error_t
arg_parser(const char *arg, float &var)
{
  if ( !arg )
    return ArgNoDefaultError ;
  char *endptr;
  errno = 0;
  float v = std::strtof(arg,&endptr);
  if (*endptr != '\0')
    return ArgParseError; // unprocessed characters at the end
  if ( v!=0 && errno == ERANGE )
    return ArgInvalidRangeError; // overflow or underflow     
  var = v ;
  return ArgSuccess;
}

template<> arg_error_t
arg_parser(const char *arg, double &var)
{
  if ( !arg )
    return ArgNoDefaultError ;
  char *endptr;
  errno = 0;
  float v = std::strtod(arg,&endptr);
  if (*endptr != '\0')
    return ArgParseError; // unprocessed characters at the end
  if ( v!=0 && errno == ERANGE )
    return ArgInvalidRangeError; // overflow or underflow     
  var = v ;
  return ArgSuccess; 
}

template<> arg_error_t
arg_parser(const char *arg, bool &var)
{
  if ( !arg )
    return ArgNoDefaultError;
  else
    return ArgManager::get_from_map(arg, bool_map, var) ;
}



#if ARG_PARSER_MAIN_SIMPLE

///////////////////////////////////////////////////////
//
// A very simple example that only show basic features
//
//////////////////////////////////////////////////////


int
main(int argc, char **argv)
{

  ArgManager amgr;

  enum Digit { zero = 0, one, two, three, four, five, six, seven, height, nine} ; 

  std::map<std::string, Digit> digit_map = {
    { "zero",   zero   },
    { "one",    one    },
    { "two",    two    },
    { "three",  three  },
    { "four",   four   },
    { "five",   five   },
    { "six",    six    },
    { "seven",  seven  },
    { "height", height },
    { "nine",   nine   },
  };

  float f = 0.0 ;
  int   x = 0;
  std::string s="";
  Digit d = zero;
  bool show_help = false;
  int   hidden = 0;
  
  std::string progname = argv[0];
  
  amgr.header("Usage: " + progname + "[options...]");
  amgr.header("");
  amgr.header("Supported Options:");
  amgr.header("");
  
  amgr.footer("");
  amgr.footer("The resulting values are printed: X is an integer,");
  amgr.footer("S is a std::string, F is a float, and D is an enum");
  amgr.footer("");
  amgr.footer("Remark: there is also a --hidden option");
    
  amgr.assign(" -h --help", show_help, true)
    .help("show this help");
  amgr.parse(" -x --set-x =ARG", x)
    .help("set X to an integer value");
  amgr.parse(" -s --set-s =ARG", s)
    .help("set S to any text");
  amgr.parse(" -f --set-f =ARG", f)
    .help("set F to an float value");
  amgr.select(" -d --set-d =ARG", d, digit_map )
    .help("set D to a digit name (zero, one, ...)");

  amgr.parse(" --hidden =ARG %hidden", hidden);

  amgr.process(argc,argv);
  
  if (argc==1 || show_help ) {
    amgr.usage(std::cout);
    return 1;
  }

  std::cout << " X = "  << x << "\n";
  std::cout << " S = '" << s << "'\n";
  std::cout << " F = "  << f << "\n";
  std::cout << " D = "  << d << "\n";
  if(hidden!=0)
    std::cout << " Hidden = "  << hidden << "\n";
  return 0;
}

#elif ARG_PARSER_MAIN_FULL

/////////////////////////////////////////////////////////////////////
//
// A more complex example showing most argument parsing cases.
//
/////////////////////////////////////////////////////////////////////


#include<cstdlib>
#include<cstdio>
#include<cstring>
#include<cerrno>
#include<climits>

enum Digit {
  zero = 0,
  one,
  two,
  three,
  four,
  five,
  six,
  seven,
  height,
  nine
} ; 

std::map<std::string, Digit> digit_map = {
  { "zero",   zero   },
  { "one",    one    },
  { "two",    two    },
  { "three",  three  },
  { "four",   four   },
  { "five",   five   },
  { "six",    six    },
  { "seven",  seven  },
  { "height", height },
  { "nine",   nine   },
};

// A set of predefined float constants
std::map<std::string, float> direction_map = {
  { "N"     , 0.0f },
  { "NE"    , float(1*M_PI/4) },
  { "E"     , float(2*M_PI/4) },
  { "SE"    , float(3*M_PI/4) },
  { "S"     , float(4*M_PI/4) },  
  { "SW"    , float(5*M_PI/4) },  
  { "W"     , float(6*M_PI/4) },  
  { "NW"    , float(7*M_PI/4) },  
}
;



arg_error_t
my_bool_arg_parser(const char *arg, bool &var)
{
  if ( !arg )
    return ArgNoDefaultError;
  else if (arg == "Yeah"sv) {
    var = true;
    return ArgSuccess;;
  } else if (arg == "Nope"sv) {
    var = true;
    return ArgSuccess;;
  } else {
    return ArgUnknownValueError;
  }
}


// A simple color 
struct RGBColor {
  uint8_t r,g,b ;
} ;

std::ostream &
operator<<(std::ostream& out, const RGBColor&col) {
  return out << "#"
             << std::setfill('0') << std::hex
             << std::setw(2) << unsigned(col.r)
             << std::setw(2) << unsigned(col.g)
             << std::setw(2) << unsigned(col.b)
             << std::setfill(' ') << std::dec
    ;
}

//
// Parse a RGBColor as #ARB or #AARRGG
//
arg_error_t
my_rgb_parser(const char *arg, RGBColor &out)
{
  char *end;
  size_t len = strlen(arg);

  if (len!=4 && len!=7)
    return ArgParseError; 
  if ( arg[0] != '#' )
    return ArgParseError;
  

  unsigned v = strtol(arg+1,&end,16) ;
  if (*end!='\0') {
    return ArgParseError;
  }

  if (len==4) {
    out.r = ((v>>8) & 0xF) * 17 ;
    out.g = ((v>>4) & 0xF) * 17 ;
    out.b = ((v>>0) & 0xF) * 17 ;
  } else {
    out.r = ((v>>12) & 0xFF) ;
    out.g = ((v>>8) & 0xFF) ;
    out.b = ((v>>4) & 0xFF) ;    
  }
  return ArgSuccess;
}

std::map<std::string, RGBColor> known_colors = {
  { "white"    , RGBColor{0xFF,0xFF,0xFF} },
  { "black"    , RGBColor{0x00,0x00,0x00} },
  { "red"      , RGBColor{0xFF,0x00,0x00} },
  { "blue"     , RGBColor{0x00,0x00,0xFF} },
  { "green"    , RGBColor{0x00,0xFF,0x00} },
  { "yellow"   , RGBColor{0xFF,0xFF,0x00} },
  { "magenta"  , RGBColor{0xFF,0x00,0xFF} },
  { "cyan"     , RGBColor{0x00,0xFF,0xFF} },
};

int main(int argc, char **argv) {

  std::string program = argv[0];

  ArgManager amgr;

  float A = 1.0;
  bool  B = false ;
  RGBColor C{0,0,0};
  Digit D = zero ;
  std::string S="" ;
  int   V = 0;
  int   X = 0;
  int   Y = 0;
  int   Z = 0;


  // 
  amgr.action(
    "-h -? --help ",        
    ARG_LAMBDA_ACTION(str,&amgr) {
      amgr.usage(std::cerr);
      exit(1);
    })
    .help("show this help");
    
  amgr.parse( "-x --x =VAL %once ", X )
    .help("set the  value of X (once)")
    ;

  amgr.parse( "-y --y =VAL", Y )
    .help("set the value of Y")
    ;
 
  amgr.assign( "--secret %hidden", X, 42 )
    .help("Set X to 42. This is a hidden options")
    .help("that does not appear in the help")
    ;
 
  amgr.parse( "-z --z =VAL", Z , arg_parser_between(0,255) )
    .help("set the value of Z to a value between 0 and 255")
    ;
  
  amgr.parse( "-Z --ZZ =ARG", Z )
    .optional("42")
    .help("set the value of Z. If no argument is specified")
    .help("then 42 is used.");    
    ;
  
  amgr.parse( "-a --alpha =VALUE", A)
    .help("set the floating-point value of A")
    ;

  amgr.assign( "--red", C, RGBColor{255,0,0})
    .help("set C to the color red (#FF0000)")
    ;

  amgr.assign( "--green", C, RGBColor{0,255,0})
    .help("set C to the color green (#00FF00)")
    ;
  

  amgr.parse( "-c =RGB", C, my_rgb_parser)
    .help("set the color as #RGB or #RRGGBB")
    ;

  // Process an argument using both a map and a specific parser. 
  amgr.select_or_parse( "-C --color =RGB", C, known_colors, my_rgb_parser )
    .help("set the color as #RGB or #RRGGBB or using one")
    .help("of the predefined color names:")
    .help("   white, black, red, green, blue, yellow") 
    .help("   megenta, cyan");
    ;

  amgr.select( "--colorname =RGB", C, known_colors )
    .help("set the color using a predefined color name:")
    .help("   white, black, red, green, blue, yellow") 
    .help("   megenta, cyan");    
    ;

  amgr.select_or_parse( "-A --angle =VALUE", A, direction_map)
    .help("same as -a but somce symbolic directions are")
    .help("allowed: N, NE, E, SE, S, SW, W, and NW")

    ;
  
  amgr.parse( "-S --string", S)
    .help("set S to any string")
    ;
  
  amgr.parse( "-s --single %once", S)
    .help("set S to any string but only once")
    ;
  
  amgr.assign( "-b", B, true)
    .help("set B to true")
    ;

  amgr.assign( "-B", B, false)
    .help("set B to false")
    ;
  
  
  amgr.increment( "-v" , V)
    .help("increment V by 1")
    ;

  amgr.assign( "--pi", A, 3.14159f )
    .help("set A to PI")
    ;

  amgr.assign( "--evil", X, 666)
    .help("set X to 666")
    ;

  amgr.parse( "--unit", A, arg_parser_between(0.0f,1.0f) )
    .help("set A to a value between 0.0 and 1.0")
    ;

  // Using a parser function
  amgr.parse(" --bee =ARG", B, my_bool_arg_parser )
    .help("set B using 'Yeah' for true and 'Nope'")
    .help("for false")
    ;

  // Using a lamba parser 
  amgr.parse(" --foo =ARG", X,
             ARG_LAMBDA_PARSER(str,int,out) {
               if (str == "foo"sv) {
                 out = 42;
               } else if (str == "bar"sv) {
                 out = 666;
               } else {
                 return ArgUnknownValueError;
               }
               return ArgSuccess;            
             }
    )
    .help("set X to 42 if ARG=Foo or to 666")
    .help("if ARG=bar")
    ;


  // Using a lamba parser declared previously 
  auto lucky_parser = ARG_LAMBDA_PARSER(str,int,out) {
    auto err = arg_parser<int>(str,out);
    if (err==ArgSuccess && out==13 ) 
      out = 42;
    return err;
  };
  
  amgr.parse(" --lucky =ARG", X, lucky_parser )
    .help("set X but replace 13 by 42")
    ;

#if 0
  amgr.parse(" --BAD =ARG", B, yy_parser )
    .help("This is not gping to work because B is a bool")
    .help("and yy_parser is an 'int' parser")
    ;
#endif
  
  amgr.select( "-d =DIGIT", D, digit_map )
    .help("set D to a digit (one, two, ...)")
    ;

  amgr.parse( "-c --cookie =NUM", V)
    .optional("99999") 
    .help("set V to the specied value or to 99999")
    .help("when NUM is not specified.")
    ;
 
  
  //
  // Use an action to set multiple variables at once.
  //  
  amgr.action(
    "--xyz =NUM",
    ARG_LAMBDA_ACTION(str,&amgr,&X,&Y,&Z) {
      int v;
      auto err = arg_parser(str,v);
      if (err)
        return err;      
      X = Y = Z = v;
      return ArgSuccess;
    }
    )
    .help("set x, y and z") ;
  
  
  //
  // An action can also be used to perfrom any task while parsing the
  // command line arguments.
  //
  // Here, the -S option will dump the current state of all the variables.
  //
  amgr.action(
    "-S  ?TITLE",
    ARG_LAMBDA_ACTION(str,&) {
      std::cout << "######## " << (str?str:"<null>") << "\n";
      std::cout << "A = " << A << "\n";
      std::cout << "B = " << B << "\n";
      std::cout << "C = " << C << "\n";
      std::cout << "D = " << D << "\n";
      std::cout << "S = '" << S << "'\n";
      std::cout << "V = " << V << "\n";
      std::cout << "X = " << X << "\n";
      std::cout << "Y = " << Y << "\n";
      std::cout << "Z = " <<Z << "\n";
      return ArgSuccess;
    }
    )
    .help("dump all values now and continue parsing")
    .help("arguments.") ;

  /////////////////////////
  // Set header and footer  
  /////////////////////////
      
  amgr
    .header("Usage: " + program+ " [OPTIONS]...\n" ) 
    .header("This is test program for my C++ class ArgManager")
    .header("")
    ;  

  amgr
    .footer("" ) 
    .footer("Examples:")
    .footer("   " + program + " -b --xyz 444 -d six -x1111 -Saaaa --cookie  -Sxxxxx --evil -S -h")
    .footer("")
    ;  

  amgr.process(argc,argv) ;

  // And dump the final result
  
  std::cout << "######## Done\n";
  std::cout << "A = " << A << "\n";
  std::cout << "B = " << B << "\n";
  std::cout << "C = " << C << "\n";
  std::cout << "D = " << D << "\n";
  std::cout << "S = '" << S << "'\n";
  std::cout << "V = " << V << "\n";
  std::cout << "X = " << X << "\n";
  std::cout << "Y = " << Y << "\n";
  std::cout << "Z = " << Z << "\n";

  
  return 0;
}

#endif
