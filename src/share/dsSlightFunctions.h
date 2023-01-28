/*
 * $Id: dsSlightFunctions.h,v 1.1.1.1 2004/09/10 08:12:19 dsamersoff Exp $
 */


#ifndef dsSlightFunctions_h
#define dsSlightFunctions_h 

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>

#include <dsSmartException.h>


/* some usefull typedefs usually defined by OS */
typedef unsigned short ushort;
typedef unsigned long  ulong;
typedef unsigned int   uint;

namespace libdms5
{

DECLARE_EXCEPTION(dsSlightFunction);

/** 
 * This function convert decimal ascii \e src 
 * to unsigned long if first passed character is 
 * not numeric throws exception
 * @param src - conversion source 
 * @param n  - read no more than n bytes from stream (0 no limit)
 *             throw exception if n bytes couldn't be converted
 *
 * @return conversion result
 * @throw dsSlightFunctionException if conversion couldn't be done
 */
    ulong dsAtoul(const char *src, int n = 0);

/**
 * convert ascii string to unsigned long
 * @param is - stream to read from
 * @param n  - read no more than n bytes from stream (0 no limit)
 *             throw exception if n bytes couldnt be converted
 * @param consumed - return number of bytes consumed from input (could be 0)
 *
 * @return conversion result
 * @throw dsSlightFunctionException if conversion couldn't be done
 */
    ulong dsAtoul(std::istream& is, int n = 0, int *consumed = 0 );


/**
 * This function returns dynamic copy of substring 
 * of passed string.
 * Some of systems luck this functions, other one call 
 * strlen inside it.
 * This version is safe to be used with mmap();
 * 
 * @param s - source string
 * @param len - substring length
 */
    char *dsStrndup(const char *s, int len);

/**
 * This function return dynamic copy of passed string.
 * It different from system one - call new[] instead of malloc()
 * and rise exception if passed string is 0
 *
 * @param s - source string
 */
    char *dsStrdup(const char *s);

    void *dsMemdup(const void *s, size_t size);

/**
 * Common string manipulation routines
 */

    char *dsStrnchr(char *s, char c, int len);

    bool dsStartsWith(const char *s, const char *prefix);
    bool dsEndsWith(const char *s, const char *suffix);

/**
 * Copy string.
 * if length of source string greater than size 
 * throw dsSlightFunctionException
 *
 * @param dest - destinagtion buffer
 * @param src  - source string
 * @param size - size of destination buffer
 * 
 * @throw dsSlightFunctionException if source or destination is null or 
 * length of source string greater than size
 */
    void dsSizeStrcpy(char *dest, const char *src, int size);

    char *dsZalloc(size_t size);    
/**/
    void dsDecodeBase64(std::ostream& os, char *ptr, int len);
    void dsDecodeBase64(char *out, int *outlen, char *ptr, int len);

    void dsEncodeBase64(std::ostream& dest, char *src, int len);

    void dsDecodeHex(std::ostream& os, const char *ptr, int len);
    void dsDecodeHex(char *out, const char *ptr, int len);

    void dsEncodeHex(std::ostream& os, const char *ptr, int len);

    void dsDecodeQ(char *out, int *outlen, char *ptr, int len);
    void dsDecodeQ(std::ostream& os, char *ptr, int len);

};

#endif
