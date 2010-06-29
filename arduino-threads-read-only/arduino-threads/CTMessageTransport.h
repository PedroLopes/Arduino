
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

#ifndef CTMESSAGETRANSPORT_H_
#define CTMESSAGETRANSPORT_H_

#include <stdlib.h>
#include <string.h>
#include "ct.h"
#include "ctpriv.h"

#include "CTScheduler.h"
#include "CTDataStore.h"
#include "CTOut.h"
#include "CTAssert.h"

class CTMessageTransport {

    public:

        CTMessageTransport();
        CTMessageTransport( CTScheduler& ctScheduler,
                CTDataStore& ctDataStore, CTMemory& ctMemory );
        virtual ~CTMessageTransport();
        
    private:
        
        CTScheduler ctScheduler;
        CTDataStore ctDataStore;
        CTMemory ctMemory;

        /********************************************************************
         Send a message to a designated addressee
         *******************************************************************/
        
        int ct_send_msg(Ct_msgtype type, void * pData, size_t len,
                Ct_handle dest);
        
        /********************************************************************
         Enqueue a designated thread for execution
         *******************************************************************/
        
        int ct_enqueue(Ct_handle dest);
        
        /********************************************************************
         Send a message to whatever threads have subscribed to the specified
         message type
         *******************************************************************/
        
        int ct_distribute_msg(Ct_msgtype type, void * pData, size_t len);
        
        /********************************************************************
         Enqueue whatever threads have subscribed to the specified message type
         *******************************************************************/
        
        int ct_distribute_enq(Ct_msgtype type);
        
        /********************************************************************
         Send a message to all threads
         *******************************************************************/
        
        int ct_broadcast_msg(Ct_msgtype type, void * pData, size_t len);
        
        /********************************************************************
         Enqueue all threads
         *******************************************************************/
        
        int ct_broadcast_enq(void);
        
        /********************************************************************
         Construct a message event
         *******************************************************************/
        
         Ct_event * construct_msg_event(Ct_msgtype type, void * pData,
                size_t len, Ct_dispatch_type dispatch_type);
        
        /********************************************************************
         Construct an enqueue event
         *******************************************************************/
        
         Ct_event * construct_enq_event(Ct_msgtype type,
                Ct_dispatch_type dispatch_type);
        
        /********************************************************************
         Fetch the header of the next pending message, if any, for the
         current thread.
         *******************************************************************/
        
        Ct_msgheader ct_query_msg(void);
        
        /********************************************************************
         Fetch the contents of the next pending message into a specified
         buffer.
         *******************************************************************/
        
        int ct_dequeue_msg(unsigned char * buff);
        
        /********************************************************************
         Dequeue and discard the next pending message, if any.
         *******************************************************************/
        
        void ct_discard_msg(void);
        
};

#endif /*CTMESSAGETRANSPORT_H_*/
