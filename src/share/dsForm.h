/*
 *  $Id: dsForm.h,v 1.1.1.1 2004/09/10 08:12:19 dsamersoff Exp $
 */

#ifndef dsForm_h
#define dsForm_h

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <iostream>
#include <iomanip>

namespace libdms5{

  class dsForm
  {
  public:

    static std::ostream& form(std::ostream& os, const char *format,   ... );
    static std::ostream& vform(std::ostream& os, const char *format, va_list ap );
  };

};


#endif
