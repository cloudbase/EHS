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

#ifndef SOCKET_H
#define SOCKET_H

///////////////////////////////////
#ifdef _WIN32 // windows headers //
///////////////////////////////////

// Pragma'ing away nasty MS 255-char-name problem.  Otherwise
// you will get warnings on many template names that
//	"identifier was truncated to '255' characters in the debug information".
#pragma warning(disable : 4786)

// to use winsock2.h instead of winsock.h
#define _WIN32_WINNT 0x0400
#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include <assert.h>

// make stricmp sound like strcasecmp
#define strcasecmp stricmp

// make windows sleep act like UNIX sleep
#define sleep(seconds) (Sleep(seconds * 1000))

///////////////////////////////////
#else // unix headers go here    //
///////////////////////////////////

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>

///////////////////////////////////
#endif // end platform headers   //
///////////////////////////////////

#include "networkabstraction.h"

/// plain socket implementation of NetworkAbstraction
class Socket : public NetworkAbstraction {

    public:

        /// Registers a PrivilegedBindHelper for use by this instance.
        virtual void RegisterBindHelper(PrivilegedBindHelper *);

        /// sets up socket stuff (mostly for win32) and then listens on specified port
        virtual InitResult Init ( int inPort );

        /// default constructor
        Socket ( );

        /// client socket constructor
        Socket ( int inAcceptSocket, sockaddr_in * );

        /// destructor
        virtual ~Socket ( );

        /// Sets the bind address of the socket
        virtual void SetBindAddress ( const char * bindAddress );

        /// returns the FD associated with this socket
        virtual int GetFd ( ) { return m_nAcceptSocket; };

        /// implements standard FD read
        virtual int Read ( void * ipBuffer, int ipBufferLength );

        /// implements standard FD send
        virtual int Send ( const void * ipMessage, size_t inLength, int inFlags = 0 );

        /// implements standard FD close
        virtual void Close ( );

        /// implements standard FD accept
        virtual NetworkAbstraction * Accept ( );

        /// Returns false, plain sockets are not secure
        virtual int IsSecure ( ) { return 0; }

    protected:

        /// returns the port of the incoming connection
        int GetPort ( );

        /// returns the address of the incoming connection
        std::string GetAddress ( );

        /// Socket on which this connection came in
        int m_nAcceptSocket;

        /// stores the address of the current connection
        sockaddr_in m_oInternetSocketAddress;

        /// stores the bind address
        sockaddr_in m_oBindAddress;

        /// Our bind helper
        PrivilegedBindHelper *m_poBindHelper;

};

#endif // SOCKET_H
