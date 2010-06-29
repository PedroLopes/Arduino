
#include "CTScheduler.h"
        
class CTDataStore;

// Constructor /////////////////////////////////////////////////////////////////
// Function that handles the creation and setup of instances

CTScheduler::CTScheduler() {
    
    pCurr_thread = NULL;

    /* Ptrs to head and tail of event queue */

    ev_head = NULL;
    ev_tail = NULL;

    /* Index into priority queue of current thread, if any: */

    curr_priority = -1;

    pri_penalty = 0;
    init_countdown = CT_DEFAULT_COUNTDOWN;
    
    //unsigned CTScheduler::countdown;

    opened = 0; /* boolean for enforcing initialization */
    fatal_error = 0; /* boolean for noting fatal error */
    halted = 0; /* boolean for halting scheduler  */
    self_msg = 0; /* boolean for message to self */

    /* function pointers for user exits, to be invoked */
    /* just before and just after each thread step:    */

    pre_function = NULL;
    post_function = NULL;
 
}


/*****************************************************************
 Enter a (potentially endless) loop, invoking different active
 threads in succession.
 ****************************************************************/

CTScheduler::CTScheduler( CTDataStore& ctDataStore ) {
    
    CTScheduler();
    
    ctDataStore = ctDataStore;

    int rc = CT_OKAY;

    if (curr_priority >= 0) {
        CTOut::ct_report_error("ct_schedule: illegal recursive call");
        ct_fatal_error();

        /* In case of recursion we do an early return in order to  */
        /* avoid calling ct_clean_up_all() more than once.  The outer */
        /* layer of this function will call it on the way out. */

        /*  return CT_ERROR; WHATDOWEDO WITHTHIS?  */
    }
    else
        if ( !opened) {
            CTOut::ct_report_error("ct_schedule: not opened");
            ct_fatal_error();
            /*  return CT_ERROR; WHATDOWEDO WITHTHIS?  */
        }

    countdown = init_countdown;

    while ( !halted) {
        /* Dispatch any pending events to the relevant threads */

        if (ev_head != NULL)
            dispatch_event_queue();

#if defined CT_TIMEOUT

        /* Enqueue any threads that have timed out */

        if( check_timeouts() )
        dispatch_event_queue();
#endif

        /* Select the next thread to run */

        pick_thread();
        if (NULL == pCurr_thread) {
#if defined CT_TIMEOUT
            if( CT_STATUS_TIMEOUT == sleepers.pNext->status )
            /* Keep waiting for timeout */

            continue;
            else
#endif
            /* No active threads -- we're done. */

            break;
        }
        else {
            ASSERT( CT_MAGIC == pCurr_thread->magic );
            rc = step();
            if (rc != CT_OKAY)
                break;
        }

        --countdown;
        if ( 0 == countdown) {
            scrunch_queue();
            countdown = init_countdown;
        }
    }

    if (fatal_error) {
        CTOut::ct_report_error("ct_schedule: halted with fatal error");
        rc = CT_ERROR;
    }

    clean_up_all();

    /*  return rc;  WHAT IS THIS FOR?  */

}

CTScheduler::~CTScheduler() {
}

/****************************************************************
 Select the next thread for execution.  Leave the results in
 pCurr_thread and curr_priority (NULL and -1, respectively, if
 there are no threads to be scheduled).
 ***************************************************************/

void CTScheduler::pick_thread(void) {
    int i;

    /* Scan the priority levels for a non-empty queue, i.e. one */
    /* that doesn't just point to itself: */

    for (i = 0; i <= CT_PRIORITY_MAX; ++i) {
        if (pri_q[ i ].pNext != pri_q + i) {
            pCurr_thread = pri_q[ i ].pNext;
            ASSERT( CT_MAGIC == pCurr_thread->magic );
            curr_priority = i;
            return;
        }
    }

    /* If we reach this point, then we didn't find a thread. */

    pCurr_thread = NULL;
    curr_priority = -1;
}

/****************************************************************
 Invoke the specified cheap thread for one step.  If the thread
 is now defunct, discard it.  Otherwise move it to the appropriate
 priority queue, or to the sleeper list.
 ***************************************************************/

