
/*********************************************************************
 Routines for passing messages among cheap threads

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

 ********************************************************************/

#include "CTMessageTransport.h"

CTMessageTransport::CTMessageTransport() {
}

CTMessageTransport::CTMessageTransport( CTScheduler& ctScheduler,
        CTDataStore& ctDataStore, CTMemory& ctMemory ) {
    
    ctScheduler = ctScheduler;
    ctDataStore = ctDataStore;
    ctMemory = ctMemory; 
    
    
}


CTMessageTransport::~CTMessageTransport() {
}

/********************************************************************
 Send a message to a designated addressee
 *******************************************************************/

int CTMessageTransport::ct_send_msg(Ct_msgtype type, void * pData, size_t len, Ct_handle dest) {
    Ct_event * pE;

    if ( ! ctDataStore.ct_valid_handle( &dest) ) {
        /* Addressee doesn't exist.  We don't treat this as */
        /* an error because the addressee may have expired  */
        /* without the current thread's knowing about it.   */

        return CT_OKAY;
    }

    if (NULL == pData && len > 0) {
        CTOut::ct_report_error("ct_send_msg: No data provided");
        ctScheduler.ct_fatal_error();
        return CT_ERROR;
    }

    if ( 0 == type) {
        /* Zero is reserved to denote the absence of a message */

        CTOut::ct_report_error("ct_send_msg: Invalid message type");
        ctScheduler.ct_fatal_error();
        return CT_ERROR;
    }

    pE = construct_msg_event(type, pData, len, CT_DISPATCH_ADDRESSEE);
    if (NULL == pE)
        return CT_ERROR;
    else {
        pE->addressee = dest;

        /* Enqueue the event */

        return ctScheduler.ct_enqueue_event(pE);
    }
}

/********************************************************************
 Enqueue a designated thread for execution
 *******************************************************************/

int CTMessageTransport::ct_enqueue(Ct_handle dest) {
    Ct_event * pE;

    if ( ! ctDataStore.ct_valid_handle( &dest) ) {
        /* Addressee doesn't exist.  We don't treat this as */
        /* an error because the addressee may have expired  */
        /* without the current thread's knowing about it.   */

        return CT_OKAY;
    }

    pE = construct_enq_event( 0, CT_DISPATCH_ADDRESSEE);
    if (NULL == pE)
        return CT_ERROR;
    else {
        pE->addressee = dest;

        /* Enqueue the event */

        return ctScheduler.ct_enqueue_event(pE);
    }
}

/********************************************************************
 Send a message to whatever threads have subscribed to the specified
 message type
 *******************************************************************/

int CTMessageTransport::ct_distribute_msg(Ct_msgtype type, void * pData, size_t len) {
    Ct_event * pE;

    if (NULL == pData && len > 0) {
        CTOut::ct_report_error("ct_distribute_msg: No data provided");
        ctScheduler.ct_fatal_error();
        return CT_ERROR;
    }

    if ( 0 == type) {
        /* Zero is reserved to denote the absence of a message */

        CTOut::ct_report_error("ct_distribute_msg: Invalid message type");
        ctScheduler.ct_fatal_error();
        return CT_ERROR;
    }

    pE = construct_msg_event(type, pData, len, CT_DISPATCH_SUBSCRIBER);
    if (NULL == pE)
        return CT_ERROR;
    else {
        /* Enqueue the event */

        return ctScheduler.ct_enqueue_event(pE);
    }
}

/********************************************************************
 Enqueue whatever threads have subscribed to the specified message type
 *******************************************************************/

int CTMessageTransport::ct_distribute_enq(Ct_msgtype type) {
    Ct_event * pE;

    if ( 0 == type) {
        /* Zero is reserved to denote the absence of a message */

        CTOut::ct_report_error("ct_distribute_enq: Invalid message type");
        ctScheduler.ct_fatal_error();
        return CT_ERROR;
    }

    pE = construct_enq_event(type, CT_DISPATCH_SUBSCRIBER);
    if (NULL == pE)
        return CT_ERROR;
    else {
        /* Enqueue the event */

        return ctScheduler.ct_enqueue_event(pE);
    }
}

/********************************************************************
 Send a message to all threads
 *******************************************************************/

int CTMessageTransport::ct_broadcast_msg(Ct_msgtype type, void * pData, size_t len) {
    Ct_event * pE;

    if (NULL == pData && len > 0) {
        CTOut::ct_report_error("ct_broadcast_msg: No data provided");
        ctScheduler.ct_fatal_error();
        return CT_ERROR;
    }

    if ( 0 == type) {
        /* Zero is reserved to denote the absence of a message */

        CTOut::ct_report_error("ct_broadcast_msg: Invalid message type");
        ctScheduler.ct_fatal_error();
        return CT_ERROR;
    }

    pE = construct_msg_event(type, pData, len, CT_DISPATCH_ALL);
    if (NULL == pE)
        return CT_ERROR;
    else {
        /* Enqueue the event */

        return ctScheduler.ct_enqueue_event(pE);
    }
}

/********************************************************************
 Enqueue all threads
 *******************************************************************/

int CTMessageTransport::ct_broadcast_enq(void) {
    Ct_event * pE;

    pE = construct_enq_event( 0, CT_DISPATCH_ALL);
    if (NULL == pE)
        return CT_ERROR;
    else {
        /* Enqueue the event */

        return ctScheduler.ct_enqueue_event(pE);
    }
}

/********************************************************************
 Construct a message event
 *******************************************************************/

