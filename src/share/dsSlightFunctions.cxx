/*
 * $Id: dsSlightFunctions.cxx,v 1.1.1.1 2004/09/10 08:12:19 dsamersoff Exp $
 *
 */

#include <dsSlightFunctions.h>

using namespace std;

/* convert src to unsigned long, rise
 * AutoStrException if conversion couldn't be done.
 * Stop on first non-numeric character
 */ 

ulong libdms5::dsAtoul(const char *src, int n)
{
   ulong num = 0;
   const char *s = src;
   if (!s || *s < '0' || *s > '9')
       throw dsSlightFunctionException("dsAtol (1): Conversion can't be done");
 
   while(s && *s >= '0' && *s <='9' )
   {
        num = (num*10) + (*s - '0');
   }

   return num;
}

/**
 * convert ascii string to unsigned long
 * @param is - stream to read from
 * @param n  - read no more than n bytes from stream (0 no limit)
 *             throw exception if n bytes couldnt be converted
 * @param consumed - return number of bytes consumed from input (could be 0)
 */

ulong libdms5::dsAtoul(istream& is, int n, int *consumed )
{
   ulong num = 0;
   char ch;
   int i = 0;

   while(1)
   {
       is >> ch;   

       if (ch < '0' || ch > '9')
       {
           /* at least one or n digit required */
            if (n > 0 || i == 0 )
            {
              if (consumed)
                    *consumed = i;
              throw dsSlightFunctionException("dsAtol (2): Conversion can't be done");
            }
              
        break;
      }

       if (n > 0 && i > n)
           break;

       ++i;

       num = (num*10) + (ch - '0');
   }

   if (consumed)
       *consumed = i;

   return num;
}

void libdms5::dsSizeStrcpy(char *dest, const char *src, int size)
{
    if (!src)
       throw dsSlightFunctionException("dsSizeStrcpy: Attempt to copy null pointer.");

    for(int i = 0; i < size; ++i)
    {
       dest[i] = src[i];
       if (src[i] == 0)
                  return;
    }

    dest[size] = 0;
    
    throw dsSlightFunctionException("dsSizeStrcpy: Attempt to overload variable (%d)", size);
}

char *libdms5::dsStrndup(const char *s, int len)
{
   int q;
   for(q = 0; q < len && s[q]; ++q);
   char *tmp = new char[q+2];
   memmove(tmp,s, q); tmp[q] = 0;
   return tmp;
}

char *libdms5::dsStrdup(const char *s)
{  int q;
   for(q = 0; s[q]; ++q);
   char *tmp = new char[q+2];
   memmove(tmp,s, q); tmp[q] = 0;
   return tmp;
}

char *libdms5::dsStrnchr(char *s, char c, int len)
{
  int i;
  for (i=0; i <= len; ++i) {
     if (s[i] == c) return &(s[i]);
  }
  return 0;
}


bool libdms5::dsStartsWith(const char *s, const char *prefix) {
  int len = strlen(prefix);
  return (strncmp(s,prefix,len) == 0);
}

bool libdms5::dsEndsWith(const char *s, const char *suffix) {
   int st_len = strlen(s);
   int su_len = strlen(suffix);
   if (su_len > st_len) {
     return false;
   }

   return (strcmp(s+(st_len-su_len), suffix) == 0);
}


void *libdms5::dsMemdup(const void *s, size_t size)
{
    char *tmp = new char[size];
    memmove(tmp,s, size);
    return (void *)tmp;
}

char *libdms5::dsZalloc(size_t size)
{
   char *tmp = new char[size+1];
      memset(tmp, 0, size+1);
   return tmp;
}


/* base64 and quoted printable conversion funtions */

#define XX '\xFE'
#define ES '\xFD'

static char index_64[256] = {
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,62, XX,XX,XX,63,
    52,53,54,55, 56,57,58,59, 60,61,XX,XX, XX,ES,XX,XX, 
    XX, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
    15,16,17,18, 19,20,21,22, 23,24,25,XX, XX,XX,XX,XX,
    XX,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
    41,42,43,44, 45,46,47,48, 49,50,51,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
};

#define CHAR64(c)  (index_64[(unsigned char)(c)])


void libdms5::dsDecodeBase64(ostream& os, char *ptr, int len)
{
    int c1, c2, c3, c4;
    char d;
    char *s = ptr, *eptr = s+len;

    while (s < eptr)
    {
        while (s < eptr && CHAR64(*s) == XX) ++s;
        c1 = CHAR64(*s); ++s;

        while (s < eptr && CHAR64(*s) == XX) ++s;
        c2 = CHAR64(*s); ++s;

        while (s < eptr && CHAR64(*s) == XX) ++s;
        c3 = CHAR64(*s); ++s;

        while (s < eptr && CHAR64(*s) == XX) ++s;
        c4 = CHAR64(*s); ++s;


        if (c1 == ES || c2 == ES) break;

        d = (c1 << 2) | ( (c2 & 0x30) >> 4);
        os.put(d);

        if ( c3 == ES ) break;
        d = ((c2 & 0x0F) << 4) | ((c3 & 0x3C) >> 2); 
        os.put(d);

        if ( c4 == ES ) break;
        d = ((c3 & 0x03) << 6) | c4; 
        os.put(d);

    } // while
}