int CTScheduler::step() {
    int rc= CT_OKAY;
    int thread_rc;

    ASSERT( pCurr_thread != NULL );
    ASSERT( CT_MAGIC == pCurr_thread->magic );

    /* Invoke the pre-function, if one has been installed */

    if (pre_function != NULL) {
        rc = pre_function(pCurr_thread->pData);
        if (rc != CT_OKAY)
            return CT_ERROR;
    }

    /* Invoke the thread for one step, then any post-function: */

#ifdef CT_RETURN
    if( setjmp( jumper ) ) /* if returning from longjmp() */
    thread_rc = jump_rc; /* as set by ct_return() */
    else
#endif
    thread_rc = pCurr_thread->step(pCurr_thread->pData);

    if (post_function != NULL && CT_OKAY == thread_rc)
        thread_rc = post_function(pCurr_thread->pData);

    /* Remove the thread from the priority queue by stitching  */
    /* the adjacent threads together.   We'll let the thread's */
    /* own forward and backward pointers dangle harmlessly for */
    /* a little while... */

    ASSERT( CT_MAGIC == pCurr_thread->pNext->magic );
    ASSERT( CT_MAGIC == pCurr_thread->pPrev->magic );

    pCurr_thread->pNext->pPrev = pCurr_thread->pPrev;
    pCurr_thread->pPrev->pNext = pCurr_thread->pNext;

    /* Don't let a thread put itself to sleep if */
    /* it still has a message in its input queue */

    if (pCurr_thread->msg_q != NULL && CT_STATUS_ASLEEP == pCurr_thread->status)
        pCurr_thread->status = CT_STATUS_ACTIVE;

    switch (pCurr_thread->status) {
        case CT_STATUS_ACTIVE:
        case CT_STATUS_AWAKENED:

            /* Reinsert the thread into the priority queue */

            rc = insert_thread(pCurr_thread);
            break;

        case CT_STATUS_ASLEEP:

            /* Add the thread to the tail of the sleeper list. */

            pCurr_thread->pPrev = sleepers.pPrev;
            pCurr_thread->pNext = &sleepers;
            sleepers.pPrev->pNext = pCurr_thread;
            sleepers.pPrev = pCurr_thread;
            break;

#if defined CT_TIMEOUT
            case CT_STATUS_TIMEOUT :

            insert_timeout();
            break;
#endif

        case CT_STATUS_DEFUNCT:

            /* Deallocate the thread.  This function call sets */
            /* pCurr_thread to NULL.  Consequently any message */
            /* dispatching later will know better than to send */
            /* a message to this defunct thread. */

            ctDataStore.ct_destruct( &pCurr_thread);

            break;

        default:
            CTOut::ct_report_error("step: invalid thread status");
            rc = CT_ERROR;
            break;
    }

    if (thread_rc != CT_OKAY)
        rc = CT_ERROR;

    return rc;
}

/*****************************************************************
 Append the specified cheap thread to the queue at the appropriate
 priority level.
 ****************************************************************/

int CTScheduler::insert_thread(Ct_thread * pThread) {
    int i;

    ASSERT( pThread != NULL );
    ASSERT( CT_MAGIC == pThread->magic );

    i = pThread->priority;
    ASSERT( i <= CT_PRIORITY_MAX );
    ASSERT( i >= 0 );

    pThread->status = CT_STATUS_ACTIVE;

    /* Impose penalty, if any, but keep the result in bounds */

    i += pri_penalty;
    pri_penalty = 0;
    if (i > CT_PRIORITY_MAX)
        i = CT_PRIORITY_MAX;

    /* If the thread still has a message in its input queue,   */
    /* enqeue it at the highest priority, despite any penalty. */

    if (pThread->msg_q != NULL)
        i = 0;

    /* Juggle the pointers, appending */
    /* the new thread to the tail     */

    pThread->pNext = pri_q + i;
    pThread->pPrev = pri_q[ i ].pPrev;
    pri_q[ i ].pPrev = pThread;
    pThread->pPrev->pNext = pThread;

    return CT_OKAY;
}

#if defined CT_TIMEOUT
/********************************************************************
 Put the current thread to sleep for the specified interval.
 *******************************************************************/

int CTScheduler::ct_wait_on_timeout( unsigned long interval )
{
    if( NULL == pCurr_thread )
    {
        ct_report_error( "ct_wait_on_timeout: no thread is active" );
        ct_fatal_error();
        return CT_ERROR;
    }
    else
    {
        ASSERT( CT_MAGIC == pCurr_thread->magic );

        pCurr_thread->status = CT_STATUS_TIMEOUT;
        pCurr_thread->deadline = ticker();

        if( ULONG_MAX - pCurr_thread->deadline.tick < interval )
        ++pCurr_thread->deadline.era;
        pCurr_thread->deadline.tick += interval;
        return CT_OKAY;
    }
}

