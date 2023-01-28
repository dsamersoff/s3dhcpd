/* 
 *  
 * $Id: dsApprc.cxx,v 1.1.1.1 2004/09/10 08:12:19 dsamersoff Exp $
 *
 */
#include <dsApprc.h>

using namespace std;
using namespace libdms5;


#define KEY_PART_DELIMITER  '-'
#define MAX_RC_LINE        1024

dsApprc::dsApprc()
{
  _h = new dsHashTable(256);
  _key_delim = KEY_PART_DELIMITER;
  _keyst = 0; 
  _kslen = 0;
  _recursion_cnt = 0;

}

dsApprc::dsApprc(const char *fname)
{
  _h = new dsHashTable(256);
  _key_delim = KEY_PART_DELIMITER;
  _keyst = 0; 
  _kslen = 0;
  _recursion_cnt = 0;
  
  load(fname);
}

dsApprc::dsApprc(istream& is)
{
  _h = new dsHashTable(256);
  _key_delim = KEY_PART_DELIMITER;
  _keyst = 0; 
  _kslen = 0;
  _recursion_cnt = 0;  

  load(is);
}


dsApprc::~dsApprc()
{

    dsHashTableItem *tmp;

    for (int i =0; i < _h->HSize(); ++i)
    {
        if ( (tmp =_h->walk(i)) )
        {
            delete tmp;
        }
    }

    if (_keyst)
        delete _keyst;

    if (_h)
        delete _h;
}

dsApprc *dsApprc::global_rc() {
  static dsApprc *apprc = NULL;
  if (apprc == NULL) {
    apprc = new dsApprc();
  }
  return apprc;
}

/* in-place eat up leading and trailing spaces */
char *dsApprc::trim(char *src)
{
    char *s = src;
    while ( *s && (*s == ' ' || *s == '\t' || *s == '\n' ) ) ++s;
    if (s != src)
    {
        memmove(src,s,strlen(s)+1);
    }

    char *se = src+strlen(src)-1;
    while ( se != src && (*se == ' ' || *se == '\t' || *se == '\n') )
    {
        -- se;
    }

    *(se+1) = 0;

    return src;
}

#define WORD_BRAKES "\t ,;:!?$#@/\"{(})\\"
 
char *dsApprc::expand_var(char *name, int *vlen)
{
    char *c;
    char *rname = 0;
    int env = 0;

    try
    {
        if (*name == '$') // look env to
                     env = 1; 


         rname = new char[ strlen(name) +1 ];

         char bracket = (*(name+env) == '(') ? ')' : 
                                (*(name+env) == '{') ? '}' : 0;

         char *s = (bracket) ? name+env+1 : name+env;
         char *d = rname;

         while (*s && *s != bracket &&
                 !(bracket ==0 && strchr(WORD_BRAKES,*s)) 
               )
         {
           *d = *s; ++d; ++s; 
         }

         if (bracket && !*s)
            throw keyException("Unclosed bracket '%c'", bracket); 

         *d = 0;
         if (vlen) *vlen = (d-rname)+ ((bracket) ? 2:0)+1+env; //$ and two brackets 
             
         c = seek(rname);  
         if (!c && env)
             c = getenv(rname);
 
         if (!c )
            throw keyException("Undefined variable '%s'", rname); 

         delete[] rname;

       return c;
    }
    catch( ... )
    {
       if (rname)
          delete rname;
       throw;
    } 
}

/* .if $var == strvalue
 * or 
 * .if $var != strvalue
 *
 */ 
int dsApprc::expand_condition(char *cond)
{
    int rc;
    int condition = -1;
    char *p;

    if ((p = strchr(cond,'=')) == 0 )
        throw keyException("invalid condition format");

    if (*(p-1) == '!' )
    {
        condition = 0; 
        --p;
    }
    else if ( *(p+1) == '=')
    {
        condition = 1; 
    }
    else
        throw keyException("invalid condition format");

    char *lh = dsStrndup( cond, p -cond );
    char *rh = dsStrdup( p+3 );      

// 
    char *e;
    if ((e = strchr(rh,'&')) != 0 )
    {
        if (*(e+1) == '&')
        {
            *e = 0;
            condition &= expand_condition(e+2);
        }
    }

    if ((e = strchr(rh,'|')) != 0 )
    {
        if (*(e+1) == '|')
        {
            *e = 0;
            condition |= expand_condition(e+2);
        }
    }


    trim(lh);
    trim(rh);

    // printf("xxx: '%s' '%s'\n", lh, rh);

    char *c;

    if (lh[0] == '$' && (c = expand_var(lh+1))  )
    {
        rc = strcmp(c,rh); 
    }
    else
    {
        rc = strcmp(lh,rh);
    }

    delete rh;
    delete lh;


    return(condition) ? (rc == 0) : (rc != 0); 
}

