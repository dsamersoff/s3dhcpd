/*
 * $Id: dsStrstream.cxx,v 1.1.1.1 2004/09/10 08:12:19 dsamersoff Exp $
 *
 */

#include <dsStrstream.h>

using namespace std;
using namespace libdms5;


///__int64 const std::_Fpz = 0;

dsStrstreambuf::dsStrstreambuf(size_t size)
: streambuf() {
    _palloc=malloc;
    _pfree=free;

    _flags = 0;
    _overflow_cnt =0;
    setseg();

    _gnext = 0;
    _pos_n = (streampos) size;
    _pbeg  = 0; 

    char *ptr = (char *) _palloc(size+1);
#ifdef _DS_AUTOTERMINATE
    memset(ptr, 0, size+1);
#endif

    setp(ptr, ptr + size);
    setg(ptr, ptr, ptr);

}

dsStrstreambuf::dsStrstreambuf( p_alloc cust_alloc, p_free cust_free ) {
    _palloc=cust_alloc;
    _pfree=cust_free;
    _flags = 0;

    _overflow_cnt =0;
    setseg();
    _gnext = 0;
    _pos_n = (streampos) _seg_size;
    _pbeg  = 0; 


    char *ptr = (char *) _palloc(_seg_size+1);
#ifdef _DS_AUTOTERMINATE
    memset(ptr, 0, _seg_size+1);
#endif
    setp(ptr, ptr + _seg_size);
    setg(ptr, ptr, ptr);
}

dsStrstreambuf::dsStrstreambuf(char *gnext, streampos n, char *pbeg ) {
    _palloc=malloc;
    _pfree=free;

    _flags = _DS_IO_STATIC;
    _overflow_cnt =0;
    setseg();


    /* store paremeters for clean functions */
    _gnext = gnext;
    _pbeg = pbeg; 

    size_t N = (size_t) n;

    if (gnext)
        N = (N > 0) ? N : ((N == 0) ? strlen(gnext) : INT_MAX);

    _pos_n = N;

    if (pbeg) {
        setg(gnext, gnext, pbeg);
        setp(pbeg, pbeg + N);
    } else {
        setg(gnext, gnext, gnext + N);
    }

}

dsStrstreambuf::dsStrstreambuf(const char *gnext, streampos n ) {
    _palloc=malloc;
    _pfree=free;

    _flags = _DS_IO_STATIC | _DS_IO_CONSTANT;
    _overflow_cnt =0;
    setseg();

    /* store paremeters for clean functions */
    _gnext = (char *) gnext;
    _pbeg  = 0; 


    size_t N = (size_t) n;

    if (gnext)
        N = (N > 0) ? N : ((N == 0) ? strlen(gnext) : INT_MAX);

    _pos_n = N;

    setg( (char *) gnext, (char *) gnext, (char *) gnext + N);
}

/* Support for copy constructors */

dsStrstreambuf::dsStrstreambuf(const dsStrstreambuf * src) {
    _flags        = src->_flags;
    _overflow_cnt = src->_overflow_cnt;
    _seg_size     = src->_seg_size;
    _pos_n        = src->_pos_n;
    _gnext        = src->_gnext;
    _pbeg         = src->_pbeg;

    _palloc       = src->_palloc;
    _pfree        = src->_pfree;

    streamsize  ptr_size =  src->epptr() - src->pbase();

    if ( !(_flags & _DS_IO_STATIC) ) {  // dynamic stream
        char *ptr = (char *) _palloc( ptr_size + 1 );
        memcpy(ptr, src->pbase(), ptr_size + 1 );

        setp(ptr, ptr + ptr_size);
        setg(ptr, ptr + (src->eback() - src->gptr()), ptr);
        pbump(src->pptr() - src->pbase());
    } else { // static stream
        char *ptr = src->pbase();
        setp(ptr, ptr + ptr_size);
        setg(ptr, ptr+(src->eback() - src->gptr()), ptr);
        pbump(src->pptr() - src->pbase());
    }

    freeze(false); // new created stream always unfreezed
}