/********************************************************************
 Insert a thread at or near the head of the sleeper queue.  If any
 threads are awaiting a timeout, we store them at the head end of
 the sleeper queue, sorted by deadline.
 *******************************************************************/

void CTScheduler::insert_timeout( void )
{
    Ct_time deadline = pCurr_thread->deadline;
    Ct_thread * pThread;

    /* Find the thread following the proper spot to put */
    /* the current thread, in order.  Since the list is */
    /* circular, anchored by a dummy thread with status */
    /* != CT_STATUS_TIMEOUT, we know the loop will terminate. */

    for( pThread = sleepers.pNext;
            CT_STATUS_TIMEOUT == pThread->status
            && ct_timecmp( &pThread->deadline, &deadline ) <= 0;
            pThread = pThread->pNext )
    ;

    /* Insert the current thread before */
    /* the thread that we just found.   */

    pCurr_thread->pNext = pThread;
    pCurr_thread->pPrev = pThread->pPrev;
    pThread->pPrev->pNext = pCurr_thread;
    pThread->pPrev = pCurr_thread;
}

/********************************************************************
 Send a timeout message to each sleeping thread that is overdue.
 *******************************************************************/

int check_timeouts( void )
{
    Ct_thread * pThread = sleepers.pNext;
    Ct_time t = ticker();
    int found = FALSE;
    Ct_handle handle;

    while( CT_STATUS_TIMEOUT == pThread->status
            && ct_timecmp( &t, &pThread->deadline )> 0 )
    {
        found = TRUE;
        handle.p = pThread;
        handle.incarnation = pThread->incarnation;
        ct_send_msg( CT_TIMEOUT_MSGTYPE, NULL, 0, handle );

        pThread = pThread->pNext;
    }

    return found;
}

/********************************************************************
 Default clock for timeouts.

 NOTE: This implementation assumes that a clock_t is an integral type.
 *******************************************************************/

Ct_time CTScheduler::default_clock( void )
{
    static Ct_time t = {0, 0};
    static clock_t old_time = 0;
    clock_t new_time;
    clock_t diff;

    /* Make sure that clock_t is an integral type.  We */
    /* don't use an ASSERT in order to avoid compiler  */
    /* warnings about unreachable code.  Hence the     */
    /* problem will not be reported until run time.    */

    /* We could have reported an error at compile time */
    /* by using the #error directive.  However then    */
    /* you might not be able to compile this function  */
    /* even if you were never going to call it. */

#if (clock_t) 0.5 > 0
    CTOut::ct_report_error( "clock_t is not an integral type" );
    ct_fatal_error();
#endif

    /* Determine how many ticks have ticked since */
    /* the last time we looked at the clock */

    new_time = clock();
    if( new_time> old_time )
    {
        diff = new_time - old_time;
        old_time = new_time;
    }
    else if( new_time < old_time )
    {
        /* clock has rolled over */

        diff = CT_CLOCK_MAX - old_time;
        diff += new_time - CT_CLOCK_MIN + 1;
        old_time = new_time;
    }
    else
    return t; /* No change */

    /* Add the difference */

    if( ULONG_MAX - t.tick < diff )
    ++t.era;
    t.tick += diff;

    return t;
}

/**********************************************************
 Install a clock to replace the default clock.  Return a
 pointer to the previous clock.  A NULL parameter restores
 the default clock.
 *********************************************************/

Ct_clock CTScheduler::ct_install_clock( Ct_clock clock_function )
{
    Ct_clock prev_clock = ticker;

    if( CT_STATUS_TIMEOUT == sleepers.pNext->status )
    {
        ct_report_error( "ct_install_clock: Cannot replace clock "
                "when a timeout is already pending" );
        ct_fatal_error();
    }
    else
    {
        if( NULL == clock_function )
        ticker = default_clock;
        else
        ticker = clock_function;
    }

    return prev_clock;
}
#endif

/**********************************************************
 Compare two Ct_times; analogous to strcmp().

 We don't make this function subject to conditional
 compilation because it might be useful for some kind of
 timing mechanism even when timeouts are not enabled.
 *********************************************************/

int CTScheduler::ct_timecmp(const Ct_time * t1, const Ct_time * t2) {
    if (t1->era == t2->era) {
        if (t1->tick > t2->tick)
            return 1;
        else
            if (t1->tick < t2->tick)
                return -1;
            else
                return 0;
    }
    else
        if (t1->era > t2->era)
            return 1;
        else
            return -1;
}

