
/* Routines for dispatching subscribed messages to subscribers 

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

 */

#include "CTMessageDispatcher.h"

    CTScheduler ctScheduler;
    CTDataStore ctDataStore;
    CTMemory  ctMemory;


CTMessageDispatcher::CTMessageDispatcher() {

    pFirst = NULL; /* List of message type lists */
    pLast = NULL;

    #define MAX_FREE_SUBS 10
    
    free_subs = NULL;
    free_sub_count = 0;

    #define MAX_FREE_HEADS 3
    
    free_heads= NULL;
    free_head_count = 0;

}

CTMessageDispatcher::CTMessageDispatcher( CTScheduler& ctScheduler,
        CTDataStore& ctDataStore, CTMemory& ctMemory ) {
    

    ctScheduler = ctScheduler;
    ctDataStore = ctDataStore;
    ctMemory = ctMemory;
    
}

CTMessageDispatcher::~CTMessageDispatcher() {
}

/************************************************************************
 Subscribe to a message type.  I.e. until further notice, a specified
 thread is to receive all distributed events of a specified message type.
 ***********************************************************************/

int CTMessageDispatcher::ct_subscribe(Ct_msgtype type, Ct_handle handle) {
    int rc= CT_OKAY;
    Ct_thread * pThread;
    Ct_sub * pSub;
    Sub_list_head * pHead;

    if ( !ctDataStore.ct_valid_handle( &handle) ) {
        CTOut::ct_report_error("ct_subscribe: invalid thread handle");
        ctMemory.freeMemory();
        ctScheduler.ct_fatal_error();
        return CT_ERROR;
    }
    else
        if ( 0 == type) {
            CTOut::ct_report_error("ct_subscribe: invalid message type");
            ctScheduler.ct_fatal_error();
            return CT_ERROR;
        }

    pThread = (Ct_thread *) handle.p;
    ASSERT( pThread != NULL );

    if (pThread->subscriptions != NULL) {
        /* See if we're already subscribed */

        pSub = pThread->subscriptions;
        while (pSub != NULL && pSub->type != type)
            pSub = pSub->pNext;
        if (pSub != NULL)
            return CT_OKAY;/* already subscribed */
    }

    /* Find the list of subscriptions for this message type */

    pHead = find_sub_head(type);
    if ( NULL == pHead)
        return CT_ERROR;

    /* Allocate and populate a subscription */

    pSub = alloc_sub();
    if ( NULL == pSub)
        return CT_ERROR;/* Out of memory */

    pSub->pNext_sub = NULL;
    pSub->pPrev_sub = NULL;
    pSub->type = type;
    pSub->handle = handle;

    /* Add the new subscription to this thread's list */

    pSub->pNext = pThread->subscriptions;
    pThread->subscriptions = pSub;

    /* Add the new subscription to the list for this message type */

    pSub->pNext_sub = pHead->sub.pNext_sub;
    pSub->pPrev_sub = &pHead->sub;
    pHead->sub.pNext_sub = pSub;
    pSub->pNext_sub->pPrev_sub = pSub;

    return rc;
}

/********************************************************************
 Stop sending subscribed events of a given type to a given thread.
 *******************************************************************/

int CTMessageDispatcher::ct_unsubscribe(Ct_msgtype type, Ct_handle handle) {

    int rc= CT_OKAY;
    Ct_thread * pThread;
    Ct_sub * pSub;
    Ct_sub * pPrev;

    if ( !ctDataStore.ct_valid_handle( &handle) )
        return CT_OKAY; /* No thread to unsubscribe */
    else
        if ( 0 == type) {
            CTOut::ct_report_error("ct_unsubscribe: invalid message type");
            ctScheduler.ct_fatal_error();
            return CT_ERROR;
        }

    pThread = (Ct_thread *) handle.p;
    ASSERT( pThread != NULL );

    pSub = pThread->subscriptions;
    if ( NULL == pSub)
        return CT_OKAY; /* No subscriptions */

    /* Find the specified subscription */

    pPrev = NULL;
    while (pSub != NULL && pSub->type != type) {
        pPrev = pSub;
        pSub = pSub->pNext;
    }

    if ( NULL == pSub)
        return CT_OKAY; /* Not subscribed to this type */

    /* Remove it from this thread's list of subscriptions */

    if ( NULL == pPrev)
        pThread->subscriptions = pSub->pNext;
    else
        pPrev->pNext = pSub->pNext;

    pSub->pNext = NULL;

    /* Remove it from the list of threads subscribed to this       */
    /* message type.  Here we treat it as a list of subscriptions, */
    /* although in this case it is a list with only one node.      */

    ct_destruct_sub_list( &pSub);

    return rc;
}

/**********************************************************************
 Stop sending any distributed events to a given thread.
 *********************************************************************/

void CTMessageDispatcher::ct_unsubscribe_all(Ct_handle handle) {
    if ( ! ctDataStore.ct_valid_handle( &handle) ) {
        return; /* No thread to unsubscribe */
    }
    else {
        Ct_thread * pThread = (Ct_thread *) handle.p;

        ASSERT( pThread != NULL );
        ct_destruct_sub_list( &pThread->subscriptions);
    }
}