void libdms5::dsDecodeBase64(char *out, int *outlen, char *ptr, int len)
{
    int c1, c2, c3, c4;
    char *d = out;
    char *s = ptr, *eptr = s+len;

    while (s < eptr)
    {
        while (s < eptr && CHAR64(*s) == XX) ++s;
        c1 = CHAR64(*s); ++s;

        while (s < eptr && CHAR64(*s) == XX) ++s;
        c2 = CHAR64(*s); ++s;

        while (s < eptr && CHAR64(*s) == XX) ++s;
        c3 = CHAR64(*s); ++s;

        while (s < eptr && CHAR64(*s) == XX) ++s;
        c4 = CHAR64(*s); ++s;


        if (c1 == ES || c2 == ES) break;

        *d = (c1 << 2) | ( (c2 & 0x30) >> 4); ++d;

        if ( c3 == ES ) break;
        *d = ((c2 & 0x0F) << 4) | ((c3 & 0x3C) >> 2); ++d;

        if ( c4 == ES ) break;
        *d = ((c3 & 0x03) << 6) | c4; ++d;

    } // while

  --d;
  if (outlen) *outlen = d - out;

} // sub


/* base 64 encoding */
unsigned char dtable[]=
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void libdms5::dsEncodeBase64(ostream& dest, char *src, int len)
{
    char *s = src;
    char *eptr = src+len;

    while (s != eptr)
    {
        unsigned char ig[3];
        ig[0] = 0; ig[1] = 0; ig[2] = 0;

        int n;

        for ( n = 0; n < 3; ++n)
        {
            if ( ! *(s+n) )
                break;
            ig[n] = *(s+n);
        }

        dest <<  dtable[   ig[0]  >> 2 ];
        dest <<  dtable[ ((ig[0] &   3) << 4) | (ig[1] >> 4) ];

        dest << (unsigned char)  ( (n > 1) ? dtable[ ((ig[1] & 0xF) << 2) | (ig[2] >> 6) ] : '='  );
        dest << (unsigned char)  ( (n > 2) ? dtable[   ig[2] & 0x3F ] : '=' );


        if (n < 3)
            break;
        s+= 3;
    }
}

void libdms5::dsDecodeHex(ostream& os, const char *ptr, int len)
{
    char d;
    const char *s = ptr, *eptr = s+len;

    while (s < eptr)
    {
        d =  ((*s <= '9') ? *s - '0' : *s - 'A'+10)* 16 +
             (*(s+1) <= '9' ? *(s+1) - '0' : *(s+1) - 'A'+10);
        s+=2;
        os.put(d);
    } // while

}

void libdms5::dsDecodeHex(char *out, const char *ptr, int len)
{
    char *d = out;
    const char *s = ptr, *eptr = s+len;

    while (s < eptr)
    {
        *d =  ((*s <= '9') ? *s - '0' : *s - 'A'+10)* 16 +
              (*(s+1) <= '9' ? *(s+1) - '0' : *(s+1) - 'A'+10);
        s+=2;
        ++d;
    } // while

    *d = 0;
}

void libdms5::dsEncodeHex(ostream& os, const char *ptr, int len)
{
    for (int i = 0; i < len; ++i )
    {
        char b1 = ((ptr[i]  >> 4) & 0xF) >= 10 ? (((ptr[i]  >> 4) & 0xF) - 10) + 'A' : ((ptr[i]  >> 4) & 0xF) + '0';
        char b2 = (ptr[i] & 0xF) >= 10 ? ((ptr[i] & 0xF) - 10) + 'A' : (ptr[i] & 0xF) + '0';   

        os <<  b1 << b2;
    }

} 



void libdms5::dsDecodeQ(char *out, int *outlen, char *ptr, int len)
{
    char *d = out;
    char *s = ptr, *eptr = s+len;

    while (s < eptr)
    {
        switch (*s)
        {
            case '=' :
                if ( *(s+1) == '\n')
                {
                    s+=2; continue;
                }

                *d =  (*(s+1) <= '9' ? *(s+1) - '0' : *(s+1) - 'A'+10)* 16 +
                      (*(s+2) <= '9' ? *(s+2) - '0' : *(s+2) - 'A'+10);
                s+=2;

                break;
            case '_' :
                *d = ' ';
                break;

            default:
                *d = *s;
        }

        ++s; ++d;
    } // while

    --d;

    if (outlen) *outlen = d - out;
}


void libdms5::dsDecodeQ(ostream& os, char *ptr, int len)
{
    char d;
    char *s = ptr, *eptr = s+len;

    while (s < eptr)
    {
        switch (*s)
        {
            case '=' :
                if ( *(s+1) == '\n')
                {
                    s+=2; continue;
                }

                d =  (*(s+1) <= '9' ? *(s+1) - '0' : *(s+1) - 'A'+10)* 16 +
                     (*(s+2) <= '9' ? *(s+2) - '0' : *(s+2) - 'A'+10);
                s+=2;

                break;
            case '_' :
                d = ' ';
                break;

            default:
                d = *s;
        }

        ++s; 
        os.put(d);
    } // while

}