/********************************************************************
 Collapse the priority queue into fewer thread lists.  Append queue
 1 to queue 0; then move queues 2-15 to queues 1-14, leaving queue
 15 empty.

 Periodically scrunching the queue guarantees that every active thread
 will execute eventually.
 *******************************************************************/

void CTScheduler::scrunch_queue(void) {
    int i;

    for (i = 1; i <= CT_PRIORITY_MAX; ++i)
        append_queue(i, i - 1);
}

/********************************************************************
 Transfer one thread list to the tail of another.
 *******************************************************************/

void CTScheduler::append_queue(int from, int to) {
    if (pri_q[ from ].pNext == pri_q + from)

        /* Nothing to transfer -- bail out */

        return;

    if (pri_q[ to ].pPrev == pri_q + to) {
        /* Receiving list is empty */

        pri_q[ to ].pNext = pri_q[ from ].pNext;
        pri_q[ to ].pPrev = pri_q[ from ].pPrev;

        pri_q[ to ].pPrev->pNext = pri_q + to;
        pri_q[ to ].pNext->pPrev = pri_q + to;
    }
    else {
        /* Append one non-empty list to another */

        pri_q[ to ].pPrev->pNext = pri_q[ from ].pNext;
        pri_q[ from ].pNext->pPrev = pri_q[ to ].pPrev;

        pri_q[ from ].pPrev->pNext = pri_q + to;
        pri_q[ to ].pPrev = pri_q[ from ].pPrev;
    }

    pri_q[ from ].pNext = pri_q[ from ].pPrev = pri_q + from;
    return;
}

/*******************************************************************
 Create a new cheap thread and add it to the priority queue as an
 active process.  Return a handle if a pointer to one is supplied.
 *******************************************************************/

int CTScheduler::ct_create_thread(Ct_handle * pHandle, int priority,
        void * pData, Ct_step_function step, Ct_destructor destruct) {
    int rc;
    Ct_thread * pThread;

    /* Initialize the queue and the sleeper */
    /* list, if we haven't already done so. */

    if ( !opened) {
        ct_open();
        opened = 1;
    }

    if (priority > CT_PRIORITY_MAX)
        priority = CT_PRIORITY_MAX;
    else
        if (priority < 0)
            priority = 0;

    pThread = ctDataStore.ct_construct(priority, pData, step, destruct);
    if (NULL == pThread)
        rc = CT_ERROR;
    else {
        /* Insert the thread into the priority queue.  If the  */
        /* attempt fails, discard the thread.  We here rely on */
        /* destruct_ct() to set pThread to NULL. */

        ASSERT( CT_MAGIC == pThread->magic );

        rc = insert_thread(pThread);
        if ( CT_OKAY != rc)
            ctDataStore.ct_destruct( &pThread);

        if (pHandle != NULL) {
            /* Provide a handle to the new thread */

            pHandle->p = pThread;
            if (NULL == pThread)
                pHandle->incarnation = 0;
            else
                pHandle->incarnation = pThread->incarnation;
        }
    }

    return rc;
}

/*******************************************************************
 Create a new cheap thread and add it to the sleeper list.  Return
 a handle if a pointer to one is supplied.
 *******************************************************************/

int CTScheduler::ct_create_sleeping_thread(Ct_handle * pHandle, int priority,
        void * pData, Ct_step_function step, Ct_destructor destruct) {
    int rc;
    Ct_thread * pThread;

    /* Initialize the queue and the sleeper */
    /* list, if we haven't already done so. */

    if ( !opened) {
        ct_open();
        opened = 1;
    }

    if (priority > CT_PRIORITY_MAX)
        priority = CT_PRIORITY_MAX;
    else
        if (priority < 0)
            priority = 0;

    pThread = ctDataStore.ct_construct(priority, pData, step, destruct);
    if (NULL == pThread)
        rc = CT_ERROR;
    else {
        ASSERT( CT_MAGIC == pThread->magic );
        pThread->status = CT_STATUS_ASLEEP;

        /* Add the thread to the tail of the sleeper list.  */

        pThread->pPrev = sleepers.pPrev;
        pThread->pNext = &sleepers;
        sleepers.pPrev->pNext = pThread;
        sleepers.pPrev = pThread;

        if (pHandle != NULL) {
            /* Provide a handle to the new thread */

            pHandle->p = pThread;
            pHandle->incarnation = pThread->incarnation;
        }

        rc = CT_OKAY;
    }

    return rc;
}

