#ifndef CTASSERT_H_
#define CTASSERT_H_

#include <stdlib.h>
#include <string.h>
#include "ct.h"

#include "ctutil.h"

#include "CTOut.h"

#if ! defined ASSERT
#define ASSERT(f)    \
if( f )      \
;        \
else         \
CTAssert::ct_assert( __FILE__, __LINE__ )
#endif


/**********************************************************************
 * 
 *  ct_assert() is the basis for the Cheap Threads ASSERT macro, which
 *  is a variant of the standard assert macro.  It is specialized to
 *  issue the error message through a call to ct_report_error()
 *  rather than through writing directly to stderr.  Thus the assertion
 *  failures may be reported even in environments where stderr is
 *  inconvenient or unavailable.
 *
 *  Most of the logic is devoted to the construction of an error message.
 *  This tedious operation would have been a lot easier with fprintf() or
 *  sprintf().  However the former requires a file pointer, and the latter
 *  risks buffer overflow.  Both functions may be unavailable for an
 *  embedded application.
 * 
 ***********************************************************************/

#define MAX_ASSERT_MSG_LENGTH 200

class CTAssert {

    public:

        CTAssert();
        virtual ~CTAssert();

        /**********************************************************************
         * 
         *  ct_assert() is the basis for the Cheap Threads ASSERT macro, which
         *  is a variant of the standard assert macro.  It is specialized to
         *  issue the error message through a call to ct_report_error()
         *  rather than through writing directly to stderr.  Thus the assertion
         *  failures may be reported even in environments where stderr is
         *  inconvenient or unavailable.
         *
         *  Most of the logic is devoted to the construction of an error message.
         *  This tedious operation would have been a lot easier with fprintf() or
         *  sprintf().  However the former requires a file pointer, and the latter
         *  risks buffer overflow.  Both functions may be unavailable for an
         *  embedded application.
         * 
         ***********************************************************************/

         static void ct_assert(const char * sourceFile, unsigned sourceLine);


         /******************************************************/
         /*                                                    */
         /* Format a message into a supplied buffer            */
         /*                                                    */
         /******************************************************/

         static void build_msg(char * buf, const char * sourceFile,
                 unsigned sourceLine);
        
        /******************************************************/
        /*                                                    */
        /* Format a character string to represent an unsigned */
        /* int.  This function is comparable to the common    */
        /* (but non-standard ) function itoa().               */
        /*                                                    */
        /******************************************************/

        static void format_num(char * buf, unsigned n);
        
};

#endif /*CTASSERT_H_*/