/****************************************************************
 Distribute an event to all the threads that have subscribed to it.
 ***************************************************************/

int CTMessageDispatcher::ct_dispatch_subscription(Ct_event * pE) {
    int rc= CT_OKAY;
    Sub_list_head * pHead;
    Ct_sub * pSub;

    ASSERT( pE != NULL );
    ASSERT( EVENT_MAGIC == pE->magic );
    ASSERT( CT_DISPATCH_SUBSCRIBER == pE->dispatch_type );

    pHead = seek_sub_head(pE->type);
    if ( NULL == pHead || pHead->sub.type != pE->type)
        return CT_OKAY;/* No subscribers for this message type */

    ASSERT( pHead->sub.pNext_sub != &pHead->sub );
    /* list not empty */
    pSub = pHead->sub.pNext_sub;

    do {
        /* Deliver the event to each subscriber */

        ASSERT( pSub != NULL );
        ASSERT( ctDataStore.ct_valid_handle( &pSub->handle ) );

        rc = ctScheduler.ct_deliver_event(pE, (Ct_thread *) pSub->handle.p );
        if (rc != CT_OKAY)
            break;

        pSub = pSub->pNext_sub;
    }
    while (pSub != &pHead->sub);

    return rc;
}

/*************************************************************************
 Look for the Sub_list_head for a given message type.  If you don't find
 it, make one, and add it to the list.  Return a pointer to the new
 Sub_list_head (or NULL if out of memory).
 ************************************************************************/

Sub_list_head * CTMessageDispatcher::find_sub_head(Ct_msgtype type) {
    Sub_list_head * pHead;
    Sub_list_head * pNew_head;

    pHead = seek_sub_head(type);
    if (pHead != NULL && pHead->sub.type == type)
        return pHead;/* bingo */

    /* Otherwise construct and populate a new one.  The Ct_sub      */
    /* member will start out pointing to itself in both directions. */

    pNew_head = alloc_head();
    if ( NULL == pNew_head)
        return NULL;

    pNew_head->sub.pNext_sub = &pNew_head->sub;
    pNew_head->sub.pPrev_sub = &pNew_head->sub;
    pNew_head->sub.pNext = NULL;
    pNew_head->sub.type = type;
    pNew_head->sub.handle.p = NULL;
    pNew_head->sub.handle.incarnation = USHRT_MAX;

    /* Add it to the list of list heads */

    if ( NULL == pFirst) {
        /* List was originally empty */

        ASSERT( NULL == pLast );
        pNew_head->pNext = NULL;
        pNew_head->pPrev = NULL;
        pFirst = pLast = pNew_head;
    }
    else
        if ( NULL == pHead) {
            /* No existing successor; add to the end */

            pNew_head->pNext = NULL;
            pNew_head->pPrev = pLast;
            pLast->pNext = pNew_head;
            pLast = pNew_head;
        } else {
            /* Insert prior to its existing successor */

            pNew_head->pNext = pHead;
            pNew_head->pPrev = pHead->pPrev;
            if ( NULL == pHead->pPrev)
                pFirst = pNew_head; /* beginning of list */
            else
                pHead->pPrev->pNext = pNew_head; /* end of list */
            pHead->pPrev = pNew_head;
        }

    return pNew_head;
}

/*************************************************************************
 Look for the Sub_list_head for a given message type.  If you find it,
 return a pointer to it.  Otherwise return a pointer to the one
 following its position in the list, or NULL if there is no such
 successor.
 ************************************************************************/

Sub_list_head * CTMessageDispatcher::seek_sub_head(Ct_msgtype type) {
    Sub_list_head * pHead;

    pHead = pFirst;

    while (pHead != NULL && pHead->sub.type < type)
        pHead = pHead->pNext;

    return pHead;
}

/*************************************************************************
 Allocate a Ct_sub, from the free list if possible, from the heap if
 necessary.  We don't populate the Ct_sub here; we just allocate memory.
 ************************************************************************/
Ct_sub * CTMessageDispatcher::alloc_sub(void) {
    Ct_sub * pSub;

    if ( NULL == free_subs)
        pSub = (Ct_sub*) ctMemory.allocMemory(sizeof(Ct_sub));
    else {
        pSub = free_subs;
        free_subs = free_subs->pNext;
    }

    if ( NULL == pSub) {
        CTOut::ct_report_error("alloc_sub: Out of memory");
        ctScheduler.ct_fatal_error();
    }

    return pSub;
}

/*************************************************************************/
/* Destruct all the subs in a list.  This function is designed mainly to */
/* unsubscribe a thread.  There is no need for a function to destruct    */
/* all the subs for a given message type.   When the scheduler completes */
/* it will destruct all the threads, and the threads will unsubscribe    */
/* themselves.                                                           */
/*************************************************************************/