/******************************************************************
 Destruct all thread lists in the priority queue.  Before
 destructing each non-empty list, we convert it from a circular
 list to a linear list.
 *****************************************************************/

void CTScheduler::clear_priority_queue(void) {
    int i;

    for (i = CT_PRIORITY_MAX; i >= 0; --i) {
        if (pri_q[ i ].pNext != NULL) {
            pri_q[ i ].pPrev->pNext = NULL;
            pri_q[ i ].pPrev = pri_q + i;

            destruct_thread_list( &pri_q[ i ].pNext);
            pri_q[ i ].pNext = pri_q + i;
        }
    }
}

/******************************************************************
 Destruct and discard a NULL-terminated linked list of cheap
 threads.  We use only the forward pointers, ignoring the backwards
 pointers.
 *****************************************************************/

void CTScheduler::destruct_thread_list(Ct_thread ** ppFirst) {
    Ct_thread * pThread;
    Ct_thread * pTemp;

    if (ppFirst != NULL && *ppFirst != NULL) {
        pThread = *ppFirst;
        *ppFirst = NULL;

        while (pThread != NULL) {
            ASSERT( CT_MAGIC == pThread->magic );

            pTemp = pThread->pNext;
            ctDataStore.ct_destruct( &pThread);
            pThread = pTemp;
        }
    }
}

/***************************************************************
 If the scheduler is not running, clean up all internal data
 structures.  This function enables the client code to back off
 any installed threads if it decides not to run the scheduler
 after all, due for example to an error condition.  It also
 enables the client code to clear an error condition.
 **************************************************************/

void CTScheduler::ct_clear(void) {
    if (curr_priority < 0) {
        clean_up_all();
        fatal_error = 0;
    }
}

/***************************************************************
 Clear out the priority queue, destruct all threads, and
 restore the initial state of the module (except that it does
 not restore the default error reporting function).
 **************************************************************/

void CTScheduler::clean_up_all(void) {
    /* Discard any remaining threads on the queues.  */

    clear_priority_queue();

    /* Likewise discard any sleeping threads.  If any are left, */
    /* either we're aborting from a fatal error, or threads are */
    /* waiting on events that will never occur (because only an */
    /* active thread can send an event). */

    sleepers.pPrev->pNext = NULL;
    sleepers.pPrev = &sleepers;
    destruct_thread_list( &sleepers.pNext);
    sleepers.pNext = &sleepers;

    /* Now that we have logically discarded all threads, physically */
    /* deallocate all memory resources for threads, messages, etc.  */

    ctDataStore.ct_free_all_threads();
    ctDataStore.ct_destruct_event_list( &ev_head);
    ctDataStore.ct_free_all_msgnodes();
    ctDataStore.ct_free_all_events();
    ctDataStore.ct_free_subscriptions();

    /* Restore initial values of static variables -- except    */
    /* for fatal_error, which can be reset only by ct_clear(). */

    ev_tail = ev_head = NULL;
    pCurr_thread = NULL;
    curr_priority = -1;
    pri_penalty = 0;
    opened = 0;
    halted = 0;
    self_msg = 0;
#if defined CT_TIMEOUT
    ticker = default_clock;
#endif
}

/***************************************************************
 Install a callback function to be invoked before each thread
 step (or disable the callback if the argument is NULL).  Return
 a pointer to the previous pre-function, or NULL if there was none.
 **************************************************************/

Ct_user_exit CTScheduler::ct_install_pre_function(Ct_user_exit f) {
    Ct_user_exit prev_exit = pre_function;

    pre_function = f;
    return prev_exit;
}

/***************************************************************
 Install a callback function to be invoked after each thread
 step (or disable the callback if the argument is NULL).  Return
 a pointer to the previous post-function, or NULL if there was none.
 **************************************************************/

Ct_user_exit CTScheduler::ct_install_post_function(Ct_user_exit f) {
    Ct_user_exit prev_exit = post_function;

    post_function = f;
    return prev_exit;
}

/***************************************************************
 Set the initial countdown value.  The lower this number, the
 more often we scrunch the queue, and the more the queue acts
 like a simple round-robin scheduler.  With higher countdown
 values the priority settings become more dominant, because the
 lower-priority threads take longer to reach the head of the
 queue.

 We return the previous value of the countdown so that the
 application can restore it later.
 **************************************************************/

unsigned CTScheduler::ct_set_countdown(unsigned n) {
    unsigned prev_countdown;

    prev_countdown = init_countdown;

    if ( 0 == n)
        init_countdown = 1;
    else
        init_countdown = n;

    return prev_countdown;
}

