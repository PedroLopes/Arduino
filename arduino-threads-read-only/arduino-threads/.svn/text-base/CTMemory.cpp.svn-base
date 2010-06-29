
/* ctmemory -- a collection of memory-management routines; a wrapper
 for malloc() and free().

 Copyright (C) 2001  Scott McKellar  mck9@swbell.net

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 One purpose of this module is to issue a uniform message when 
 memory allocation fails.

 Another is to provide a more robust interface, with more stringent
 validations: no allocations of zero bytes, no freeing of a NULL
 pointer.

 Another is to provide a mechanism for freeing memory which is
 allocated but currently unused.  So if malloc() fails, we can
 free whatever memory we can spare and try again.  In theory we
 can avoid some out-of-memory failures.

 But the main purpose is to provide a home for layers of debugging
 code, so that we can detect and track down memory management bugs
 more readily.

 This approach is based on the techniques detailed by Steve Maguire
 in Writing Solid Code (Microsoft Press, 1993, Redmond, WA).

 */

#include "CTMemory.h"

CTMemory::CTMemory() {

    slotCount = 0;
    excessPools = 0;

#ifndef NDEBUG

    allocationCount = 0;
    outstandingCount = 0;
    maxCount = 0;

#endif

}

CTMemory::~CTMemory() {
}

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

void * CTMemory::allocMemory(size_t size) {
    void * p;

#ifndef NDEBUG

    static int firstTime = TRUE;

    /* Note that functions installed by atexit() are installed
     in one order but called in the reverse order.  We install
     reportMemory() first so that it will be called last.
     */

    if (TRUE == firstTime) {
        
       /*  WHERE ARE THIS FUNCTIONS?
        * 
        atexit(reportMemory);
        atexit(freePoolMemory);
        firstTime = FALSE;
        
        */
    }
    allocationCount++;

#endif

    ASSERT(size != 0);

    p = malloc(size);

    /* In case of failure we release all memory pools and try again. */

    if (NULL == p && slotCount > 0) {
        freePoolMemory();
        p = malloc(size);
    }

    if (NULL == p) {
        CTOut::ct_report_error("allocMemory: unable to allocate memory");
        return NULL;
    }
    else {

#ifndef NDEBUG

        memset(p, NEWGARBAGE, size);
        outstandingCount++;
        if (outstandingCount > maxCount)
            maxCount = outstandingCount;

#endif

        return p;
    }
}

/********************************************************************
 freeMemory -- a wrapper for free().  So far nothing special.  We
 don't try to fill the memory with garbage because, in the absence
 of additional machinery, we don't know how big the block is.

 For the debug version we decrement the allocation count.
 *********************************************************************/

void CTMemory::CTMemory::freeMemory(void * pMem) {
    ASSERT(NULL != pMem);

    free(pMem);
#ifndef NDEBUG

    outstandingCount--;

#endif
}

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
void CTMemory::registerMemoryPool(void (* pFunction) (void *), void * p) {
    MemoryPool * pMP;
    MemoryPool * pOpenSlot = NULL;
    int i;

    ASSERT(pFunction != NULL);

    /* Scan all the pools.  Maybe this pool is already registered.  If
     so, the debug version aborts, but the production version just 
     returns without complaint.

     If the pool is not already registered, look for an open slot for it.
     */

    for (pMP = memoryPoolList, i = slotCount; i > 0; pMP++, i--) {
        if (NULL == pMP->freeFunc) {
            ASSERT(NULL == pMP->genericPtr);
            pOpenSlot = pMP;
            break;
        }
        else {
            ASSERT(pMP->freeFunc != pFunction || pMP->genericPtr != p);
            if (pMP->freeFunc == pFunction && pMP->genericPtr == p)
                return;
        }
    }

    /* if we haven't found an open slot yet, bump the counter
     (and check for overflow)
     */

    if (NULL == pOpenSlot) {
        if (slotCount >= POOLMAX) {
            /* no available slot -- forget it */

            excessPools++;
            return;
        }
        else {
            pOpenSlot = memoryPoolList + slotCount;
            slotCount++;
        }
    }

    /* Having picked a slot, store the pointers in it */

    pOpenSlot->freeFunc = pFunction;
    pOpenSlot->genericPtr = p;

    return;
}

/*******************************************************************
 UnRegisterMemoryPool -- removes a memory pool from the list.
 *******************************************************************/

void CTMemory::unRegisterMemoryPool(void (* pFunction) (void *), const void * p) {
    MemoryPool *pMP;
    int i;

    ASSERT(pFunction != NULL);

    /* scan the pools, looking for the one we're supposed to remove. */

    pMP = memoryPoolList;
    for (i = slotCount; i > 0; i--) {
        if (pMP->freeFunc == pFunction && pMP->genericPtr == p) {
            pMP->freeFunc = NULL;
            pMP->genericPtr = NULL;
            return;
        }
        else
            pMP++;
    }

    /* if we haven't found the specified pool, we must have run out
     of slots at some point -- or else we're being asked to
     unregister a pool that was never registered in the first place.
     */

    ASSERT(slotCount == POOLMAX);

    return;
}

/*******************************************************************
 freePoolMemory -- calls all registered routines for freeing memory
 *******************************************************************/

void CTMemory::freePoolMemory(void) {
    MemoryPool *pMP;
    int i;

    pMP = memoryPoolList;
    for (i = slotCount; i > 0; i--) {
        if (NULL != pMP->freeFunc) {
            pMP->freeFunc(pMP->genericPtr);
        }
        pMP++;
    }

    return;
}

#ifndef NDEBUG

/********************************************************************
 reportMemory -- reports memory usage.
 ********************************************************************/

void CTMemory::reportMemory(void) {
    char buf[ 100 ];

    sprintf(buf, "Maximum memory pools registered: %u", slotCount);
    CTOut::ct_report_error(buf);

    if (excessPools != 0) {
        sprintf(buf, "Memory pools that could not be registered: %u",
                excessPools);
        CTOut::ct_report_error(buf);
    }

    sprintf(buf, "Total memory allocations: %lu", allocationCount);
    CTOut::ct_report_error(buf);

    sprintf(buf, "Maximum outstanding memory allocations: %lu", maxCount);
    CTOut::ct_report_error(buf);

    if (outstandingCount != 0) {
        sprintf(buf, "\nMEMORY LEAK!  %lu un-freed allocations remain",
                outstandingCount);
        CTOut::ct_report_error(buf);
    }
}

#endif

