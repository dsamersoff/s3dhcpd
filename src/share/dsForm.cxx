/*
 * $Id: dsForm.cxx,v 1.1.1.1 2004/09/10 08:12:19 dsamersoff Exp $
 *
 */

#include <dsForm.h>
#ifdef WITH_XLAT
# include <dsXlat.h>
#endif

using namespace std;
using namespace libdms5;

ostream& dsForm::form(ostream& os, const char *format,    ... )
{
  va_list ap;
  va_start(ap, format);
  ostream& ros = dsForm::vform(os,format,ap);
  va_end(ap);

  return ros;
}

ostream& dsForm::vform(ostream& os, const  char *format, va_list ap )
{
  const char* fptr = format;

  while (1)
  {
    if (*fptr == '%')
    {
      ++fptr; 

      /* init state */ 
      int width     = -1;
      int modifier  = 'i';
      int fill      = ' ';
      int left      = 0;

      os.unsetf( ios::uppercase ); 


      if (*fptr == '-') {
        left = 1;
        ++fptr;
      }
      else {
        left = 0;
      }

      // set zerro fill if necessary
      if (*fptr == '0') {
        fill = '0';
        ++fptr;
      }

      //XXX ??
      if (*fptr == '.') {
        ++fptr;
        fill = '0';
      }

      // extract width from varg or from fromat
      if (*fptr == '*') {
        width = va_arg(ap, int);
        ++ fptr;
      }
      else {
        for (; (*fptr >= '0' && *fptr <= '9'); ++fptr) {
          if (width < 0) {
            width = 0;
          }
          width = width*10 + (*fptr - '0');
        }
      }

      if (*fptr == 'l') { // handle %lX && %ld
        modifier = 'l';
        ++fptr;
      }

      switch (*fptr) {
        case 'c': {
          int c = va_arg(ap, int);
          os << (char) c;
        }
        break;
        case 's': {
          char* s = va_arg(ap, char *);
          if (!s) {
             s = (char *) "(null)";
          }
          if(!left){
            os << setw(width);
          }
          os << s;
          // Manually emulate ios::left as it not always work
          if (width > 0 && left) {
            int len = strlen(s);
            if (len < width) {
              for(int i = 0; i < width-len; ++i) {
                os << " ";
              }
            }
          }
        }
        break;
#ifdef WITH_XLAT
        case 'U': { // decode utf8 string
          os << setw(width);
          char* s = va_arg(ap, char *);
          int len = va_arg(ap, int);
          decode_utf8(s, len, os);
         }
         break;
#endif
         case 'd' : {
            os << setw(width);
            os << setfill( (char) fill);
            os << dec;
            if (modifier == 'l') {
              long ld = va_arg(ap, long);
              os << ld;
            }
            else if (modifier == 'i') {
              int id = va_arg(ap, int);
              os << id;
            }
          }
          break;
        case 'X' :
          os.setf(ios::uppercase); 
        case 'p' :
        case 'x' : {
          os << setw(width);
          os << setfill( (char) fill);
          os << hex;
          if (modifier == 'l') {
            long ld = va_arg(ap, long);
            os << ld;
          }
          else if (modifier == 'i') {
            int id = va_arg(ap, int);
            os << id;
          }
        }
        break;
        case 'f' : {
            os << setiosflags(ios::fixed);
            os << setprecision( (width >= 0) ? width : 6 );
            double ld = va_arg(ap, double);
            os << ld;
        }
        break;
        case 'm': {
            os << strerror(errno);            
        }
        break;
#ifdef  __BFD_H_SEEN__
        case 'B': {
          // Print lib bfd error and clear it afteward
          bfd_error_type err =  bfd_get_error();
          if (err != (bfd_error_type) 0) {
            os << bfd_errmsg(err);
            bfd_set_error((bfd_error_type) 0);
          }
        }
#endif
      } 

      ++fptr;   // don't copy format characters
    } // if %

    if (!*fptr)
      break;

    os << (char) *fptr;
    ++fptr;
  } // while

  return os;
} 