/***************************************************************
 Impose a penalty on the current thread.  Next time we schedule
 the thread we'll temporarily give it a lower priority.
 **************************************************************/

void CTScheduler::ct_penalize(unsigned penalty) {
    if (NULL == pCurr_thread) {
        CTOut::ct_report_error("ct_penalize: no thread is active");
        ct_fatal_error();
    }
    else {
        ASSERT( CT_MAGIC == pCurr_thread->magic );

        /* Here we are careful to avoid numeric    */
        /* overflow, however unlikely that may be. */

        if (penalty > CT_PRIORITY_MAX)
            pri_penalty = CT_PRIORITY_MAX;
        else {
            pri_penalty += penalty;
            if (pri_penalty > CT_PRIORITY_MAX)
                pri_penalty = CT_PRIORITY_MAX;
        }
    }
}

/***************************************************************
 Note a fatal error.
 **************************************************************/

void CTScheduler::ct_fatal_error(void) {
    if (curr_priority >= 0) {
        fatal_error = 1;
        ct_halt();
    }
}

/***************************************************************
 Halt the scheduler.
 **************************************************************/

void CTScheduler::ct_halt(void) {
    if (curr_priority >= 0)
        halted = 1;
}

/***************************************************************
 Mark current thread for extinction.
 **************************************************************/

int CTScheduler::ct_exit(void) {
    if (NULL == pCurr_thread) {
        CTOut::ct_report_error("ct_exit: no thread is active");
        ct_fatal_error();
        return CT_ERROR;
    }
    else {
        ASSERT( CT_MAGIC == pCurr_thread->magic );

        pCurr_thread->status = CT_STATUS_DEFUNCT;
        return CT_OKAY;
    }
}

/***************************************************************
 Mark current thread to be moved to the sleeper list to wait
 for an event.
 **************************************************************/

int CTScheduler::ct_wait(void) {
    if (NULL == pCurr_thread) {
        CTOut::ct_report_error("ct_wait: no thread is active");
        ct_fatal_error();
        return CT_ERROR;
    }
    else {
        ASSERT( CT_MAGIC == pCurr_thread->magic );

        pCurr_thread->status = CT_STATUS_ASLEEP;
        return CT_OKAY;
    }
}

/*************************************************************
 Return a handle to the current thread, if any.
 ************************************************************/

Ct_handle CTScheduler::ct_self(void) {
    Ct_handle handle;

    if (NULL == pCurr_thread) {
        handle.p = NULL;
        handle.incarnation = 0;
    }
    else {
        ASSERT( CT_MAGIC == pCurr_thread->magic );

        handle.p = (void *) pCurr_thread;
        handle.incarnation = pCurr_thread->incarnation;
    }

    return handle;
}

/*************************************************************
 Return a pointer to the thread's data, as installed when it
 was created.
 ************************************************************/

void * CTScheduler::ct_self_data(void) {
    if (NULL == pCurr_thread)
        return NULL;
    else {
        ASSERT( CT_MAGIC == pCurr_thread->magic );
        return pCurr_thread->pData;
    }
}

/****************************************************************
 Enqueue a Ct_event for eventual dispatching.
 ***************************************************************/

int CTScheduler::ct_enqueue_event(Ct_event * pE) {
    if (NULL == pE) {
        CTOut::ct_report_error("ct_enqueue_event: No event supplied");
        ct_fatal_error();
        return CT_ERROR;
    }

    ASSERT( EVENT_MAGIC == pE->magic );

    if (NULL == ev_head)
        ev_head = ev_tail = pE;
    else {
        ASSERT( EVENT_MAGIC == ev_head->magic );
        ASSERT( ev_tail != NULL );
        ASSERT( EVENT_MAGIC == ev_tail->magic );
        ev_tail->pNext = pE;
        ev_tail = pE;
    }

    pE->pNext = NULL;

    return CT_OKAY;
}

/****************************************************************
 Dispatch all the pending events.
 ***************************************************************/

