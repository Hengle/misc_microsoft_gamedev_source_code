#ifndef __CMDLINE_TOOLS__
#define __CMDLINE_TOOLS__

#include <string>

#define CMDLINE_TITLE                     "\n -= compiled on " __TIMESTAMP__ " - (c) Microsoft Corp. =- \n\n"
#define CMDLINE_PARAM_BEGIN               std::string __help_string=CMDLINE_TITLE; \
                                          { int _i=0; while (_i<argc) { bool _none=true;
#define CMDLINE_PARAM_END                 if (_none) _i++; } }
#define CMDLINE_SHOW_HELP                 cerr << __help_string << endl;
#define CMDLINE_PARAM_FLAG(CMD,FLAG,MSG)            \
    if (_i == 0) __help_string += std::string(CMD) + std::string(" : ") + std::string(MSG) + "\n"; \
    else if (_i < argc && !strcmp(CMD,argv[_i])) {  \
      FLAG=true;                                    \
      cerr << "* " MSG " enabled" << endl;          \
      _none=false;                                  \
      _i++;                                       }
#define CMDLINE_PARAM_VAR(CMD,VAR,FCT,MSG)                       \
    if (_i == 0) __help_string += std::string(CMD) + std::string(" : ") + std::string(MSG) + "\n"; \
    else if (_i < argc && !strcmp(CMD,argv[_i]) && _i+1<argc) {  \
      VAR=FCT(argv[_i+1]);                                       \
      cerr << "* " MSG " set to: " << VAR << endl;               \
      _none=false;                                               \
      _i+=2;                                                  }

#endif
