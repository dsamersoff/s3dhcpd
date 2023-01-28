/* 
 * $Id: dsSmartException.h,v 1.1.1.1 2004/09/10 08:12:19 dsamersoff Exp $
 */
#ifndef dsSmartException_h
#define dsSmartException_h

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif

#include <dsStrstream.h>
#include <dsForm.h>


#define DECLARE_EXCEPTION(tag) class  tag##Exception:public libdms5::dsSmartException\
                        { public:\
                             tag##Exception(){ }\
                             tag##Exception(const char *format, ... )\
                               { va_list ap; va_start(ap, format); fire(#tag, (const char *) format, ap); va_end(ap); }\
                             tag##Exception(int xfacility, const char *format, ... )\
                               { va_list ap; va_start(ap, format); fire(xfacility, #tag, (const char *) format, ap); va_end(ap); }\
                        }


#define DECLARE_EXCEPTION2(tag, parent) class  tag##Exception:public parent##Exception\
                        { public:\
                             tag##Exception(const char *format, ... )\
                               { va_list ap; va_start(ap, format); fire(#tag, (const char *) format, ap); va_end(ap); }\
                             tag##Exception(int xfacility, const char *format, ... )\
                               { va_list ap; va_start(ap, format); fire(xfacility, #tag, (const char *) format, ap); va_end(ap); }\
                        }


#ifdef NO_DS_ASSERTS
# define ds_assert(a, b)
#else
# define ds_assert(a, b) if (!(a)){ throw libdms5::dsSmartException("Assert", "Assertion failed at %s:%d '%s'", __FILE__, __LINE__,(b) ); }
#endif

#define dsAssert(a, b) ds_assert(a,b)

#define BACKTRACE_SIZE 25

namespace libdms5
{

/**
 * Easy way to create and maintain your own exceptions
 */
    class dsSmartException {

    protected:
        dsStrstream _message; 
        dsStrstream _tag;
        int _facility;
#ifdef HAVE_EXECINFO_H
        void *_backtrace_buffer[BACKTRACE_SIZE];
        int _backtrace_nptrs;
#endif

    public:
        const char *msg() { return _message.str();}
        const char *tag() { return _tag.str();}

        dsSmartException();
        dsSmartException(const char * tag, const char *format, ... );
        dsSmartException(int facility, const char * tag, const char *format, ... );
        virtual ~dsSmartException();

        void fire(int facility, const char * tag, const char *format, va_list ap );

        void fire(const char *format, va_list ap );
        void fire(const char * tag, const char *format, va_list ap );

        virtual std::ostream& print(std::ostream& os = std::cerr);

        int operator ==(char *val){ return( strcmp(_tag.str(), val) == 0 );}  
        friend std::ostream &operator<<(std::ostream &stream, dsSmartException &e);
    };


    inline std::ostream &operator<<(std::ostream &stream, dsSmartException &e) {
        return  e.print(stream);
    }
};

#endif

