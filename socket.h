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

    private:

        Socket(const Socket &);

        Socket & operator=(const Socket &);

    protected:
        /**
         * Constructs a new Socket, connected to a client.
         * @param fd The socket descriptor of this connection.
         * @param peer The peer address of this socket
         */
        Socket(int fd, sockaddr_in *peer);

    public:

        /**
         * Default constructor
         */
        Socket();

        virtual void RegisterBindHelper(PrivilegedBindHelper *helper);

        virtual void Init(int port);

        virtual ~Socket();

        virtual void SetBindAddress(const char * bindAddress);

        virtual int GetFd() const { return m_fd; }

        virtual int Read(void *buf, int bufsize);

        virtual int Send(const void *buf, size_t buflen, int flags = 0);

        virtual void Close();

        virtual NetworkAbstraction *Accept();

        /// Determines, whether the underlying socket is secure.
        /// @return false, because this instance does not use SSL.
        virtual bool IsSecure() const { return false; }

        virtual void ThreadCleanup() { }

    protected:

        int GetPort() const;

        std::string GetAddress() const;

        /// The file descriptor of the socket on which this connection came in
        int m_fd;

        /// Stores the peer address of the current connection
        sockaddr_in m_peer;

        /// Stores the bind address
        sockaddr_in m_bindaddr;

        /// Our bind helper
        PrivilegedBindHelper *m_pBindHelper;

};

#endif // SOCKET_H
