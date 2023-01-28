/*
 * $Id: dsGetopt.h,v 1.1.1.1 2004/09/10 08:12:19 dsamersoff Exp $
 *
 */

#ifndef dsGetopt_h
#define dsGetopt_h

#include <iostream>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <dsSmartException.h>
#include <dsApprc.h>

namespace libdms5
{

    DECLARE_EXCEPTION(dsGetopt);

/**
 * This class is designed to provide
 * portable, C++ version of getopt
 * Now it operates only in posix standard
 * "REQUIRE_ORDER" mode, i. e. first non-option argument
 * stops option processing. 
 * Argv[0] ignored.
 */

    class dsGetopt {

        char **_argv;
        int    _argc;

        const char *_optarg;
        int   _optind;     // index into parent argv vector 
        int   _optopt;     // character checked for validity
        int  _optreset;    // reset getopt 
        const char *_place;

    public:
/**
 * Construct an object, set argc and argv for future processing
 *
 * @param argc argument count
 * @param argv pointer to argument array
 * @throws dsGetoptException if no arguments given
 */
        dsGetopt(int argc, char **argv);
        virtual ~dsGetopt();

/**
 * Return next option
 *
 * @param optstr getopt option string
 * @throws dsGetoptException 
 */
        virtual int next(const char *optstr);

/**
 * 
 * @return argument of last option
 */ 

        const char *optarg(){ return _optarg;}

/**
 * @return iposition of last option
 */

        int   optind(){ return _optind;}

    };

    class dsGetoptMore {

        dsHashTable *_ht;
        dsApprc     *_ap;

        int _argc;
        char **_argv;

        int   _optind;

    protected:   
        virtual   void usage();

    public:

        dsGetoptMore(dsApprc *dest, int argc, char *argv[]);
        virtual  ~dsGetoptMore();


        void setArg(char *xarg, char *xdescr = 0);
        void execute();

        int   optind();

    };

    inline int dsGetoptMore::optind() {
        return _optind; 
    }

};

#endif