void CTScheduler::dispatch_event_queue(void) {
    Ct_event * pE;

    while (ev_head != NULL) {
        /* Detach event from queue */

        pE = ev_head;
        ASSERT( EVENT_MAGIC == pE->magic );
        ev_head = ev_head->pNext;
        pE->pNext = NULL;

        /* Dispatch event according to type */

        switch (pE->dispatch_type) {
            case CT_DISPATCH_ADDRESSEE:
                dispatch_addressee(pE);
                break;
            case CT_DISPATCH_SUBSCRIBER:
                ct_dispatch_subscription(pE);
                break;
            case CT_DISPATCH_ALL:
                dispatch_all(pE);
                break;
            default:
                CTOut::ct_report_error("dispatch_queue: Illegal event type");
                ct_fatal_error();
                break;
        }

        if (CT_EV_ENQ == pE->ev_type) {
            /* For an enqueue we don't attach the event to any */
            /* threads, so we can't rely on the threads to     */
            /* destruct it.  Destruct it now. */

            ctDataStore.ct_destruct_event( &pE);
        }
    }

    ev_tail = NULL;

    /* If the current thread sent an event to itself,  we */
    /* didn't enqueue it yet.  Now that we have enqueued  */
    /* all other recipients, enqueue the current thread.  */

    if (self_msg) {
        int saved_priority;

        ASSERT( pCurr_thread != NULL );
        ASSERT( CT_MAGIC == pCurr_thread->magic );

        /* Detach the thread from whatever queue it's on */

        pCurr_thread->pPrev->pNext = pCurr_thread->pNext;
        pCurr_thread->pNext->pPrev = pCurr_thread->pPrev;

        /* Enqueue it at the highest priority */

        saved_priority = pCurr_thread->priority;
        pCurr_thread->priority = 0;
        insert_thread(pCurr_thread);
        pCurr_thread->priority = saved_priority;

        self_msg = 0;
    }
}

/****************************************************************
 Dispatch an event to every thread, then destroy the event.
 ***************************************************************/

void CTScheduler::dispatch_all(Ct_event * pE) {
     
    int i;

    if (pE != NULL)
        ASSERT( EVENT_MAGIC == pE->magic );

    /* Broadcast to active threads on each queue */

    for (i = 0; i <= CT_PRIORITY_MAX; ++i) {
        if (broadcast_to_queue(pE, pri_q + i) != CT_OKAY) {
            ct_fatal_error();
            return;
        }
    }

    /* Broadcast to sleeping threads, if any */

    if (broadcast_to_queue(pE, &sleepers) != CT_OKAY) {
        ct_fatal_error();
        return;
    }
}

/****************************************************************
 Deliver an event to each thread in a specified queue.
 ***************************************************************/

int CTScheduler::broadcast_to_queue(Ct_event * pE, Ct_thread * pT) {
    Ct_thread * pNext;

    ASSERT( CT_STATUS_DUMMY == pT->status );

    /* pT points to a dummy thread serving as anchor for a     */
    /* circular list.  Skip it and start with the next thread. */

    pNext = pT->pNext;

    for (;;) {
        pT = pNext;
        pNext = pT->pNext;

        if (CT_STATUS_DUMMY == pT->status)
            break;
        else {
            if (this.ct_deliver_event(pE, pT) != CT_OKAY)
                return CT_ERROR;
        }
    } /* end for */

    return CT_OKAY;
}

/******************************************************************
 Deliver an event to a specified thread
 *****************************************************************/

int CTScheduler::ct_deliver_event(Ct_event * pE, Ct_thread * pT) {
    ASSERT( pE != NULL );
    ASSERT( EVENT_MAGIC == pE->magic );
    ASSERT( pT != NULL );
    ASSERT( CT_MAGIC == pT->magic );

    if (CT_EV_MSG == pE->ev_type) {
        Ct_msgnode * pM = ctDataStore.ct_alloc_msgnode();
        if (NULL == pM)
            return CT_ERROR;
        else {
            /* Populate the newly-allocated message */
            /* node, giving it a copy of the data.  */

            pM->pNext = NULL;
#ifndef NDEBUG
            pM->magic = MSGNODE_MAGIC;
#endif
            pM->pE = pE;
            ++pE->refcount;
            attach_msg(pM, pT);
        }
    }

    if (pT == pCurr_thread)

        /* Don't enqueue the originator yet -- wait until */
        /* all other affected threads have been enqueued. */

        self_msg = 1;
    else
        enqueue(pT);

    return CT_OKAY;
}

/****************************************************************
 Attach a message to a specified thread.

 This operation could be faster if each thread stored a pointer to
 the tail of its queue, especially if the queue is long, but at the
 cost of extra memory even for threads (or entire applications)
 that don't even use messages.  This implementation chooses to
 assume that message queues will generally be short or empty.
 ***************************************************************/

