/*
 * $Id: dsApprc.h,v 1.1.1.1 2004/09/10 08:12:19 dsamersoff Exp $
 *
 */

#ifndef dsApprc_h
#define dsApprc_h

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <iostream>
#include <fstream>

#include <dsHashTable.h>
#include <dsSmartException.h>
#include <dsSlightFunctions.h>
#include <dsStrstream.h>

#define MAX_INCLUDE_LEVEL 10

#define GRC_ADD(k,v)   dsApprc::global_rc()->add(k,v)

#define GRC_GET(k)     dsApprc::global_rc()->getstring(k)
#define GRC_GETI(k)    dsApprc::global_rc()->getint(k)
#define GRC_GETL(k)    dsApprc::global_rc()->getlong(k)
#define GRC_GETB(k)    dsApprc::global_rc()->getbool(k)
#define GRC_GETD(k)    dsApprc::global_rc()->getdouble(k)

#define GRC_PUSH(k)    dsApprc::global_rc()->push(k)
#define GRC_POP()      dsApprc::global_rc()->pop()

namespace libdms5
{

    DECLARE_EXCEPTION(key);
    DECLARE_EXCEPTION2(INI,key);

/** 
 *  This class is designed to
 *  support application resource file with conditional block, variables,
 *  and arrays of key.
 *  Each variable should be as key=value pair, <code> Server-startup-chdir=Yes</code>
 *  you can access it by absolute reference <code> GETB("Server-startup-chdir")</code> 
 *  or put into stack commopn prefix and then relate them 
 *  <p> 
 *  <code>
 *    PUSH("Server");<br>
 *    PUSH("startup");<br>
 *    GETB("-chdir");<br>
 *  </code>
 *  </p> 
 *
 *  It's also possible to use conditional blocks and variables:
 *  <p>
 *  <b> Inside your program:</b><br>
 *  <code>
 *   ADD("Debug","Yes");<br>
 *   ADD("TesChdir","No");<br>
 *  </code>  
 *  <b> Inside resource file:</b><br>
 *  </code>
 *  .if $(Debug) == Yes && $(TestChdir)== NO<br>
 *   Server-startup-chdir=No<br>
 *  .endif<br>
 *   My-key=$(Debug)/name<br>
 *   My-env-key=/usr/bin:$$(PATH)<br>
 *   My-array-key[]=button1<br>
 *   My-array-key[]=button2<br>
 *   My-array-key[]=button3<br>
 *  </code>
 *  </p> 
 *  <p>
 */

    class dsApprc
    {
    protected:
        dsHashTable *_h;

        int _recursion_cnt;

        char *_keyst;
        int  _kslen;
        char _key_delim;

        int expand_condition(char *cond);
        char *expand_var(char *ptr, int *vlen = 0);

        void _add(char *line);
        void _add2(const char *key, const char *val);

        char *trim(char *src);
        void undefined_key_exception(const char *key);
    public:

/**
 *  Build object, but doesn't load rc file
 */
        dsApprc(); 

/**
 *  Buid object and load file 
 *  @param filename name of file to load
 *  @throws keyException thrown if file contains reference to unknown keys
 */
        dsApprc(const char *filename); 

/**
 *  Buid object and load stream 
 *  @param  is stream to load
 *  @throws keyException thrown if file contains reference to unknown keys
 */
        dsApprc(std::istream& is); 

        ~dsApprc();
 /**
  *  Convenience interface for the case of the only application-wide rc file
  */
        static dsApprc* global_rc();

/**
 *  Load and parse data from file
 *  already loaded data will be overwritten
 *
 *  @param filename name of file to load
 *  @throws INIException thrown if file contains reference to undefined key
 */
        void load(const char *filename); 

/**
 *  Load and parse data from stream
 *  already loaded data will be overwritten
 *
 *  @param  is stream to load
 *  @throws INIException thrown if file contains reference to undefined key
 */

