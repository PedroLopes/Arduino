
/* myAssert -- called by ASSERT macro.  This code is mostly cribbed
 from: Writing Solid Code, by Steve Maguire, p. 18; Microsoft Press,
 1993, Redmond, WA.

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

#include "CTAssert.h"

CTAssert::CTAssert() {
}

CTAssert::~CTAssert() {
}

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

static void ct_assert(const char * sourceFile, unsigned sourceLine) {
    
    char msg[ MAX_ASSERT_MSG_LENGTH + 1 ];

    CTAssert::build_msg(msg, sourceFile, sourceLine);
    CTOut::ct_report_error(msg);
    abort();
}

/******************************************************/
/*                                                    */
/* Format a message into a supplied buffer            */
/*                                                    */
/******************************************************/

static void build_msg(char * buf, const char * sourceFile, unsigned sourceLine) {
    
    static char * p;
    static size_t len_to_add;
    static size_t len_left;
    static const char line_literal[] = ", line ";

    strcpy(buf, "Assertion failed: ");
    p = buf + strlen(buf);
    len_left = MAX_ASSERT_MSG_LENGTH - strlen( buf );

    /* Append name of source file */

    len_to_add = strlen(sourceFile);
    if (len_to_add > len_left)
        len_to_add = len_left;
    if (len_to_add > 0) {
        strncpy(p, sourceFile, len_to_add);
        p += len_to_add;
        *p = '\0';
        len_left -= len_to_add;
    }

    /* Append a literal */

    if (len_left > 0) {
        len_to_add = strlen(line_literal);
        if (len_to_add > len_left)
            len_to_add = len_left;
        if (len_to_add > 0) {
            strncpy(p, line_literal, len_to_add);
            p += len_to_add;
            *p = '\0';
            len_left -= len_to_add;
        }
    }

    if (len_left > 0) {
        char num_buf[ 11 ];

        CTAssert::format_num(num_buf, sourceLine);

        len_to_add = strlen(num_buf);
        if (len_to_add > len_left)
            len_to_add = len_left;
        if (len_to_add > 0) {
            strncpy(p, num_buf, len_to_add);
            p += len_to_add;
            *p = '\0';
            len_left -= len_to_add;
        }
    }
}

/******************************************************/
/*                                                    */
/* Format a character string to represent an unsigned */
/* int.  This function is comparable to the common    */
/* (but non-standard ) function itoa().               */
/*                                                    */
/******************************************************/

static void format_num(char * buf, unsigned n) {
    char * p = buf;
    char temp;

    /* Compute each digit in turn */

    while (n) {
        *p++ = '0' + (n % 10 );
        n /= 10;
    }
    *p = '\0';

    /* Reverse the order of digits */

    --p;
    while (p > buf) {
        temp = *p;
        *p-- = *buf;
        *buf++ = temp;
    }
}