dsStrstreambuf::~dsStrstreambuf() {
    char *ptr = pbase();

    if (ptr && 
        !(_flags & _DS_IO_FROZEN) && 
        !(_flags & _DS_IO_STATIC) )
        _pfree((void *) ptr);
}

void dsStrstreambuf::clear() {
    if ( !(_flags & _DS_IO_FROZEN) ) { // we don't touch frozen buffer  
        _overflow_cnt =0;
        setseg();

        if ( !(_flags & _DS_IO_STATIC) ) {  // dynamic stream
            _pfree( pbase() );

            char *ptr = (char *) _palloc( (size_t)_pos_n +1);
#ifdef _DS_AUTOTERMINATE
            memset(ptr, 0, (size_t)_pos_n+1);
#endif
            setp(ptr, ptr + _pos_n);
            setg(ptr, ptr, ptr);
        } else { // static stream
            if (_pbeg) {
                setg(_gnext, _gnext, _pbeg);
                setp(_pbeg, _pbeg + _pos_n);
            } else {
                setg( _gnext, _gnext, _gnext + _pos_n);
            }
        }
    }
}

void dsStrstreambuf::setseg(size_t new_size) {
    _seg_size = new_size;
}

int dsStrstreambuf::frozen() {
    return _flags &  _DS_IO_FROZEN; 
}

void dsStrstreambuf::freeze(bool n) {
    if (n)
        _flags |=   _DS_IO_FROZEN;
    else
        _flags &= ~  _DS_IO_FROZEN;
}

size_t dsStrstreambuf::pcount() {
    return pptr() ? pptr() - pbase() : 0; 
}

char *dsStrstreambuf::str() {
#ifdef _DS_ENABLE_AUTOFREEZE
    freeze(true);
#endif
    return eback();
}


int dsStrstreambuf::overflow(int ch) {
    if (ch == _DS_IO_EOF )
        return _DS_IO_NOT_EOF;

/* Expanding buffer */
    if ( pptr() == epptr() 
         && !(_flags & _DS_IO_NOTMOVABLE) && _seg_size > 0) {

        int old_size= epptr() - pbase();
        char *old_buf = pbase(); 
        int old_get_offset = (gptr()) ? gptr() - eback() : 0;

#ifdef _DS_ADAPTIVE_INCREMENT
        ++_overflow_cnt;
        int new_size = old_size+ _overflow_cnt * _seg_size;
#else 
        int new_size = old_size+_seg_size;
#endif

        char *new_buf = (char *) _palloc(new_size+1);
        memcpy(new_buf, old_buf, old_size);
#ifdef _DS_AUTOTERMINATE
        memset(new_buf+old_size, 0, new_size+1 - old_size);
#endif
        setp(new_buf, new_buf + new_size);
        pbump(old_size);

        setg(new_buf,new_buf+old_get_offset, new_buf);

        _pfree(old_buf);
    }

    if (pptr() != epptr() ) {
        *pptr() = ch;
        pbump(1);

        return ch;
    }


    return _DS_IO_EOF;
}

int dsStrstreambuf::underflow() {
    if (gptr() == egptr() && pptr() && pptr() > egptr())
        setg(eback(), gptr(), pptr());

    if (gptr() != egptr())
        return(unsigned char) *gptr();
    else
        return _DS_IO_EOF;
}

int dsStrstreambuf::pbackfail(int c) {
    if (gptr() != eback()) {
        if (c == _DS_IO_EOF) {
            gbump(-1);
            return _DS_IO_NOT_EOF;
        } else {
            if ( !(_flags & _DS_IO_CONSTANT) ) {
                gbump(-1);
                *gptr() = c;
                return c;
            }
        }
    }

    return _DS_IO_EOF;
}


#ifndef _DS_USE_SGI_SEEK_ROUTINE 

