/* $Id$
 * EHS is a library for embedding HTTP(S) support into a C++ application
 * Copyright (C) 2004 Zachary J. Hansen
 *
 *    This library is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License version 2.1 as published by the Free Software Foundation;
 *
 *    This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with this library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    This can be found in the 'COPYING' file.
 *
 */

#ifndef _DEBUG_H
#define _DEBUG_H

#ifdef EHS_DEBUG
# include <iostream>
#endif

#define EHS_ASSERT assert

#ifdef EHS_DEBUG
# define EHS_VERIFY(test) EHS_ASSERT(test)
#else
# define EHS_VERIFY(test)	test
#endif

inline void EHS_TRACE( const char* szFormat ... )
{
#ifdef EHS_DEBUG
    const int bufsize = 100000; 
    char buf [ bufsize ] ; 
    va_list VarList;
    va_start( VarList, szFormat );
    vsnprintf(buf, bufsize - 1, szFormat, VarList ) ;
    va_end ( VarList ) ;
# ifdef _WIN32
    OutputDebugStringA(buf) ;
# else
    std::cerr << buf << std::endl; std::cerr.flush();
# endif
#endif
}

#define _STR(x) #x
#define EHS_TODO			(_message) message ("  *** TODO: " ##_message "\t\t\t\t" __FILE__ ":" _STR(__LINE__) )	
#define EHS_FUTURE		(_message) message ("  *** FUTURE: " ##_message "\t\t\t\t" __FILE__ ":" _STR(__LINE__) )	
#define EHS_TODOCUMENT	(_message) message ("  *** TODOCUMENT: " ##_message "\t\t\t\t" __FILE__ ":" _STR(__LINE__) )	
#define EHS_DEBUGCODE	(_message) message ("  *** DEBUG CODE (REMOVE!): " ##_message "\t\t\t\t" __FILE__ ":" _STR(__LINE__) )	

#endif // _DEBUG_H
