/*
 * $Id: dsSmartException.cxx,v 1.1.1.1 2004/09/10 08:12:19 dsamersoff Exp $
 *
 */

#include <dsSmartException.h>

using namespace std;
using namespace libdms5;


dsSmartException::dsSmartException() {
  // pass
}

dsSmartException::~dsSmartException() {
 // pass
}

dsSmartException::dsSmartException(const char * tag, const char *format, ... ) {
  va_list ap;
  va_start(ap, format);

  fire(tag, format, ap);

  va_end(ap);
}

dsSmartException::dsSmartException(int facility, const char * tag, const char *format, ... ) {
  va_list ap;
  va_start(ap, format);

  fire(facility, tag, format, ap);

  va_end(ap);
}


void dsSmartException::fire(const char * tag, const char *format, va_list ap ) {
  fire(0, tag, format,  ap);
}

void dsSmartException::fire(const char *format, va_list ap ) {
  fire(0, 0, format,  ap);
}

void dsSmartException::fire(int facility, const char * tag, const char *format, va_list ap ) {

  _facility = facility;

  if (tag) {
    _tag << tag << ends;
  }

#ifdef HAVE_VFORM
  _message.vform(format, ap);
#else
  dsForm::vform(_message, format, ap);
#endif

  _message << ends;
#ifdef HAVE_EXECINFO_H
  _backtrace_nptrs = backtrace(_backtrace_buffer, BACKTRACE_SIZE);
#endif
}

ostream& dsSmartException::print(ostream& os)
{  
   os << tag() << '#' << msg() << endl;

   // TODO: copy bfd based code HERE

#ifdef HAVE_EXECINFO_H
   char **strings = backtrace_symbols(_backtrace_buffer, _backtrace_nptrs);
   if (strings != NULL) {
     for (int j = 0; j < _backtrace_nptrs; j++) {
        os << strings[j] << "\n";
     }
     free(strings);
   }
#endif

   return os;
}

