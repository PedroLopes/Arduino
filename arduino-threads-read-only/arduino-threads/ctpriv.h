/*****************************************************************
 ctpriv.h -- private header for cheap threads

 Copyright (C) 2002  Scott McKellar  mck9@swbell.net

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

 ****************************************************************/
#ifndef CTPRIV_H
#define CTPRIV_H

typedef enum
{
    CT_STATUS_ACTIVE,
    CT_STATUS_AWAKENED,
    CT_STATUS_ASLEEP,
#if defined CT_TIMEOUT
    CT_STATUS_TIMEOUT,
#endif
    CT_STATUS_DEFUNCT,
    CT_STATUS_DUMMY
} CT_STATUS;

struct Ct_sub;
typedef struct Ct_sub Ct_sub;

typedef enum
{
    CT_DISPATCH_ADDRESSEE,
    CT_DISPATCH_SUBSCRIBER,
    CT_DISPATCH_ALL
} Ct_dispatch_type;

typedef struct Ct_thread Ct_thread;
typedef struct Ct_msgnode Ct_msgnode;
typedef struct Ct_event Ct_event;

#ifndef NDEBUG
#define MSGNODE_MAGIC 9485763L
#endif

enum Ct_event_type
{
    CT_EV_MSG,
    CT_EV_ENQ
};
typedef enum Ct_event_type Ct_event_type;

/* The following represents a pending event */
/* not yet dispatched to any thread:        */

struct Ct_event {
        Ct_event * pNext;
        Ct_msgtype type;
        Ct_event_type ev_type;
        size_t msg_len;
        unsigned refcount; /* reference count */
        void * pData; /* for long messages */
        unsigned char buff [CT_MSG_BUF_LEN ]; /* for short messages */
        Ct_dispatch_type dispatch_type;
        Ct_handle addressee;
#ifndef NDEBUG
        long magic;
#endif
};

#ifndef NDEBUG
#define EVENT_MAGIC 7846735L
#endif

/* The following structure represents a message assigned */
/* to a thread but not yet dequeued by that thread:      */

struct Ct_msgnode {
        Ct_msgnode * pNext;
        Ct_event * pE;
#ifndef NDEBUG
        long magic;
#endif
};

struct Ct_thread {
        Ct_thread * pNext;
        Ct_thread * pPrev;
        CT_STATUS status;
        int priority;
        unsigned short incarnation;
        void * pData;
        Ct_msgnode * msg_q;
        Ct_sub * subscriptions;
        Ct_step_function step;
        Ct_destructor destruct;
#if defined CT_TIMEOUT
        Ct_time deadline;
#endif
#ifndef NDEBUG
        long magic;
#endif
};

#ifndef NDEBUG
#define CT_MAGIC 3984756L
#endif

#endif
