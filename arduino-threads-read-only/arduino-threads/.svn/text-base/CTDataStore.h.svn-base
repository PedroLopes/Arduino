
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
7 
 **************************************************************/

#ifndef CTDATASTORE_H_
#define CTDATASTORE_H_

#include <stdlib.h>
#include "ct.h"
#include "ctpriv.h"

#include "CTScheduler.h"
#include "CTOut.h"
#include "CTMemory.h"
#include "CTAssert.h"

class CTDataStore {
    
    public:
        
        CTDataStore();
        CTDataStore( CTScheduler& ctScheduler,
                CTMemory& ctMemory);
        virtual ~CTDataStore();        

        /*****************************************************************
         Return CT_TRUE if the handle points to a valid Ct_thread, and
         CT_FALSE otherwise.

         Note that this function is not safe if passed a wild pointer, or
         a pointer to a handle containing a wild pointer.  The safest
         implementation would be to maintain a list of valid threads, and
         search it as needed.  Even that approach won't avoid a hardware
         exception in some architectures.
         ****************************************************************/
        
        int ct_valid_handle(const Ct_handle * pH);
        
        /*********************************************************************
         Free all threads.  This routine should be called only when all
         threads have been destructed and the machinery is shutting down.
         ********************************************************************/
        
        void ct_free_all_threads(void);
        
        /*******************************************************************
         Deallocate a list of events by sticking them on the free list.
         Use this function only for a list of events that have not yet
         been dispatched, so that we can ignore the reference counts.
         ******************************************************************/
        
        void ct_destruct_event_list(Ct_event ** ppE);
        
        /*******************************************************************
         Deallocate a list of message nodes by sticking them on the free
         list.  Deallocate associated memory, and detach from the
         associated event.
         ******************************************************************/
        
        void ct_destruct_msgnode_list(Ct_msgnode ** ppM);
        
        /* -------------------- Ct_event functions ------------------------- */
        
        /*******************************************************************
         Allocate a Ct_event, from the free list if possible, from the
         heap if necessary.  We don't populate it here; we just allocate
         memory for it.
         ******************************************************************/
        
        Ct_event * ct_alloc_event(void);

        /*********************************************************************
         Free a single event by placing it on the free list.  Deallocate any
         associated memory.
         *********************************************************************/
        
        void ct_destruct_event(Ct_event ** ppE);
    
    private:  

        Ct_thread *free_ct_list;
        int free_ct_count;

        Ct_msgnode *free_msgnode_list;
        int free_msgnode_count;

        Ct_event *free_event_list;
        int free_event_count;
        
        /*****************************************************************
         Return CT_TRUE if two handles refer to the same incarnation of
         the same thread.  Otherwise return CT_FALSE.
         ****************************************************************/
        
        int ct_same_thread(const Ct_handle * pH_1, const Ct_handle * pH_2);
            
        /*****************************************************************
         Allocate and initialize a Ct_thread.
         ****************************************************************/
        
        Ct_thread * ct_construct(int priority, void * pData,
                Ct_step_function step, Ct_destructor destruct);
        
        /*****************************************************************
         Destruct and deallocate a cheap thread.  Call the destructor
         callback function if there is one.  Also destruct any pending
         messages.
         ****************************************************************/
        
        void ct_destruct(Ct_thread ** ppThread);
        
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
        
        Ct_thread * alloc_ct(void);
        
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
        
         void free_ct(Ct_thread ** ppThread);

         
        /* ---------------- msgnode functions: ----------------------------- */

         
        /*******************************************************************
         Allocate a Ct_msgnode, from the free list if possible, from the
         heap if necessary.  We don't populate it here; we just allocate
         memory for it.
         ******************************************************************/
        
        Ct_msgnode * ct_alloc_msgnode(void);
        
        /*********************************************************************
         Free all message nodes.  This routine should be called only when all
         threads and message nodes have been destructed and the machinery is
         shutting down.
         ********************************************************************/
        
        void ct_free_all_msgnodes(void);

        
        /* -------------------- Ct_event functions ------------------------- */
        
        
        /*********************************************************************
         Free all events that are on the free list.  This routine should be
         called only when all threads and event nodes have been destructed
         and the machinery is shutting down.
         ********************************************************************/
        
        void ct_free_all_events(void);
        
};
        
#endif /*CTDATASTORE_H_*/