void CTMessageDispatcher::ct_destruct_sub_list(Ct_sub ** ppSub) {
    Ct_sub * pSub;
    Ct_sub * pTail;

    ASSERT( ppSub != NULL );
    pSub = *ppSub;
    *ppSub = NULL;
    if ( NULL == pSub)
        return;

    /* Find the tail of the list, detaching each Sub from its neighbors */
    /* along the way. */

    pTail = pSub;
    for (;;) {
        ASSERT( pTail->pNext_sub != NULL );
        ASSERT( pTail->pPrev_sub != NULL );

        /* Detach current Sub from its neighbors */

        pTail->pNext_sub->pPrev_sub = pTail->pPrev_sub;
        pTail->pPrev_sub->pNext_sub = pTail->pNext_sub;

        /* If the list from which we just      */
        /* detached is now empty, destruct it. */

        if (pTail->pNext_sub == pTail->pPrev_sub) {
            /* The only remaining member of the circular list is */
            /* the dummy node at the head of the list.  Since    */
            /* the list is now empty, discard the head.          */

            /* The following cast relies on the fact that the    */
            /* Sub_list_head contains a Ct_sub as its first      */
            /* member, so the pointers are equivalent:           */

            discard_head( (Sub_list_head *) pTail->pNext_sub );
        }

        /* Not necessary, but good hygiene: */

        pTail->pNext_sub = pTail->pPrev_sub = NULL;

        ++free_sub_count;

        if ( NULL == pTail->pNext)
            break;
        else
            pTail = pTail->pNext;
    }

    /* Prepend the list to the free list */

    pTail->pNext = free_subs;
    free_subs = pSub;

    while (free_sub_count > MAX_FREE_SUBS) {
        ASSERT( free_subs != NULL );

        pSub = free_subs;
        free_subs = free_subs->pNext;
        ctMemory.freeMemory(pSub);
        --free_sub_count;
    }
}

/************************************************************************
 Remove the list head from the list of list heads and deallocate it.
 ***********************************************************************/

void CTMessageDispatcher::discard_head(Sub_list_head * pHead) {
    ASSERT( pHead != NULL );

    if ( NULL == pHead->pPrev) {
        ASSERT( pFirst == pHead );
        pFirst = pHead->pNext;
    }
    else
        pHead->pPrev->pNext = pHead->pNext;

    if ( NULL == pHead->pNext) {
        ASSERT( pLast == pHead );
        pLast = pHead->pPrev;
    }
    else
        pHead->pNext->pPrev = pHead->pPrev;

    dealloc_head( &pHead);
}

/*************************************************************************
 Allocate a Sub_list_head, from the free list if possible, from the heap
 if necessary.  We don't populate it here; we just allocate memory.
 ************************************************************************/

Sub_list_head * CTMessageDispatcher::alloc_head(void) {
    Sub_list_head * pHead;

    if ( NULL == free_heads)
        pHead = (Sub_list_head*) (ctMemory.allocMemory(sizeof(Sub_list_head)) );
    else {
        pHead = free_heads;
        free_heads = free_heads->pNext;
    }

    if ( NULL == pHead) {
        CTOut::ct_report_error("alloc_head: Out of memory");
        ctScheduler.ct_fatal_error();
    }

    return pHead;
}

/********************************************************************
 Remove a Sub_head_list from the list where it's embedded and either
 transfer it to the free list or, if the free list is full,
 physically free it.
 *******************************************************************/

void CTMessageDispatcher::dealloc_head(Sub_list_head **ppHead) {
    Sub_list_head * pHead;

    ASSERT( ppHead != NULL );
    if ( NULL == ppHead)
        return;

    pHead = *ppHead;
    *ppHead = NULL;
    ASSERT( pHead != NULL );
    if ( NULL == pHead)
        return;

    /* Assert that the list is empty */

    ASSERT( pHead->sub.pNext_sub == &pHead->sub );
    ASSERT( pHead->sub.pPrev_sub == &pHead->sub );

    /* Detach it from the list of Sub_list_heads */

    if ( NULL == pHead->pPrev)
        pFirst = pHead->pNext;
    else
        pHead->pPrev->pNext = pHead->pNext;

    if ( NULL == pHead->pNext)
        pLast = pHead->pPrev;
    else
        pHead->pNext->pPrev = pHead->pPrev;

    /* Deallocate it */

    if (free_head_count < MAX_FREE_HEADS) {
        pHead->pNext = free_heads;
        free_heads = pHead;
        ++free_head_count;
    }
    else
        ctMemory.freeMemory(pHead);
}

/********************************************************************
 Physically free all the Ct_subs and Sub_list_heads on the free lists.
 This function should be called when the scheduler has destructed all
 the threads.
 *******************************************************************/

void CTMessageDispatcher::ct_free_subscriptions(void) {
    Ct_sub * pSub;
    Sub_list_head * pHead;

    ASSERT( NULL == pFirst );
    ASSERT( NULL == pLast );

    while (free_subs != NULL) {
        pSub = free_subs->pNext;
        ctMemory.freeMemory(free_subs);
        free_subs = pSub;
    }
    free_sub_count = 0;

    while (free_heads != NULL) {
        pHead = free_heads->pNext;
        ctMemory.freeMemory(free_heads);
        free_heads = pHead;
    }
    free_head_count = 0;
}