void dsApprc::load(const char *fname)
{
     fstream fs(fname,ios::in);
     if (!fs.good() )
       throw keyException("Can't open file '%s' (%m)", fname); 
     
     add(".file-[]", fname);
     load(fs);
}

void dsApprc::load(istream& is)
{
  char read_buf[MAX_RC_LINE], *buf;
  int line   = 0;
  int ignore[30], lev = 0, do_ignore = 0, i; 
  ignore[0] = 0;
  
  char *p;

  try{

  while ( ! is.getline(read_buf,MAX_RC_LINE-1).eof() )
  {
    line ++;
    buf = read_buf;

//  if ( buf[strlen(buf)-1] == '\n' )  buf[strlen(buf)-1] = 0; 

    while(*buf && (*buf == ' ' || *buf =='\t')) ++buf;
    

    if ( *buf =='\0' || *buf == '#' || *buf == ';') continue;

/* Includes */
    if ( strncmp(buf,".include", 8) == 0 )
    {
        if ( _recursion_cnt++ > MAX_INCLUDE_LEVEL)
           throw keyException("Too many nested includes."); 
         
        dsStrstream tmp;
        char *s = buf+8, *res;
        while (*s && *s != '\"') ++s;

        while (*s && *(++s) != '\"') 
        {
          if (*s == '$')
          { int n = 0;
            if ( !(res = expand_var(s+1, &n) ) )
                  throw keyException("Variable can't be expanded.'%s'", s); 
            tmp << res;
            s+=(n-1);
          }
          else  
           {  tmp << *s;
           }
        }

        if (!*s || !tmp.pcount() )
           throw keyException("Invalid include syntax. Use .include \"filename\"" ); 

       load( tmp.str() );
       continue;
        
    }   

/* conditional expressions */
    if ( strncmp(buf,".if", 3) == 0 )
       { ignore[++lev] = ! expand_condition( buf+3 );
         continue;
       }
    
    if ( strncmp(buf,".endif",6) == 0) 
       { if ( lev ==  0 )
             throw keyException("unmaptched .endif"); 
         lev --;
         continue;
       }

    if ( strncmp(buf,".else", 5) == 0) 
       { ignore[lev] = !ignore[lev];
         continue;
       }

    if ( strncmp(buf,".end",4) == 0 )
                                   break;
 
    for (i = 0, do_ignore = 0; i <= lev; ++i) do_ignore |= ignore[i];
    if (do_ignore)
          continue; 
  
     _add(buf); 

  } // while

  if ( lev !=  0 )
     throw keyException("Unexpected end of file (unclosed .if)"); 
 }
 catch( keyException &e )
 {
   throw keyException("Error: %d: %s", line, e.msg() ); 
 }
}

/*
 *  Temporal solution for
 *  user-space add routine
 */

void dsApprc::add(const char *key, const char *val)
{
    dsStrstream out;
    out << key << '=' << val << ends;
    _add(out.str());
}

void dsApprc::_add(char *line)
{
  /* Handle literals: like
   * {this is complex = part one}-{complex part two}={complex value}   
   */

   int len = strlen(line), lit = 0;

/* XXX:
    change encoding if necessary 
    code removed, iconv support 
    should be here
*/

   char *s = line, *eol = line+len-1;

   dsStrstream stKey, stVal;
   dsStrstream *stCurr = &stKey;

   // remove trailing spaces
   while(eol && (*eol == ' ' || *eol == '\t' || *eol == '\n') ) --eol;

   while(s <= eol)
   {
     switch (*s)
     {
       case  '\\': // handle escapes
       {
           switch( *(s+1) )
           {   
             case 'n': (*stCurr) << '\n'; break;
             case 'r': (*stCurr) << '\r'; break;
             case 't': (*stCurr) << '\t'; break;
             case '{': (*stCurr) << '{' ; break;
             case '}': (*stCurr) << '}' ; break;
             case '\\': (*stCurr) << '\\' ; break;
             deafult:
               if (!lit)
                 (*stCurr) << *(s+1); else (*stCurr) << *s << *(s+1);
           } // switch
           s+=2; 
           continue;
       }

       case '{' :
       {   ++lit; 
           ++s;
          continue;
       } 
        
       case '}' :
       { 
         --lit; 
         if (lit < 0)
             throw keyException("Unbalanced '}' in literal.'%s'", line); 
          ++s;
          continue;
        }

        case '\t': // eat-up any unescaped space in keys;
        case ' ': // eat-up any unescaped space in keys;
        {
           if(!lit && stCurr == &stKey) 
           {
             ++s;
             continue;
           }

         break;
        }
 
        case '$' :  //expand variables 
        {
           int n = 0;
           char *res = expand_var(s+1, &n);
           if (!res)
                throw keyException("Variable can't be expanded.'%s'", s); 
           (*stCurr) << res;
           s+=n;
           continue;
        }

        case '=' :
        {
           if (!lit && stCurr != &stVal)
           { stCurr = &stVal;
             ++s;
             // eat-up leading space in value
             while(*s == ' ' || *s == '\t') ++s;
             continue;
           }
         break;
        }
     }  // switch

      (*stCurr) <<  *s;
      ++s; 
    } 

    if ( stCurr == &stKey )
       throw keyException("Invalid line format. No unescaped '=' tag"); 

    if (lit)
       throw keyException("Unbalanced '{' in literal.'%s'", line); 

    _add2( stKey.str(), stVal.str() );
}

