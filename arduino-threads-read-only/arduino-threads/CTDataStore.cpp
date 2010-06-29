
/***************************************************************
 Memory management routines for cheap threads and associated
 events

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

 **************************************************************/

#include "CTDataStore.h"

#define FREE_MSGNODE_MAX 15
#define FREE_EVENT_MAX    6
    
    CTScheduler ctScheduler;
    CTMemory ctMemory;

CTDataStore::CTDataStore() {

    free_ct_list = NULL;
    free_ct_count = 0;

    free_msgnode_list= NULL;
    free_msgnode_count = 0;

    free_event_list= NULL;
    free_event_count = 0;

}

CTDataStore::CTDataStore( CTScheduler& ctScheduler,
        CTMemory& ctMemory ) {

    /* HOW DO YOU CALL THE VOID CONSTRUCTOR  */
    
    CTDataStore();
    
    ctScheduler = ctScheduler;
    ctMemory = ctMemory;


}

CTDataStore::~CTDataStore() {
}

/*****************************************************************
 Return CT_TRUE if the handle points to a valid Ct_thread, and
 CT_FALSE otherwise.

 Note that this function is not safe if passed a wild pointer, or
 a pointer to a handle containing a wild pointer.  The safest
 implementation would be to maintain a list of valid threads, and
 search it as needed.  Even that approach won't avoid a hardware
 exception in some architectures.
 ****************************************************************/

int CTDataStore::ct_valid_handle(const Ct_handle * pH) {
    if ( NULL == pH) {
        return CT_FALSE;
    }
    else {
        Ct_thread * pThread = (Ct_thread) pH->p;

        if ( NULL == pThread)
            return CT_FALSE;
        else
            if (CT_STATUS_DEFUNCT == pThread->status)
                return CT_FALSE;
            else
                if (pThread->incarnation != pH->incarnation)
                    return CT_FALSE;
                else
                    return CT_TRUE;
    }
}

/*****************************************************************
 Return CT_TRUE if two handles refer to the same incarnation of
 the same thread.  Otherwise return CT_FALSE.
 ****************************************************************/

int CTDataStore::ct_same_thread(const Ct_handle * pH_1, const Ct_handle * pH_2) {
    if ( NULL == pH_1 || NULL == pH_2) {
        CTOut::ct_report_error("ct_same_thread: null pointer argument");
        ctScheduler.ct_fatal_error();
        return CT_FALSE;
    }
    else
        if (pH_1->p == pH_2->p && pH_1->incarnation == pH_2->incarnation)
            return CT_TRUE;
        else
            return CT_FALSE;
}

/*****************************************************************
 Allocate and initialize a Ct_thread.
 ****************************************************************/

Ct_thread * CTDataStore::ct_construct(int priority, void * pData,
        Ct_step_function step, Ct_destructor destruct) {
    Ct_thread * pThread;

    pThread = alloc_ct();
    if (pThread != NULL) {
        pThread->pNext = pThread->pPrev = NULL;
        pThread->status = CT_STATUS_ACTIVE;
        pThread->priority = priority;
        pThread->pData = pData;
        pThread->msg_q = NULL;
        pThread->subscriptions = NULL;
        pThread->step = step;
        pThread->destruct = destruct;
#ifndef NDEBUG
        pThread->magic = CT_MAGIC;
#endif
    }

    return pThread;
}

/*****************************************************************
 Destruct and deallocate a cheap thread.  Call the destructor
 callback function if there is one.  Also destruct any pending
 messages.
 ****************************************************************/

void CTDataStore::ct_destruct(Ct_thread ** ppThread) {
    Ct_thread * pThread;

    if ( NULL == ppThread || NULL == *ppThread)
        return;

    pThread = *ppThread;
    *ppThread = NULL;

    ASSERT( CT_MAGIC == pThread->magic );

    /* Destruct associated events, if any */

    if (pThread->msg_q != NULL)
        ct_destruct_msgnode_list( &pThread->msg_q);

    if (pThread->subscriptions != NULL)
        ct_destruct_sub_list( &pThread->subscriptions);

    /* Call the thread's destructor, if there is one */

    if (pThread->destruct != NULL)
        pThread->destruct(pThread->pData);

    free_ct( &pThread);
}

/*****************************************************************
 Allocate a Ct_thread; from the free list if possible, or from the
 heap if necessary.

 Either initialize or increment the incarnation member.  This
 sequence number distinguishes among different reuses of the
 same physical memory location occupied by the Ct_thread.  It
 offers some protection (though still not utterly foolproof)
 against the possibility that a Ct_handle will outlive the
 thread to which it refers, and wind up referring to an unrelated
 thread that reuses the same memory.
 ****************************************************************/

