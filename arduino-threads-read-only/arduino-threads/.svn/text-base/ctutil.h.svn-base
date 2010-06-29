/* ctutil.h -- header for miscellaneous useful functions and macros

 Copyright (C) 2001  Scott McKellar  mck9@swbell.net

 This program is open software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef CTUTIL_H
#define CTUTIL_H

#if ! defined TRUE
#define TRUE        (1)
#endif

#if ! defined FALSE
#define FALSE       (0)
#endif

#ifdef __cplusplus
	extern "C" {
#endif
    
#ifdef __cplusplus
	};
#endif

/* The following variant of assert() is mostly cribbed from: Writing Solid
   Code, by Steve Maguire, p. 17; Microsoft Press, 1993, Redmond, WA.
*/

#if ! defined NDEBUG

#ifdef __cplusplus
	extern "C"
#endif

#else /* NDEBUG is defined */

#define ASSERT(f)

#endif

#endif
