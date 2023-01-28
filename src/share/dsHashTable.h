/*
 * $Id: dsHashTable.h,v 1.1.1.1 2004/09/10 08:12:19 dsamersoff Exp $
 */

#ifndef dsHashTable_h
#define dsHashTable_h

#include <dsSmartException.h>
#include <dsSlightFunctions.h>

#define HASH_INCREMENT 1024
#define HASH_MAXSIZE 32576


namespace libdms5
{

DECLARE_EXCEPTION(dsHashTable);


/** \class dsHashTable
 *  \brief Just a hash table, not superfast but relable and convenient
 *  
 *  This class provide method to store dat in to hashed array and access them by
 *  key. You can choise from one of two different hash function and compute
 *  hash performance statistic if necessary. For large namber of keys bintree 
 *  provide better performance and memory usage
 *  
 *  \see dsBintree
 *
 *  \todo Convert defines into enum
 */

    class dsHashTableGeneric 
    {
        typedef void * tableType;

        tableType* _Table;  // Pointer to table 
        int    _HSize;  // The size of the hash table 
        bool   _purge;

        int GrowUp(void);

        void* _hi_seek(void *a, int *HIndex );

    protected:
        virtual int cf(void * a, void *b)          = 0;  
        virtual int primary_hash_func( void *a)    = 0;
        virtual int secondary_hash_func( void *a)  = 0; 

    public:

        /**
         * Construct an object
         * 
         * @param numEntries - initial table size
         * @param purge      - delete table entry
         */

        dsHashTableGeneric(int numEntries, bool purge = true);
        virtual ~dsHashTableGeneric();

/* Implementation of dsDataAccessor interfaces 
 */
        virtual void *insert(void *a, bool replace = false );
        virtual void *remove(void *a);
        virtual void *seek(void *a); 

/**
 * Reinit table, keep current size
 */
        void clear();
/**
 * Function returns table i'th table record even it is empty 
 */
        void *walk(int i);

/**
 * Function returns curren table size
 */
        int HSize(void); 

/**
 * Function returns number of non-empty records in the table
 */
        int NumEntries(void);
    }; 

    inline int dsHashTableGeneric::HSize(void) 
    {
        return _HSize; 
    }

    inline void *dsHashTableGeneric::walk(int i)
    {
        if (i > _HSize)
            throw dsHashTableException("Index out of bounds");

        return _Table[i];
    }

    inline int dsHashTableGeneric::NumEntries(void) 
    { 
        int total=0; 

        for (int i=0; i < _HSize; i++)
        {
            if (_Table[i]) total++;
        }

        return total; 
    } 


/** \class dsHashTableItem
 *  \brief Item holding structure for dsHashTable 
 */ 
    struct dsHashTableItem 
    {
        bool _purge_body;

        const char *key;
        void *body;
    
        dsHashTableItem()
        {
            key = 0; body = 0;
            _purge_body = false; 
        }

        dsHashTableItem(const char *xkey)
        { 
            key = strdup(xkey);
            body = 0;
            _purge_body = false; 
        }

        ~dsHashTableItem()
        { 
            if (key) 
                delete (char *)key; 
            
            if (body && _purge_body) 
                delete (char *) body;
        }
    }; 


    class dsHashTable : public dsHashTableGeneric
    {

        int HFuncAddition( const char *key ); // Hash function using Addition method 
        int HFuncHorner( const char *key );   // Hash function using horner's method 
        const char *key(void *a)                  { return((dsHashTableItem *)a)->key;}

    protected:
        virtual int cf(void * a, void *b)         { return strcmp(key(a), key(b));}
        virtual int primary_hash_func( void *a)   { return HFuncAddition( key(a) );}
        virtual int secondary_hash_func( void *a) { return HFuncHorner( key(a) );}

    public:
        dsHashTable(int numEntries, bool purge = true):
          dsHashTableGeneric(numEntries,purge) {}

        dsHashTable():
          dsHashTableGeneric(100, true) {}

        dsHashTableItem *seek(dsHashTableItem *dp);
        dsHashTableItem *insert(dsHashTableItem *dp, bool replace = false);

        /* convenience interfaces */
        char *      seek(const char *key);

        /* Warning! This functions store pointer without making copy of it,
        caller responsible to keep pointer valid
        */
        dsHashTableItem *insert(const char *key, void *value,        bool replace = false);

        /**
         * Functions below creates a copy of passed value
         */
        dsHashTableItem *insert(const char *key, const char * value, bool replace = false);
        dsHashTableItem *insert(const char *key, long value,         bool replace = false);
        
        dsHashTableItem *walk(int i);
    };  

/* convenience interfaces */

    inline  dsHashTableItem * dsHashTable::seek(dsHashTableItem *dp)
    { 
        return(dsHashTableItem *) dsHashTableGeneric::seek((void *)dp); 
    }

    inline dsHashTableItem *dsHashTable::insert(dsHashTableItem *dp, bool replace)
    { 
        return(dsHashTableItem *) dsHashTableGeneric::insert((void *)dp, replace);
    }

    inline  char * dsHashTable::seek(const char *xkey)
    {
        dsHashTableItem dp(xkey); 
        dsHashTableItem *res = seek(&dp); 
        return (res) ? (char *) res->body : 0;
    }


    inline dsHashTableItem *dsHashTable::insert(const char *key, void *value, bool replace)
    {
        dsHashTableItem *hi = new dsHashTableItem(key); 
        hi->body = value;

        dsHashTableItem *tmp = insert(hi, replace);
        if (tmp)
        {
            if (!replace)
                delete hi;
        }

        return tmp;
    }


    inline dsHashTableItem *dsHashTable::insert(const char *key, const char * value, bool replace)
    {
        dsHashTableItem *hi = new dsHashTableItem(key); 
        hi->body = dsMemdup( (const void *)value, strlen(value)+1 );
        
        hi->_purge_body = true;
         
        return (dsHashTableItem *) insert(hi, replace);
    }

    inline dsHashTableItem *dsHashTable::insert(const char *key, long value, bool replace)
    {
        dsHashTableItem *hi = new dsHashTableItem(key); 
        hi->body = (void *)value;
        return (dsHashTableItem *) insert(hi,replace);
    }
    
    inline dsHashTableItem *dsHashTable::walk(int i)
    {
        return (dsHashTableItem *) dsHashTableGeneric::walk(i);
    }

};

#endif 