streampos dsStrstreambuf::_seekoff_g(streamoff off, ios::seekdir dir) {
    streamoff newoff = 0;    //dir == ios::beg

    char* seekhigh = epptr() ? epptr() : egptr(); 
    char *seeklow  = eback();

    if (dir == ios::cur) {
        newoff = gptr()-seeklow;
    } else if (dir == ios::end) {
        newoff = seekhigh - seeklow; 
    }

    off += newoff;
    if (off < 0 || off > seekhigh - seeklow)
        return streampos(streamoff(-1));


    if (off <= egptr() - seeklow) {
        setg(seeklow, seeklow + off, egptr());
    } else if (off <= pptr() - seeklow) {
        setg(seeklow, seeklow + off, pptr());
    } else {
        setg(seeklow, seeklow + off, epptr());
    }

    return streampos(newoff);
} 

streampos dsStrstreambuf::_seekoff_p(streamoff off, ios::seekdir dir) {
    streamoff newoff = 0; // dir == ios::beg  

    char* seekhigh = epptr() ? epptr() : egptr(); 
    char *seeklow  = eback();

    if (dir == ios::cur) {
        newoff = pptr()-seeklow;
    } else if (dir == ios::end) {
        newoff = pptr() - seeklow;     // IMHO, there is no adecuate logic 
        //  newoff = seekhigh - seeklow; // for end of stream. treate ::end as ::cur 
    }

    off += newoff;
    if (off < 0 || off > seekhigh - seeklow)
        return streampos(streamoff(-1));

    if (seeklow + off < pbase()) {
        setp(seeklow, epptr());
        pbump(off);
    } else {
        setp(pbase(), epptr());
        pbump(off - (pbase() - seeklow));
    }

    return streampos(newoff);
}

streampos dsStrstreambuf::seekoff(streamoff off, ios::seekdir dir, ios::openmode mode) {
    streampos newoff = -1;

    if ((mode & ios::in) && pptr() ) {
        newoff = _seekoff_g(off,dir);
    }

    if (mode & ios::out) {
        newoff = _seekoff_p(off,dir);
    }

    return newoff;
}

#else // _DS_USE_SGI_SEEK_ROUTINE 

/*
 * This seek routine comes from SGI STL. GNU libstdc++ use the
 * it without significant changes. This version of seek perform somne additional 
 * checks and it seems to be safer than version writen by me.
 */

streampos dsStrstreambuf::seekoff(streamoff off, ios::seekdir dir, ios::openmode mode) {
    bool do_get = false;
    bool do_put = false;

    if ((mode & (ios::in | ios::out)) 
        == (ios::in | ios::out) &&
        (dir == ios::beg || dir == ios::end))
        do_get = do_put = true;
    else if (mode & ios::in)
        do_get = true;
    else if (mode & ios::out)
        do_put = true;

    // !gptr() is here because, according to D.7.1 paragraph 4, the seekable
    // area is undefined if there is no get area.
    if ((!do_get && !do_put) || (do_put && !pptr()) || !gptr())
        return streampos(streamoff(-1));

    char* seeklow  = eback();
    char* seekhigh = epptr() ? epptr() : egptr();

    streamoff newoff;
    switch (dir) {
    case ios::beg:
        newoff = 0;
        break;
    case ios::end:
        newoff = seekhigh - seeklow;
        break;
    case ios::cur:
        newoff = do_put ? pptr() - seeklow : gptr() - seeklow;
        break;
    default:
        return streampos(streamoff(-1));
    }

    off += newoff;
    if (off < 0 || off > seekhigh - seeklow)
        return streampos(streamoff(-1));

    if (do_put) {
        if (seeklow + off < pbase()) {
            setp(seeklow, epptr());
            pbump(off);
        } else {
            setp(pbase(), epptr());
            pbump(off - (pbase() - seeklow));
        }
    }

    if (do_get) {
        if (off <= egptr() - seeklow)
            setg(seeklow, seeklow + off, egptr());
        else if (off <= pptr() - seeklow)
            setg(seeklow, seeklow + off, pptr());
        else
            setg(seeklow, seeklow + off, epptr());
    }

    return streampos(newoff);
}

