#ifndef CTSCHEDULER_H_
#define CTSCHEDULER_H_

#include "ct.h"
#include "ctutil.h"
#include "ctpriv.h"

//#include "CTDataStore.h"
#include "CTOut.h"
#include "CTAssert.h"

class CTDataStore;

class CTScheduler {

#ifdef CT_RETURN

        /* to be used by ct_return(): */

        jmp_buf jumper;
        int jump_rc; /* status code returned by thread */

#endif

    public:
        
        CTScheduler();
        CTScheduler( CTDataStore& ctDataStore );
        virtual ~CTScheduler();
        
        void ct_fatal_error(void);
        
        Ct_handle ct_self(void);
        int ct_enqueue_event(Ct_event * pE);
        int ct_deliver_event(Ct_event * pE, Ct_thread * pT);
            

        // library-accessible "private" interface
        
    private:
        
        CTDataStore& ctDataStorep;
        
        /* Array of thread lists, each representing */
        /* a different priority level: */

        Ct_thread pri_q[CT_PRIORITY_MAX + 1 ];

        /* Dummy heading a doubly-linked list of */
        /* threads waiting on an event */

        Ct_thread sleepers;
        Ct_thread *pCurr_thread;

        /* Ptrs to head and tail of event queue */

        Ct_event * ev_head;
        Ct_event * ev_tail;

        /* Index into priority queue of current thread, if any: */

        int curr_priority;

        unsigned pri_penalty;
        unsigned init_countdown;
        unsigned countdown;

        int opened; /* boolean for enforcing initialization */
        int fatal_error; /* boolean for noting fatal error */
        int halted; /* boolean for halting scheduler  */
        int self_msg; /* boolean for message to self */

        /* function pointers for user exits, to be invoked */
        /* just before and just after each thread step:    */

        Ct_user_exit pre_function;
        Ct_user_exit post_function;


        void pick_thread(void);
        int step();
        int insert_thread(Ct_thread * pThread);
        void ct_wait_on_timeout(unsigned long interval);
        void insert_timeout(void);
        int check_timeouts(void);
        Ct_time default_clock(void);
        int ct_timecmp(const Ct_time * t1, const Ct_time * t2);
        void scrunch_queue(void);
        void append_queue(int from, int to);
        int ct_create_thread(Ct_handle * pHandle, int priority, void * pData,
                Ct_step_function step, Ct_destructor destruct);
        int ct_create_sleeping_thread(Ct_handle * pHandle, int priority,
                void * pData, Ct_step_function step, Ct_destructor destruct);
        void clear_priority_queue(void);
        void destruct_thread_list(Ct_thread ** ppFirst);
        void ct_clear(void);
        void clean_up_all(void);
        Ct_user_exit ct_install_pre_function(Ct_user_exit f);
        Ct_user_exit ct_install_post_function(Ct_user_exit f);
        unsigned ct_set_countdown(unsigned n);
        void ct_penalize(unsigned penalty);
        void ct_halt(void);
        int ct_exit(void);
        int ct_wait(void);
        void * ct_self_data(void);
        void dispatch_event_queue(void);
        void dispatch_all(Ct_event * pE);
        int broadcast_to_queue(Ct_event * pE, Ct_thread * pT);
        void attach_msg(Ct_msgnode * pM, Ct_thread * pT);
        void enqueue(Ct_thread * pT);
        void dispatch_addressee(Ct_event * pE);
        void ct_open(void);
        void ct_return(int rc);
        
#if defined CT_TIMEOUT
   
        void Ct_clock ct_install_clock(Ct_clock clock_function);
#endif

};

#endif /*CTSCHEDULER_H_*/