Ct_event * CTMessageTransport::construct_msg_event(Ct_msgtype type, void * pData,
        size_t len, Ct_dispatch_type dispatch_type) {
    Ct_event * pE;

    ASSERT(pData != NULL || 0 == len);
    ASSERT(type != 0);

    pE = ctDataStore.ct_alloc_event();
    if (NULL == pE)
        return NULL;

    /* Populate the event */

    pE->pNext = NULL;
    pE->type = type;
    pE->ev_type = CT_EV_MSG;
    pE->msg_len = len;
    pE->refcount = 0;
#ifndef NDEBUG
    pE->magic = EVENT_MAGIC;
#endif

    if (len > CT_MSG_BUF_LEN) {
        /* Allocate a copy of the data */

        pE->pData = ctMemory.allocMemory(len);
        if (NULL == pE->pData) {
            CTOut::ct_report_error("ct_construct_msg_event: Out of memory");
            ctScheduler.ct_fatal_error();
            pE->pData = NULL;
            pE->msg_len = 0;
            ctDataStore.ct_destruct_event( &pE);
            return NULL;
        }
        else
            memcpy(pE->pData, pData, len);
    }
    else {
        pE->pData = NULL;
        if (len > 0)
            memcpy(pE->buff, pData, len);
    }

    pE->dispatch_type = dispatch_type;
    return pE;
}

/********************************************************************
 Construct an enqueue event
 *******************************************************************/
Ct_event * CTMessageTransport::construct_enq_event(Ct_msgtype type,
        Ct_dispatch_type dispatch_type) {
    Ct_event * pE;

    pE = ctDataStore.ct_alloc_event();
    if (NULL == pE)
        return NULL;

    /* Populate the event */

    pE->pNext = NULL;
    pE->type = type;
    pE->ev_type = CT_EV_ENQ;
    pE->msg_len = 0;
    pE->refcount = 0;
#ifndef NDEBUG
    pE->magic = EVENT_MAGIC;
#endif
    pE->pData = NULL;
    pE->dispatch_type = dispatch_type;
    return pE;
}

/********************************************************************
 Fetch the header of the next pending message, if any, for the
 current thread.
 *******************************************************************/

Ct_msgheader CTMessageTransport::ct_query_msg(void) {
    Ct_handle self;
    Ct_msgheader hdr;

    self = ctScheduler.ct_self();
    if (NULL == self.p) {
        /* No current thread, so no current message either */

        hdr.type = 0;
        hdr.length = 0;
    }
    else {
        Ct_thread * pThread;
        Ct_msgnode * pNode;

        pThread = (Ct_thread * ) self.p;
        ASSERT(CT_MAGIC == pThread->magic);
        pNode = pThread->msg_q;
        if (NULL == pNode) {
            /* No message waiting */

            hdr.type = 0;
            hdr.length = 0;
        }
        else {
            const Ct_event * pE = pNode->pE;

            ASSERT(pNode->magic == MSGNODE_MAGIC);
            ASSERT(pE != NULL);
            ASSERT(pE->refcount > 0);

            hdr.type = pE->type;
            hdr.length = pE->msg_len;
        }
    }

    return hdr;
}

/********************************************************************
 Fetch the contents of the next pending message into a specified
 buffer.
 *******************************************************************/

int CTMessageTransport::ct_dequeue_msg(unsigned char * buff) {
    Ct_handle self;
    Ct_thread * pThread;
    Ct_msgnode * pNode;
    const Ct_event * pE;

    if (NULL == buff) {
        CTOut::ct_report_error("ct_dequeue_msg: no buffer provided");
        ctScheduler.ct_fatal_error();
        return CT_ERROR;
    }

    self = ctScheduler.ct_self();
    if (NULL == self.p) {
        CTOut::ct_report_error("ct_dequeue_msg: No thread active");
        ctScheduler.ct_fatal_error();
        return CT_ERROR;
    }

    pThread = (Ct_thread * ) self.p;
    ASSERT(CT_MAGIC == pThread->magic);
    pNode = pThread->msg_q;
    if (NULL == pNode) {
        /* No message waiting */

        CTOut::ct_report_error("ct_dequeue_msg: no pending message");
        ctScheduler.ct_fatal_error();
        return CT_ERROR;
    }

    ASSERT(MSGNODE_MAGIC == pNode->magic);

    pE = pNode->pE;
    ASSERT(pE != NULL);
    ASSERT(pE->refcount > 0);

    if (pE->msg_len > 0) {
        if (pE->msg_len > CT_MSG_BUF_LEN)
            memcpy(buff, pE->pData, pE->msg_len);
        else
            memcpy(buff, pE->buff, pE->msg_len);
    }

    /* Dequeue and discard the message */

    pThread->msg_q = pNode->pNext;
    pNode->pNext = NULL;
    ctDataStore.ct_destruct_msgnode_list( &pNode);

    return CT_OKAY;
}

/********************************************************************
 Dequeue and discard the next pending message, if any.
 *******************************************************************/

void CTMessageTransport::ct_discard_msg(void) {
    Ct_handle self;
    Ct_thread * pThread;
    Ct_msgnode * pNode;

    self = ctScheduler.ct_self();
    if (NULL == self.p) {
        CTOut::ct_report_error("ct_discard_msg: No thread active");
        ctScheduler.ct_fatal_error();
    }

    pThread = (Ct_thread * ) self.p;
    ASSERT(CT_MAGIC == pThread->magic);
    pNode = pThread->msg_q;
    if (NULL == pNode) {
        /* No message waiting */

        return;
    }

    ASSERT(MSGNODE_MAGIC == pNode->magic);

    /* Dequeue and discard the message */

    pThread->msg_q = pNode->pNext;
    pNode->pNext = NULL;

    ctDataStore.ct_destruct_msgnode_list( &pNode);

    return;
}
