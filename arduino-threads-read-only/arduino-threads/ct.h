/*****************************************************************
 ct.h -- public header for cheap threads

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
#ifndef CT_H
#define CT_H

#include "ctutil.h"

//#define NULL 0

#define CT_OKAY 0
#define CT_ERROR 1

#define CT_TRUE 1
#define CT_FALSE 0

/* Reference to a Cheap Thread.  Client code should not   */
/* access the internal members of this struct, since they */
/* are subject to change. */

typedef struct {
        void * p;
        short incarnation;
} Ct_handle;

typedef unsigned Ct_msgtype;

typedef unsigned size_t;

typedef struct {
        Ct_msgtype type;
        void * pData;
} Ct_msg;

typedef struct {
        Ct_msgtype type;
        size_t length;
} Ct_msgheader;

#define CT_TIMEOUT_MSGTYPE ((Ct_msgtype) -1)

typedef struct /* For timers */
{
        unsigned long tick; /* ticks */
        unsigned long era; /* cycles of ULONG_MAX + 1 ticks */
} Ct_time;

int ct_timecmp(const Ct_time * t1, const Ct_time * t2);

/* The following three macros may have their values redefined */
/* by command-line compiler options, or by whatever other     */
/* means, without altering this header file: */

/* Priority is represented backwards: higher priorities have  */
/* lower priority numbers, ranging from 0 to CT_PRIORITY_MAX. */

#ifndef CT_PRIORITY_MAX
#define CT_PRIORITY_MAX 15
#endif

/* Number of threads to run between queue-scrunches: */

#ifndef CT_DEFAULT_COUNTDOWN
#define CT_DEFAULT_COUNTDOWN 8
#endif

/* Maximum data length carried by a message without */
/* additional memory allocation: */

#ifndef CT_MSG_BUF_LEN
#define CT_MSG_BUF_LEN  16
#endif

#if defined CT_TIMEOUT

/* Maximum value of a clock_t: */

#if ! defined CT_CLOCK_MAX
#define CT_CLOCK_MAX LONG_MAX
#endif

/* Number to which a clock_t rolls over */
/* when it exceeds its maximum: */

#if ! defined CT_CLOCK_MIN
#define CT_CLOCK_MIN 0
#endif

typedef Ct_time (* Ct_clock)( void );
Ct_clock ct_install_clock( Ct_clock clock_function );
int ct_wait_on_timeout( unsigned long interval );

#endif

/* typedefs for callback function pointers: */

typedef void ( * Ct_error_reporter)(const char * msg);
typedef int ( * Ct_step_function)(void *);
typedef void ( * Ct_destructor)(void *);
typedef int ( * Ct_user_exit)(void *);

#ifdef __cplusplus
extern "C"
    {
#endif
#ifdef __cplusplus
    }
#endif

void ct_return(int rc);

#endif
