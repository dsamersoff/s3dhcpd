/*
 * $Id: dsHashTable.cxx,v 1.1.1.1 2004/09/10 08:12:19 dsamersoff Exp $
 * 
 *  based on code written by Kent Schliiter
 */ 

#include <dsHashTable.h> 

using namespace libdms5;

dsHashTableGeneric::dsHashTableGeneric (int numEntries, bool purge)
{ 
    _HSize = (numEntries > 20) ? numEntries : 20; 
    _purge = purge;

    if (! (_Table =  new tableType[_HSize]) )
       throw dsHashTableException("%d:%s Not enough memory to allocate %d entries",_HSize, __LINE__,__FILE__);

    memset(_Table, 0, sizeof(_Table[0]) * _HSize);
} 

dsHashTableGeneric :: ~dsHashTableGeneric () 
{
     for (int i = 0; i < _HSize; ++i)
     { 
       if (_Table[i])
       {
         if (_purge) 
             delete (char *)(_Table[i]);
       }
     }

    delete[] _Table; 
} 

void dsHashTableGeneric::clear() 
{
     for (int i = 0; i < _HSize; ++i)
     { 
       if (_Table[i])
       {
         if (_purge) 
             delete (char *)(_Table[i]);
       }
     }

    memset(_Table, 0, sizeof(_Table[0]) * _HSize);
}
/*
 *  Reallocate table if it become full
 */
 
int dsHashTableGeneric::GrowUp(void)
{
    int i;
    tableType* old_Table = _Table;

    int old_HSize = _HSize;

    if (_HSize >= HASH_MAXSIZE)
        throw dsHashTableException("%d:%s Maximal allowed size (%d) reached",HASH_MAXSIZE, __LINE__,__FILE__);

    _HSize = ((_HSize + HASH_INCREMENT) < HASH_MAXSIZE) ? (_HSize + HASH_INCREMENT) : HASH_MAXSIZE;

    if (! (_Table =  new tableType[_HSize]) )
        throw dsHashTableException("%d:%s Not enough memory to allocate %d bytes",_HSize, __LINE__,__FILE__);

    memset(_Table, 0, _HSize * sizeof(_Table[0]));  

    /* copy table */
    for (i = 0; i < old_HSize; ++i)
    {
        if ( old_Table[i] )
        {
            int HIndex;
            /* Use seek to calculate insertion position in new table */
            if ( ! _hi_seek(old_Table[i], &HIndex) )
            {
                _Table[HIndex] = old_Table[i];
            }
        }
    }

    delete[] old_Table;
    return 0;
}

/* 
 *   Attempts to insert a key into the hash table
 */

void *dsHashTableGeneric::insert(void *a, bool replace) 
  {                   
    int HIndex;

    void *tmp = _hi_seek(a, &HIndex); 

    if (HIndex == -1)
    {
          GrowUp();  // Increase table size
          tmp = _hi_seek(a, &HIndex);
          if (HIndex == -1) 
             throw dsHashTableException("Table full and can't grow more");
    } 
    
    void *p = tmp;
      
    /* item not found  or replace set to true*/
    if (!tmp || replace)
    {  
        _Table[HIndex]= a;
    }

    return p;
} 

/* 
 *   delete key
 */
  
void * dsHashTableGeneric::remove(void *a) 
{ 
    int HIndex;

    void *tmp = _hi_seek(a, &HIndex); 
    if (tmp) 
    {
       void *p = tmp;
      _Table[HIndex] = 0;
       
       return p;
    } 

     return  0;
} 

/* 
 *  Determines if the string is allready in the hash table 
 *
 * \note
 *   This function sets the data member 'findPos' to either the index 
 *   of the hash table where the key was found, OR if it isn't in the table 
 *   'findPos' is set the the index of where it could be inserted.  'findPos' 
 *   is set to -1 if unable to insert for other reason (ie, table was full). 
 */

void *dsHashTableGeneric::_hi_seek(void *a, int *HIndex) 
{ 
    int hashV; 

    hashV = primary_hash_func(a); 

    int start_pos = hashV; 
    int hmul = 1; 

    do
    { // See if key is in table 

        if (! _Table[hashV] )
        {
            *HIndex = hashV; 
            return 0;  
        }


        if ( cf(a, _Table[hashV]) == 0 )
        {
            *HIndex = hashV; 
            return _Table[hashV]; 
        }

        hashV = ((start_pos + ( hmul * secondary_hash_func(a))) % _HSize);

        hmul++; 

    } while ( start_pos != hashV ); // If next position isn't where we started from 


    // If we are here, then something has gone wrong.  Table Full. 
    *HIndex = -1;

    return 0;
} 

void *dsHashTableGeneric::seek(void *a)
{
  int unused; 
  return  _hi_seek(a, &unused);
}


/* ======== dsHashTable ========================== */

int dsHashTable::HFuncAddition( const char *key ) 
{ 
    int hashvalue=0; 
    while (*key) 
     { hashvalue = (*key + hashvalue); 
       key++; 
     } 
    
    return (hashvalue % HSize() ); 
} 

int dsHashTable::HFuncHorner( const char *key )
  {
    int hashvalue=0;
    while (*key) 
    { 
      hashvalue = (*key + (131 * hashvalue));
      key++;
    }

    hashvalue = (hashvalue % HSize() );
    return (hashvalue > 0) ? hashvalue : 0 - hashvalue; 
}