#endif // _DS_USE_SGI_SEEK_ROUTINE

/* ====================== strstream ================================ */

dsStrstream::dsStrstream()
: iostream( (streambuf *)0 ) {
    _sb = new dsStrstreambuf(_DS_SEGMENT_SIZE);
    init(_sb);
}

dsStrstream::dsStrstream(char *s, int n, ios::openmode mode)
: iostream( (streambuf *)0 ) {
    _sb = new dsStrstreambuf(s,n,  (mode & ios::app) ? s + strlen(s) : s);
    init(_sb);
}

dsStrstream::dsStrstream( const dsStrstream& src )
: iostream( (streambuf *)0 ) {
    _sb = new dsStrstreambuf(src.rdbuf());
    init(_sb);
}


dsStrstream::~dsStrstream() {
    if (_sb)
        delete _sb;
}

dsStrstreambuf *dsStrstream::rdbuf() const {
    return _sb; 
}

size_t dsStrstream::pcount() {
    return _sb->pcount(); 
}

void dsStrstream::clear() {
    _sb->clear(); 
}

char *dsStrstream::str() {
    return _sb->str(); 
}

void dsStrstream::freeze(bool n) {
    _sb->freeze(n); 
}

#ifdef USE_DMS_IOSTREAMS

/* ====================== istrstream ================================ */

dsiStrstream::dsiStrstream(char *s)
: istream((streambuf *)0 ) {
    _sb = new dsStrstreambuf(s,0);
    init(_sb);
}

dsiStrstream::dsiStrstream(const char *s)
: istream( (streambuf *)0 ) {
    _sb = new dsStrstreambuf(s,0);
    init(_sb);
}

dsiStrstream::dsiStrstream(char *s, streampos n)
: istream((streambuf *)0 ) {
    _sb = new dsStrstreambuf(s,n);
    init(_sb);
}

dsiStrstream::dsiStrstream(const char *s, streampos n)
: istream( (streambuf *)0 ) {
    _sb = new dsStrstreambuf(s,n);
    init(_sb);
}

dsiStrstream::dsiStrstream( const dsiStrstream& src )
: istream( (streambuf *)0 ) {
    _sb = new dsStrstreambuf(src.rdbuf());
    init(_sb);
}

dsiStrstream::~dsiStrstream() {
    if (_sb)
        delete _sb;
}

dsStrstreambuf *dsiStrstream::rdbuf() const {
    return _sb; 
}

char *dsiStrstream::str() {
    return _sb->str(); 
}

void dsiStrstream::clear() {
    _sb->clear(); 
}

/* ====================== ostrstream ================================ */

dsoStrstream::dsoStrstream()
: ostream((streambuf *)0 ) {
    _sb = new dsStrstreambuf(_DS_SEGMENT_SIZE);
    init(_sb);
}

dsoStrstream::dsoStrstream(char *s, int n, ios::openmode mode)
: ostream((streambuf *)0 ) {
    _sb = new dsStrstreambuf(s,n,  (mode & ios::app) ? s + strlen(s) : s);
    init(_sb);
}

dsoStrstream::dsoStrstream( const dsoStrstream& src )
: ostream( (streambuf *)0 ) {
    _sb = new dsStrstreambuf(src.rdbuf());
    init(_sb);
}


dsoStrstream::~dsoStrstream() {
    if (_sb)
        delete _sb;
}


dsStrstreambuf *dsoStrstream::rdbuf() const {
    return _sb; 
}

void dsoStrstream::clear() {
    _sb->clear(); 
}

size_t dsoStrstream::pcount() {
    return _sb->pcount(); 
}

char *dsoStrstream::str() {
    return _sb->str(); 
}

void dsoStrstream::freeze(bool n) {
    _sb->freeze(n); 
}
#endif