Ct_thread * CTDataStore::alloc_ct(void) {
    Ct_thread * pThread;

    if ( 0 == free_ct_count) {
        pThread = (Ct_thread *) ctMemory.allocMemory(sizeof(Ct_thread));
        if ( NULL == pThread) {
            CTOut::ct_report_error("alloc_ct: out of memory");
            ctScheduler.ct_fatal_error();
        }
        else
            pThread->incarnation = 0;
    }
    else {
        --free_ct_count;
        ASSERT( 123456L == free_ct_list->magic );
        pThread = free_ct_list;
        free_ct_list = pThread->pNext;
        ++pThread->incarnation;
    }

    return pThread;
}

/*******************************************************************
 Deallocate a Ct_thread.  We do so by putting it on a free list for
 possible reallocation.  We don't actually free any of them until
 we're ready to free all of them.  This way, any pointers to a
 thread remain valid even if the threads themselves are defunct.

 Results: (1) Reallocating a defunct thread is faster than going
 back to the heap for it. (2) We don't crash and burn from trying
 to dereference an outdated pointer.

 (Otherwise we would have to worry about thread handles that
 outlive the threads to which they point.)
 ******************************************************************/

void CTDataStore::free_ct(Ct_thread ** ppThread) {
    Ct_thread * pThread;

    if ( NULL == ppThread) {
        CTOut::ct_report_error("free_ct: null double pointer argument");
        ctScheduler.ct_fatal_error();
        return;
    }
    else
        if ( NULL == *ppThread) {
            CTOut::ct_report_error("free_ct: pointer to thread is null");
            ctScheduler.ct_fatal_error();
            return;
        }

    pThread = *ppThread;
    *ppThread = NULL;

    ++free_ct_count;
    pThread->pNext = free_ct_list;
    free_ct_list = pThread;

#ifndef NDEBUG
    pThread->magic = 123456L;
#endif
}

/*********************************************************************
 Free all threads.  This routine should be called only when all
 threads have been destructed and the machinery is shutting down.
 ********************************************************************/

void CTDataStore::ct_free_all_threads(void) {
    Ct_thread * pTemp;

    while (free_ct_list != NULL) {
        ASSERT( 123456L == free_ct_list->magic );

        pTemp = free_ct_list->pNext;
        ctMemory.freeMemory(free_ct_list);
        free_ct_list = pTemp;
    }

    free_ct_count = 0;
}

/* ---------------- msgnode functions: ----------------------------- */

/*******************************************************************
 Allocate a Ct_msgnode, from the free list if possible, from the
 heap if necessary.  We don't populate it here; we just allocate
 memory for it.
 ******************************************************************/

Ct_msgnode * CTDataStore::ct_alloc_msgnode(void) {
    Ct_msgnode * pM;

    if ( NULL == free_msgnode_list) {
        pM = ctMemory.allocMemory(sizeof(Ct_msgnode));
        if ( NULL == pM) {
            CTOut::ct_report_error("ct_alloc_msgnode: Out of memory");
            ctScheduler.ct_fatal_error();
        }
    }
    else {
        --free_msgnode_count;
        pM = free_msgnode_list;
        ASSERT( 234567L == pM->magic );
        free_msgnode_list = free_msgnode_list->pNext;
    }

    return pM;
}

/*******************************************************************
 Deallocate a list of message nodes by sticking them on the free
 list.  Deallocate associated memory, and detach from the
 associated event.
 ******************************************************************/

void CTDataStore::ct_destruct_msgnode_list(Ct_msgnode ** ppM) {
    Ct_msgnode * pTail;
    Ct_msgnode * pM;

    if ( NULL == ppM || NULL == *ppM)
        return;/* No list provided, or list is empty */

    pM = *ppM;
    *ppM = NULL;

    /* Find the tail of the list, meanwhile */
    /* freeing buffers as needed */

    pTail = pM;
    for (;;) {
        ++free_msgnode_count;

        ASSERT( MSGNODE_MAGIC == pTail->magic );
#ifndef NDEBUG
        pTail->magic = 234567L;
#endif
        if (pTail->pE != NULL) {
            /* Decrement the reference count of the associated */
            /* event.  If it becomes zero, destruct the event. */

            if ( --pTail->pE->refcount)
                pTail->pE = NULL;
            else
                ct_destruct_event( &pTail->pE);
        }

        if ( NULL == pTail->pNext)
            break;
        else
            pTail = pTail->pNext;
    }

    /* Prepend the list to the head of the free list */

    pTail->pNext = free_msgnode_list;
    free_msgnode_list = pM;

    /* Physically free any excess message nodes, so that */
    /* we don't tie up too much memory with unused nodes */

    while (free_msgnode_count> FREE_MSGNODE_MAX) {
        pM = free_msgnode_list;

        ASSERT( 234567L == pM->magic );

        free_msgnode_list = pM->pNext;
        ctMemory.freeMemory(pM);
        --free_msgnode_count;
    }
}

