/*
 * $Id: dsStrstream.h,v 1.1.1.1 2004/09/10 08:12:19 dsamersoff Exp $
 */
#ifndef dsStrstream_h
#define dsStrstream_h

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/**
 * \class dsStrstream
 * \brief This is alternative implementation of std::strstream
 *
 *  This class is designed to provide fast, convenient and
 *  not depricated implementation of strstream. I belive removing
 *  strstream from standard library is not a good idea because lots of code
 *  already use it and most of system and database APIs require c-string as
 *  parameters. 
 *    I tried to compensate weakness of standard implementation:
 *   
 *   \b str() always return zero terminated string, but terminating
 *      zero is out of egptr(). 
 *      It allows code like  
 *      <code>
 *       for(int i=0; i < 26; ++i)
 *       {  dstr << '/' << (char) i + 'A';
 *          mkdir( dstr.str(), 0666 );
 *       }
 *      </code>
 *
 *   \b str() function doesn't call freeze()
 *   IMHO, returning pointer to dynamic string is bad programming 
 *   parctice, from other side invalid pointer can be located easier, then 
 *   memory leaks caused by freezed stream.
 * 
 *  \b clear() Clear function returns dynamic stream to initial state and reset get 
 *             and put pointers for static stream. 
 *  
 *  \b setseg(size_t newSize)
 *             Allow manage buffer increment at runtime, larger value increase
 *             speed, small value decrease memory consumption
 *
 *  \b seekp(n,ios::end) do the same as ios::cur. 
 *                      IMHO, there is now such thing as end of strstream. 
 *
 * \b Stream configuration section 
 *   (change it with care because most of libdms4 internals 
 *    require  _DS_AUTOTERMINATE is turned on and _DS_ENABLE_AUTOFREEZE turned off)

 *    Stream configuration contains a number of defines controlling behavioure of 
 *    stream.
 *     _DS_AUTOTERMINATE       -  initialize allocated buffer by zero, it can slow down
 *                                overflow, but keppi buffer always terminated.
 *     _DS_ENABLE_AUTOFREEZE  - str() will call freeze(), the same way as std:strstream
 *     does
 *     _DS_USE_SGI_SEEK_ROUTINE - use strstreambuf::seekoff() written by SGI, 
 *     and used in libstdc++
 *
 *
 *     _DS_ADAPTIVE_INCREMENT  - add N*_DS_SEGMENT_SIZE on each overflow instead of 
 *                               _DS_SEGMENT_SIZE where N is number of expansions. 
 *     _DS_SEGMENT_SIZE        -  default value of buffer increment segment.                         
 */
 


/*
 * Stream config 
 */

// #define _DS_ENABLE_AUTOFREEZE     
// #define _DS_USE_SGI_SEEK_ROUTINE 
#define _DS_AUTOTERMINATE 

#define _DS_ADAPTIVE_INCREMENT  
#define _DS_SEGMENT_SIZE        64

/**/

#define _DS_IO_FROZEN    0x1
#define _DS_IO_STATIC    0x2
#define _DS_IO_CONSTANT  0x4

#define _DS_IO_EOF    -1
#define _DS_IO_NOT_EOF 0

#define _DS_IO_NOTMOVABLE (_DS_IO_FROZEN | _DS_IO_STATIC | _DS_IO_CONSTANT)

/*
typedef streamoff streamoff;
typedef streampos streampos;
typedef streamsize streamsize;
*/

typedef void *(*p_alloc)(size_t);
typedef void (*p_free)(void *);

namespace libdms5
{
    class dsStrstreambuf : public std::streambuf 
    {
        long  _flags;
        p_alloc _palloc;
        p_free  _pfree;

        size_t _seg_size;
        int _overflow_cnt;

        char *_pbeg, *_gnext;
        std::streampos _pos_n;


        std::streampos _seekoff_p(std::streamoff off, std::ios::seekdir dir);
        std::streampos _seekoff_g(std::streamoff off, std::ios::seekdir dir);

    public:

        explicit  dsStrstreambuf(size_t initial_size = _DS_SEGMENT_SIZE);
        dsStrstreambuf( const dsStrstreambuf* src );
        dsStrstreambuf( p_alloc cust_alloc, p_free cust_free );
        dsStrstreambuf(char *gnext, std::streampos n, char *pbeg );
        dsStrstreambuf(const char *gnext, std::streampos n );


        virtual ~dsStrstreambuf();

        int frozen();
        void freeze(bool n = true);

        size_t pcount();
        char *str();

        int overflow(int ch);
        int underflow();
        int pbackfail(int c);

        std::streampos seekoff(std::streamoff off, std::ios::seekdir dir, std::ios::openmode mode);
        std::streampos seekpos(std::streamoff off, std::ios::openmode mode){ return seekoff(off, std::ios::beg, mode);}

        /* Extensions */
        void setseg(size_t new_size = _DS_SEGMENT_SIZE);
        void clear();

    };

    class dsStrstream :  public std::iostream
    {
        dsStrstreambuf* _sb;

    public:
        dsStrstream();
        dsStrstream(const dsStrstream& s );
        dsStrstream(char *s, int n, std::ios::openmode mode = std::ios::out | std::ios::in);
        virtual ~dsStrstream();

        dsStrstreambuf *rdbuf() const;

        size_t pcount(); 
        char *str(); 

        void freeze(bool n = true);

        void clear();
    };  


#ifdef USE_DMS_IOSTREAMS

    class dsiStrstream : public std::istream
    {
        dsStrstreambuf* _sb;

    public:
        explicit dsiStrstream(char* s);
        explicit dsiStrstream(const char* s);
        dsiStrstream(const dsiStrstream& s );
        dsiStrstream(char* s, std::streampos n);
        dsiStrstream(const char* s, std::streampos n);
        virtual ~dsiStrstream();

        dsStrstreambuf* rdbuf() const;
        char* str();            

        void clear();
    };

    // Class ostrstream
    class dsoStrstream : public std::ostream
    {
        dsStrstreambuf* _sb;

    public:
        dsoStrstream();
        dsoStrstream(const dsoStrstream& s );
        dsoStrstream(char * s, int n, std::ios::openmode = std::ios::out);
        virtual ~dsoStrstream();

        dsStrstreambuf* rdbuf() const;

        void freeze(bool n = true);

        char* str();
        size_t pcount();      

        void clear();
    };
#endif

}

#endif
