/*
 * $Id: dsGetopt.cxx,v 1.1.1.1 2004/09/10 08:12:19 dsamersoff Exp $
 *
 * This is adobtion of standard BSD getopt
 * see:
 *  http://www.openmash.org/lxr/source/compat/getopt.c
 *
 */

#include <dsGetopt.h>

using namespace std;
using namespace libdms5;

#define EMSG    ""


dsGetopt::dsGetopt(int argc, char **argv)
{
   _argv  = argv;
   _argc  = argc;

   _optind   = 1;             /* index into parent argv vector */
   _optopt   = 0;               /* character checked for validity */
   _optreset = 0;               /* reset getopt */

   _optarg = 0;
   _optind = 1;

   _place  = EMSG;
}

dsGetopt::~dsGetopt(){
// do nothing
}

int dsGetopt::next(const char *ostr)
{
          const char *oli = ostr;                        /* option letter list index */

          if (_optreset || !*_place)               /* update scanning pointer */
          {
                  _optreset = 0;
                  if (_optind >= _argc || *(_place = _argv[_optind]) != '-')
                  {
                          _place = EMSG;
                          return (EOF);
                  }

                  if (_place[1] && *++_place == '-')       /* found "--" */
                  {
                          ++_optind;
                          _place = EMSG;
                          return (EOF);
                  }
          }
                                       /* option letter okay? */
          if ( (_optopt = (int)* _place++) == (int)':' ||
              !(oli = strchr(ostr, _optopt))  )
         {
                  /*
                   * if the user didn't specify '-' as an option,
                   * assume it means EOF.
                   */
                  if (_optopt == (int)'-')
                                   return (EOF);

                  if (!*_place)
                          ++_optind;

                  if (*ostr != ':')
                      throw dsGetoptException("Illegal option '%c'", _optopt);
          }

          if (oli != NULL && *++oli != ':')                  /* don't need argument */
          {
                 _optarg = NULL;
                 if (!*_place)
                         ++_optind;
          }
          else                                   /* need an argument */
          {
                 if (*_place)                     /* no white space */
                 {
                         _optarg = _place;
                 }
                 else if (_argc <= ++_optind)    /* no arg */
                 {
                         _place = EMSG;
                        throw dsGetoptException("Not enough argument given for '%c'", _optopt);
                 }
                 else                            /* white space */
                 {
                         _optarg = _argv[_optind];
                 }

                 _place = EMSG;
                 ++_optind;
         }
         return (_optopt);                        /* dump back option letter */
 }


/* ==================================== */

dsGetoptMore::dsGetoptMore(dsApprc *ap, int argc, char *argv[])
{
  _ap   = ap;
  _argc = argc;
  _argv = (char **)argv;

 _ht = new dsHashTable(10);
}

dsGetoptMore::~dsGetoptMore()
{

   for (int i = 0; i < _ht->HSize(); ++i )
   {
         dsHashTableItem *dp = _ht->walk(i);
         if (dp)
             delete dp;
   }

  if (_ht)
    delete _ht;
}


void dsGetoptMore::usage()
{
   cerr << "Valid options for this program: " << endl;
   for (int i = 0; i < _ht->HSize(); ++i )
     {
         dsHashTableItem *dp =  _ht->walk(i);
         if (dp)
         {
             cerr << dp->key << " - " << (const char *) dp->body << endl;
         }
     }

  throw dsGetoptException("Help was requested");
}

void dsGetoptMore::setArg(char *xarg, char *xdescr)
{
     if (xarg[0] == '-' && xarg[1] == '-')
      _ht->insert(xarg+2, xdescr ?  dsStrdup(xdescr) : dsStrdup("undescribed")  );
     else
      _ht->insert(xarg, xdescr ?  dsStrdup(xdescr) : dsStrdup("undescribed")  );
}

void dsGetoptMore::execute()
{
    for(_optind = 1; _optind < _argc; ++_optind)
    {
        char *arg = _argv[ _optind ], *s;

        if ( arg[0] != '-' || arg[1] != '-') // posix mode stop processing options on first non-option argument
                                          break;

        if ( strcmp(arg,"--help") == 0 )
                                     usage();

        if ( (s = strchr( arg, '=')) != 0 )
        {
          char *tmp = dsStrndup(arg+2, s-arg-2);
           s =_ht->seek( tmp );
          delete tmp;

          if (! s )
             throw dsGetoptException("Unknown option: '%s' (try -h for help)", arg);


          _ap->_add(arg+2);
        }
        else
         {
            if ( !_ht->seek(arg) )
               throw dsGetoptException("Unknown option: '%s' (try -h for help)", arg);

             _ap->add(arg+2, "yes");
         }
    }

   return;
}