void dsApprc::_add2(const char *key, const char *val)
{
  int i, klen = 0;
  char *nkey = dsStrdup(key), *nval = dsStrdup(val);

/* Check whether key is array 
   Array definition format - last section of key should be
   [] or  you can handle array manually by adding -01, -02 etc 
   and -count:
   e.g:
      button-[]=
      button-count=
*/

   klen = strlen(nkey);

   if ( *(nkey+klen-3) == _key_delim && 
        *(nkey+klen-2) == '[' && *(nkey+klen-1) == ']' )
   {
        /* build  counter name */
         char *counter_name = new char[ klen + 10 ];
         strcpy(counter_name,nkey);             
         strcpy(counter_name+klen-3,"-count");

         int count = getlong( counter_name, 0 );
         
         char *array_key = new char[ klen+10 ];
         strcpy(array_key,nkey);             
         sprintf(array_key+klen-3, "-%02d",count);             

         /* store key */ 
         dsHashTableItem *res;
          
         // insert or REPLACE key
         if ( (res =_h->insert( array_key, (void *)nval, true)) ) 
             delete res;

           char count_tmp[32];
           sprintf(count_tmp,"%d",count+1);
          
           if ( (res =_h->insert(counter_name,count_tmp, true)) ) 
              delete res;
       
         delete[] array_key;
         delete[] counter_name;
  }
  else // usual key
  {
     _h->insert( nkey, (void *)nval, true);  // insert or REPLACE key
  }

  delete[] nkey;
}

char *dsApprc::seek(const char *key)
{
   char *k = 0, *val = 0; 

   if ( key[0] == _key_delim && _keyst)
   {
    k = new char[ strlen(_keyst) + strlen(key) +2 ];
    strcpy(k, _keyst );
    strcat(k, key);

    val = _h->seek(k); 

    if (k) {
      delete[] k;  
    }
    
   }
   else
   { val= _h->seek(key);  
   }

   return val;
} 

char *dsApprc::seek(const char *key, int idx)
{
   char *nk = new char[strlen(key)+10];
    sprintf(nk,"%s-%02d", key, idx);
    char *rv = seek(nk);
   delete[] nk;
   return rv; 
}
 
void dsApprc::push(int array_key)
{
    char tmp[10];
    sprintf(tmp, "%02d",array_key);
    push(tmp);
}

void dsApprc::push(const char *key)
{
    int klen = strlen(key);

    if (!_keyst) // stack is empty
       { _keyst = strdup(key);
         _kslen = klen;
       }
    else
      {
        int kslen = strlen(_keyst);
        
        if ( kslen+ klen + 1 >= _kslen ) 
           {
              char *tmp = new char[ kslen+ klen + 2];
              strcpy(tmp,_keyst);
              delete _keyst;
              _keyst = tmp;
              _kslen = kslen+ klen + 2;
           }
        
           _keyst[kslen] = _key_delim;
           strcpy(_keyst+kslen +1, key);
       }
}

void dsApprc::pop()
{
  int rc;
  char *p;

  if ( !_keyst || strlen(_keyst) == 0 )
     throw keyException("Key stack underflow");
  
  if ((p = strrchr(_keyst,'-')) == 0 )
     { delete _keyst;
       _keyst = 0; _kslen = 0;
     }
     else
       { *p = 0;
       }
}

void dsApprc::dump(ostream& os)
{
    dsHashTableItem *tmp;

    for (int i =0; i < _h->HSize(); ++i)
    {
        if ( (tmp = _h->walk(i)) )
        {
            //os << "\'" << tmp->key << "'[Hash: " << i << "]='" <<  (char *) tmp->val << "\'" << endl; 
            if (strchr(tmp->key,' '))
                os << '{' << tmp->key << '}' << "=";
            else
                os << tmp->key << "=";

            if (strchr((char *)tmp->body,' '))
                os << '{' <<  (char *) tmp->body << '}' << endl;
            else
                os << (char *) tmp->body  << endl; 
        }
    }
}                                                


