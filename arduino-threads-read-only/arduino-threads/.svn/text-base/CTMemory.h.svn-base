
#ifndef CTMEMORY_H_
#define CTMEMORY_H_

#include <stdlib.h>
#ifndef NDEBUG
#include <stdio.h>
#endif
#include <string.h>
#include "ct.h"
#include "ctutil.h"

#include "CTOut.h"
#include "CTAssert.h"

/******************************************************************

 For the debugging version:

 The following byte values were chosen because they are not
 printable and cannot be part of packed decimal data (i.e. COBOL
 COMP-3).  And they are neither binary zeros nor binary ones.  So
 they are unlikely to occur legitimately in long runs of garbage
 data.  If we see them in the wake of a bug, we can recognize them.

 We fill newly allocated memory with NEWGARBAGE.
  
******************************************************************/

#define NEWGARBAGE   ('\xFB')
#define POOLMAX      (32)/* number of pools we can register */

typedef struct {
        void ( * freeFunc)(void *); /* ptr to memory-freeing function */
        void * genericPtr; /* ptr to be passed to the above */
} MemoryPool;

/* Note: all pointers in the following array are implicitly
 initialized to NULL.
 */

class CTMemory {
    
    public:
        
        CTMemory();
        virtual ~CTMemory();

        /*******************************************************************
         allocMemory -- a wrapper for malloc().

         For the debugging version, we fill newly-allocated memory with an
         arbitrary value which is unlikely to occur legitimately throughout
         a memory block.  If buggy code tries to use the contents of that
         memory before initializing it, the resulting bugs will be more
         predictable and more recognizable than they would be if we just
         left random garbage in there.

         Also we record a running count of the memory allocations, and the
         maximum allocations.

         In the debugging version: at the end of the job we will free all
         memory pools, then run a memory usage report so that we can
         detect memory leaks.

         *******************************************************************/

        void * allocMemory(size_t size);

        /********************************************************************
         freeMemory -- a wrapper for free().  So far nothing special.  We
         don't try to fill the memory with garbage because, in the absence
         of additional machinery, we don't know how big the block is.

         For the debug version we decrement the allocation count.
         *********************************************************************/

        void freeMemory(void * pMem);

        /*******************************************************************
         freePoolMemory -- calls all registered routines for freeing memory
         *******************************************************************/

        void freePoolMemory(void);

        
    private:

        /* Note: all pointers in the following array are implicitly
         initialized to NULL.
         */
        
        MemoryPool memoryPoolList [ POOLMAX ];

        unsigned slotCount;
        unsigned excessPools;

#ifndef NDEBUG

        unsigned long allocationCount;
        unsigned long outstandingCount;
        unsigned long maxCount;

#endif

/*******************************************************************
 registerMemoryPool -- stores for later use: a ptr to a memory-
 freeing function and a void ptr to be
 passed to it.  If we don't have room to store
 them, we discard them.

 So what is the purpose of the void pointer?

 Suppose you have multiple instances of struct FOOBAR, and each
 FOOBAR has a memory pool associated with it.  When you construct
 each FOOBAR, you can register the memory-freeing function, together
 with a pointer to that particular FOOBAR.  When you destruct that
 FOOBAR, you should call that function and pass it the appropriate
 pointer so that it will free the memory associated just with that
 particular FOOBAR.  Then un-register the function for that FOOBAR.

 If we ever need to free memory from within allocMemory(), we will
 free memory for all the registered FOOBARs.

 Most typically, however, the void pointer will be NULL.

 The current implementation stores the pairs of function and void pointers 
 in a simple array of fixed size.  If that turns out to be too confining, 
 we can use some other approach, such as a linked list or tree.
 *******************************************************************/

void registerMemoryPool(void (* pFunction) (void *), void * p);

/*******************************************************************
 UnRegisterMemoryPool -- removes a memory pool from the list.
 *******************************************************************/

void unRegisterMemoryPool(void (* pFunction) (void *), const void * p);

#ifndef NDEBUG

/********************************************************************
 reportMemory -- reports memory usage.
 ********************************************************************/

void reportMemory(void);

#endif

};

#endif /*CTMEMORY_H_*/