        void load(std::istream& is);

/**
 *  Output existing keys to stream
 *  @param os - output stream
 */
        void dump(std::ostream& os);

/**
 *  Set delemiter of key parts. 
 *  Default is hiphen '-'
 *  
 *  @param delim delimiter of key parts. 
 */
        void set_delimiter( char delim );

/**
 *  Add  key and value. 
 *  Already defined keys will be redefined.
 *
 *  @param key   key to add
 *  @param value value to add 
 */
        void add(const char *key, const char *val);

/**
 *  Add  key and value. 
 *  Already defined keys will be redefined.
 *
 *  @param key   key to add
 *  @param value value to add 
 */
        void add(const char *key, int val);

/**
 *  Find a key 
 *
 *  @param key  key to find
 *  @return  pointer to value or <b>NULL</b> if key not defined  
 */
        char *seek(const char *key);

/**
 *  Find an element of array 
 *
 *  @param key  key to find
 *  @param idx  array index
 *  @return  pointer to value or <b>NULL</b> if key not defined  
 */
        char *seek(const char *key, int idx);

/**
 *  Push common prefix into key stack
 *  All keys begins from <b>-</b> will refer to stack 
 *
 *  @param key common prefix to push
 */
        void push(const char *key);

/**
 *  Push array index into key stack to refer array item
 *
 *  @param array_key array index
 */
        void push(int array_key);

/**
 *  Remove topmoust common prefix from stack
 *
 *  @throws keyException thrown if stack underflow
 */
        void pop();

/** 
 * Find a key, convert it's value to boolean. 
 * Recognised formats <b>Yes/No</b>, <b>True/False</b>,<b>1/0</b>
 * <i>See Also: GETB()</i>
 *  
 * @param key key to find
 * @throws INIException thrown if key not defined 
 */
        int    getbool(const char *key);

/** 
 * Find a key, convert it's value to boolean. 
 * Recognised formats <b>Yes/No</b>, <b>True/False</b>,<b>1/0</b>
 * if key not defined return default value
 * <code>
 * return getbool("Permit", getbool("Permit-default"));
 * </code>
 * 
 * @param key key to find
 * @param defval default value
 * @return default value 
 */
        int    getbool(const char *key, int defval);

/**
 *  Find a key, convert it's value to long.
 * <i>See Also: GETI()</i>
 *
 * @param key key to find
 * @throws INIException thrown if key not defined
 */
        int getint(const char *key);

/**
 * Find a key, convert it's value to long.
 * if key not defined return default value
 *
 * @param key key to find
 * @param defval default value
 * @return default value
 */
        int getint(const char *key, int defval);

/**
 *  Find a key, convert it's value to long. 
 * <i>See Also: GETL()</i>
 * 
 * @param key key to find
 * @throws INIException thrown if key not defined 
 */
        long   getlong(const char *key);

/**
 * Find a key, convert it's value to long. 
 * if key not defined return default value
 * 
 * @param key key to find
 * @param defval default value
 * @return default value 
 */
        long   getlong(const char *key, long defval);

/**
 * Find a key, convert it's value to double. 
 * <i>See Also: GETD()</i>
 * 
 * @param key key to find
 * @throws INIException thrown if key not defined 
 */
        double getdouble(const char *key);

/**
 * Find a key, convert it's value to double. 
 * if key not defined return default value
 * 
 * @param key key to find
 * @param defval default value
 * @return default value 
 */
        double getdouble(const char *key, double defval);

/**
 * Find a key, return found value
 * <i>See Also: GET()</i>
 * 
 * @param key key to find
 * @throws INIException thrown if key not defined 
 */
        const char * getstring(const char *key);

/**
 * Find an array key, return found value
 * <i>See Also: GET()</i>
 * 
 * @param key key to find
 * @param idx array index
 * @throws INIException thrown if key not defined 
 */
        const char *getstring(const char *key, int idx);

/**
 * Find a key, return found value. 
 * if key not defined return default value
 * 
 * @param key key to find
 * @param defval default value
 * @return default value 
 */
        const char * getstring(const char *key, const char *defval);

/**
 * Return maximum available key index i.e. hash table size
 * @return maximum key index
 */

        int walk_size();
/**
 * Return hash entry with given index or NULL if index is not valid
 *
 * @param idx hash entry index
 * @return key or null
 */
        const char *walk(int idx);

/**
 * Return whether the key exists or not
 * @param key key to check
 * @return true or false
 */
      bool has_key(const char *key);


      friend class dsGetoptMore;
  };


/* ======= Interfaces ========= */

    inline void dsApprc::undefined_key_exception(const char *key) 
    {
        if (_keyst && *key == _key_delim)
            throw INIException("Key (%s)%s not defined. ", _keyst, key  );
        else
            throw INIException("Key %s not defined. ", key );
    }

    inline void dsApprc::set_delimiter( char delim )
    {  _key_delim = delim; 
    }

    inline void dsApprc::add(const char *key, int val) 
    { char tmp[16]; 
        sprintf(tmp,"%d",val); 
        add(key,tmp); 
    }

    inline int dsApprc::getbool(const char *key)
    {  char *sp;
        if ( !(sp = seek(key)) )
            undefined_key_exception(key);
        return( *sp == 'y' ||  *sp == 'Y' || *sp == 't' ||  *sp == 'T' || *sp == '1' );
    }         

    inline int dsApprc::getbool(const char *key, int defval)
    {   char *sp = seek(key);
        return(sp) ? ( *sp == 'y' ||  *sp == 'Y' || *sp == 't' ||  *sp == 'T' || *sp == '1' ) : defval;
    }         

    inline int dsApprc::getint(const char *key)
    {   char *sp;
        if (!(sp = seek(key)))
            undefined_key_exception(key);
        return atoi(sp);
    }

    inline int dsApprc::getint(const char *key, int defval)
    {   char *sp = seek(key);
        return (sp) ? atol(sp) : defval;
    }


    inline long dsApprc::getlong(const char *key)
    {  char *sp;
        if ( !(sp = seek(key)) )
            undefined_key_exception(key);
        return atol( sp );
    }

    inline long dsApprc::getlong(const char *key, long defval)
    {  char *sp = seek(key);
        return(sp) ? atol( sp ) : defval;
    }

    inline double dsApprc::getdouble(const char *key)
    {  char *sp;
        if ( !(sp = seek(key)) )
            undefined_key_exception(key);
        return atof( sp );
    }

    inline double dsApprc::getdouble(const char *key, double defval)
    {  char *sp = seek(key);
        return(sp) ? atof( sp ) : defval;
    }

    inline const char *dsApprc::getstring(const char *key)
    {   char *sp;
        if ( !(sp = seek(key)) )
            undefined_key_exception(key);
        return sp;
    }

    inline const char *dsApprc::getstring(const char *key, const char *defval)
    {   char *sp = seek(key);
        return(sp) ? sp : defval;
    }

    inline const char *dsApprc::getstring(const char *key, int idx)
    {   char *sp;
        if ( !(sp = seek(key, idx)) )
            undefined_key_exception(key);
        return sp;
    }

    inline int dsApprc::walk_size() {
      return _h->HSize();
    }

    inline const char *dsApprc::walk(int idx)
    {
      dsHashTableItem *tmp;
      if (idx >= _h->HSize()) {
        return NULL;
      }

      if ((tmp = _h->walk(idx)) == NULL) {
        return NULL;
      }

      return tmp->key;
    }

    inline bool dsApprc::has_key(const char *key) {
       return seek(key) != NULL;
    }

};

#endif

