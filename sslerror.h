/* $Id$
 *
 * EHS is a library for embedding HTTP(S) support into a C++ application
 *
 * Copyright (C) 2004 Zachary J. Hansen
 *
 * Code cleanup, new features and bugfixes: Copyright (C) 2010 Fritz Elfert
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

#ifndef SSL_ERROR
#define SSL_ERROR

#ifdef COMPILE_WITH_SSL

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <string>

/// wrapper for the OpenSSL error mechanism
class SslError 
{

    public:

        /// gets info about the previous error and removes from SSL error queue returns 0 when no error available on queue
        int GetError ( std::string & irsReport, bool inPeek = false );

        /// gets info about the previous error and leaves it on SSL error queue returns 0 when no error available on queue
        int PeekError ( std::string & irsReport );

    protected:

        /// represents whether the error strings have been loaded
        static bool bMessagesLoaded;
};

#endif // COMPILE_WITH_SSL

#endif // SSL_ERROR

