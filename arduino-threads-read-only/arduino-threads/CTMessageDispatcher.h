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

#ifndef CTMESSAGEDISPATCHER_H_
#define CTMESSAGEDISPATCHER_H_

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "ct.h"
#include "ctpriv.h"

#include "CTScheduler.h"
#include "CTMemory.h"
#include "CTOut.h"
#include "CTAssert.h"

/* A Ct_sub represents the request by a given thread to receive all  */
/* distributed messages of a given type.   Each Ct_sub belongs to    */
/* two different lists: A singly linked list of all the Ct_subs for  */
/* for the same thread, and a doubly linked circular list of all the */
/* Ct_subs to the same message type.   This structure enables us to  */
/* dispatch a message quickly and also to unsubscribe quickly.       */

struct Ct_sub {
        struct Ct_sub * pNext_sub; /* same message type */
        struct Ct_sub * pPrev_sub; /* same message type */
        struct Ct_sub * pNext; /* same thread */
        Ct_msgtype type;
        Ct_handle handle;
};

/* The Ct_sub within a Sub_list_head serves an anchor for a       */
/* doubly linked circular list of all the subscription to a given */
/* message type.  We maintain a doubly linked list of all the     */
/* Sub_list_heads, ordered by message type. */

/* This relatively simple design is suitable if there are only a */
/* few subscribed message types.  It's not so good if there are  */
/* many subscribed message types, due to the need for a linear   */
/* search to find the right list.  In that case a hash table or  */
/* tree might be more appropriate.   Typically, however, the     */
/* overhead of finding the list will be swamped by the overhead  */
/* of delivering the message. */

struct Sub_list_head {
        Ct_sub sub;
        struct Sub_list_head * pNext;
        struct Sub_list_head * pPrev;
};

typedef struct Sub_list_head Sub_list_head;

class CTMessageDispatcher {

    public:
        CTMessageDispatcher();
        CTMessageDispatcher::CTMessageDispatcher(CTScheduler& pCTScheduler,
                CTDataStore& ctDataStore, CTMemory& ctMemory );
        virtual ~CTMessageDispatcher();

    private:

        Sub_list_head *pFirst; /* List of message type lists */
        Sub_list_head *pLast;

#define MAX_FREE_SUBS 10
        Ct_sub * free_subs;
        unsigned free_sub_count;

#define MAX_FREE_HEADS 3
        Sub_list_head * free_heads;
        unsigned free_head_count;

        /************************************************************************
         Subscribe to a message type.  I.e. until further notice, a specified
         thread is to receive all distributed events of a specified message type.
         ***********************************************************************/

        int ct_subscribe(Ct_msgtype type, Ct_handle handle);

        /********************************************************************
         Stop sending subscribed events of a given type to a given thread.
         *******************************************************************/

        int ct_unsubscribe(Ct_msgtype type, Ct_handle handle);

        /**********************************************************************
         Stop sending any distributed events to a given thread.
         *********************************************************************/

        void ct_unsubscribe_all(Ct_handle handle);

        /****************************************************************
         Distribute an event to all the threads that have subscribed to it.
         ***************************************************************/

        int ct_dispatch_subscription(Ct_event * pE);

        /*************************************************************************
         Look for the Sub_list_head for a given message type.  If you don't find
         it, make one, and add it to the list.  Return a pointer to the new
         Sub_list_head (or NULL if out of memory).
         ************************************************************************/

        Sub_list_head * find_sub_head(Ct_msgtype type);

        /*************************************************************************
         Look for the Sub_list_head for a given message type.  If you find it,
         return a pointer to it.  Otherwise return a pointer to the one
         following its position in the list, or NULL if there is no such
         successor.
         ************************************************************************/

        Sub_list_head * seek_sub_head(Ct_msgtype type);

        /*************************************************************************
         Allocate a Ct_sub, from the free list if possible, from the heap if
         necessary.  We don't populate the Ct_sub here; we just allocate memory.
         ************************************************************************/

        Ct_sub * alloc_sub(void);

        /*************************************************************************/
        /* Destruct all the subs in a list.  This function is designed mainly to */
        /* unsubscribe a thread.  There is no need for a function to destruct    */
        /* all the subs for a given message type.   When the scheduler completes */
        /* it will destruct all the threads, and the threads will unsubscribe    */
        /* themselves.                                                           */
        /*************************************************************************/

        void ct_destruct_sub_list(Ct_sub ** ppSub);

        /************************************************************************
         Remove the list head from the list of list heads and deallocate it.
         ***********************************************************************/

        void discard_head(Sub_list_head * pHead);

        /*************************************************************************
         Allocate a Sub_list_head, from the free list if possible, from the heap
         if necessary.  We don't populate it here; we just allocate memory.
         ************************************************************************/

        Sub_list_head * alloc_head(void);

        /********************************************************************
         Remove a Sub_head_list from the list where it's embedded and either
         transfer it to the free list or, if the free list is full,
         physically free it.
         *******************************************************************/

        void dealloc_head(Sub_list_head **ppHead);

        /********************************************************************
         Physically free all the Ct_subs and Sub_list_heads on the free lists.
         This function should be called when the scheduler has destructed all
         the threads.
         *******************************************************************/

        void ct_free_subscriptions(void);

};

#endif /*CTMESSAGEDISPATCHER_H_*/