void CTScheduler::attach_msg(Ct_msgnode * pM, Ct_thread * pT) {
    ASSERT( pT != NULL );
    ASSERT( CT_MAGIC == pT->magic );
    ASSERT( pM != NULL );
    ASSERT( MSGNODE_MAGIC == pM->magic );

    if (NULL == pT->msg_q)
        pT->msg_q = pM;
    else {
        /* First find the tail of the queue,  */
        /* then append the new message to it. */

        Ct_msgnode * pTail;

        for (pTail = pT->msg_q; pTail->pNext != NULL; pTail = pTail->pNext)
            ASSERT( MSGNODE_MAGIC == pTail->magic );

        pTail->pNext = pM;
    }
}

/****************************************************************
 Enqueue a specified thread for execution at the highest priority.
 ***************************************************************/

void CTScheduler::enqueue(Ct_thread * pT) {
    int saved_priority;

    ASSERT( pT != NULL );
    ASSERT( CT_MAGIC == pT->magic );

    /* Remove the thread from whatever list it's on -- unless   */
    /* it's already in an awakened state, meaning that it has   */
    /* received an event and hasn't had a chance to respond to  */
    /* it.  This exception prevents a succession of events from */
    /* repeatedly moving a thread to the end of the list. */

    if (pT->status != CT_STATUS_AWAKENED) {
        pT->pNext->pPrev = pT->pPrev;
        pT->pPrev->pNext = pT->pNext;
        pT->pPrev = pT->pNext = NULL;
    }

    if (pT == pCurr_thread) {
        /* The current thread is sending the event to itself.   */
        /* Defer the enqueuing of it until we're sure that all  */
        /* other recipients have been enqueued first. */

        self_msg = 1;
        pT->status = CT_STATUS_AWAKENED;
    }
    else
        if (CT_STATUS_AWAKENED == pT->status)

            /* As noted above: in this case the thread is already */
            /* queued at the highest priority, so we leave it     */
            /* where it is. */

            ;
        else {
            /* Enqueue the thread at the highest priority. */

            saved_priority = pT->priority;
            pT->priority = 0;
            insert_thread(pT);
            pT->priority = saved_priority;
            pT->status = CT_STATUS_AWAKENED;
        }
}

/****************************************************************
 Dispatch an event to a single thread.
 ***************************************************************/
void CTScheduler::dispatch_addressee(Ct_event * pE) {
    Ct_thread * pT;

    ASSERT( pE != NULL );
    ASSERT( EVENT_MAGIC == pE->magic );

    /* Validate addressee */

    if ( ! ctDataStore.ct_valid_handle( &pE->addressee) ) {
        /* Invalid addressee; silently ignore event */

        return;
    }

    pT = (Ct_thread *) pE->addressee.p;

    ct_deliver_event(pE, pT);
}

/****************************************************************
 Initialize the priority queue and the sleeper queue, mostly by
 having each dummy thread point to itself in both directions.
 ***************************************************************/

void CTScheduler::ct_open(void) {
    int i;

    for (i = 0; i <= CT_PRIORITY_MAX; ++i) {
        pri_q[ i ].pNext = pri_q[ i ].pPrev = pri_q + i;
        pri_q[ i ].status = CT_STATUS_DUMMY;
        pri_q[ i ].priority = 9999;
        pri_q[ i ].incarnation = 0;
        pri_q[ i ].pData = NULL;
        pri_q[ i ].msg_q = NULL;
        pri_q[ i ].step = NULL;
        pri_q[ i ].destruct = NULL;
#ifndef NDEBUG
        pri_q[ i ].magic = CT_MAGIC;
#endif
    }

    sleepers.pNext = &sleepers;
    sleepers.pPrev = &sleepers;
    sleepers.status = CT_STATUS_DUMMY;
    sleepers.priority = 9999;
    sleepers.incarnation = 0;
    sleepers.pData = NULL;
    sleepers.msg_q = NULL;
    sleepers.step = NULL;
    sleepers.destruct = NULL;
#ifndef NDEBUG
    sleepers.magic = CT_MAGIC;
#endif
}

#ifdef CT_RETURN
/****************************************************************
 Use longjmp() to wrest control of the current thread back into
 the scheduler
 ***************************************************************/

void CTScheduler::ct_return( int rc )
{
    ASSERT( pCurr_thread );

    if( pCurr_thread )
    {
        jump_rc = rc;
        longjmp( jumper, 1 );
    }
    else
    {
        CTOut::ct_report_error( "ct_thread: no thread is currently active" );
        ct_fatal_error();
        return;
    }
}
#endif