/*********************************************************************
 Free all message nodes.  This routine should be called only when all
 threads and message nodes have been destructed and the machinery is
 shutting down.
 ********************************************************************/

void CTDataStore::ct_free_all_msgnodes(void) {
    Ct_msgnode * pTemp;

    while (free_msgnode_list != NULL) {
        ASSERT( 234567L == free_msgnode_list->magic );
        pTemp = free_msgnode_list->pNext;
        ctMemory.freeMemory(free_msgnode_list);
        free_msgnode_list = pTemp;
    }

    free_msgnode_count = 0;
}

/* -------------------- Ct_event functions ------------------------- */

/*******************************************************************
 Allocate a Ct_event, from the free list if possible, from the
 heap if necessary.  We don't populate it here; we just allocate
 memory for it.
 ******************************************************************/

Ct_event * CTDataStore::ct_alloc_event(void) {
    Ct_event * pE;

    if ( NULL == free_event_list) {
        pE = (Ct_event *) ctMemory.allocMemory(sizeof(Ct_event));
        if ( NULL == pE) {
            CTOut::ct_report_error("ct_alloc_event: Out of memory");
            ctScheduler.ct_fatal_error();
        }
    }
    else {
        --free_event_count;
        pE = free_event_list;
        free_event_list = free_event_list->pNext;
    }

    return pE;
}

/*******************************************************************
 Deallocate a list of events by sticking them on the free list.
 Use this function only for a list of events that have not yet
 been dispatched, so that we can ignore the reference counts.
 ******************************************************************/

void CTDataStore::ct_destruct_event_list(Ct_event ** ppE) {
    Ct_event * pTail;
    Ct_event * pE;

    if ( NULL == ppE || NULL == *ppE)
        return;/* No list provided, or list is empty */

    pE = *ppE;
    *ppE = NULL;

    /* Find the tail of the list, meanwhile */
    /* freeing buffers as needed */

    pTail = pE;
    for (;;) {
        ++free_event_count;

        ASSERT( EVENT_MAGIC == pTail->magic );
        ASSERT( 0 == pTail->refcount );
#ifndef NDEBUG
        pTail->magic = 345678L;
#endif

        if (pTail->pData != NULL) {
            ctMemory.freeMemory(pTail->pData);
            pTail->pData = NULL;/* not necessary, just good hygiene */
        }
        if ( NULL == pTail->pNext)
            break;
        else
            pTail = pTail->pNext;
    }

    /* Prepend the list to the head of the free list */

    pTail->pNext = free_event_list;
    free_event_list = pE;

    /* Physically free any excess events, so that we  */
    /* don't tie up too much memory with unused nodes */

    while (free_event_count> FREE_EVENT_MAX) {
        pE = free_event_list;

        ASSERT( 345678L == pE->magic );
        free_event_list = pE->pNext;
        ctMemory.freeMemory(pE);
        --free_event_count;
    }
}

/*********************************************************************
 Free a single event by placing it on the free list.  Deallocate any
 associated memory.
 *********************************************************************/

void CTDataStore::ct_destruct_event(Ct_event ** ppE) {
    Ct_event * pE;

    ASSERT( ppE != NULL );
    if ( NULL == ppE || NULL == *ppE)
        return;

    pE = *ppE;
    *ppE = NULL;

    ASSERT( EVENT_MAGIC == pE->magic );
#ifndef NDEBUG
    pE->magic = 345678L;
#endif

    if (pE->pData != NULL) {
        ctMemory.freeMemory(pE->pData);
        pE->pData = NULL;/* not necessary, just good hygiene */
    }

    /* Prepend the dead event to the free event list */

    pE->pNext = free_event_list;
    free_event_list = pE;
    ++free_event_count;
}

/*********************************************************************
 Free all events that are on the free list.  This routine should be
 called only when all threads and event nodes have been destructed
 and the machinery is shutting down.
 ********************************************************************/

void CTDataStore::ct_free_all_events(void) {
    Ct_event * pTemp;

    while (free_event_list != NULL) {
        ASSERT( 345678L == free_event_list->magic );
        pTemp = free_event_list->pNext;
        ctMemory.freeMemory(free_event_list);
        free_event_list = pTemp;
    }

    free_event_count = 0;
}