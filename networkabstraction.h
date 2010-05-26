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

#ifndef NETWORK_ABSTRACTION_H
#define NETWORK_ABSTRACTION_H

#include <string>
#include <cstdlib>

/**
 * This abstracts an interface to an external bind helper
 * program which facilitates binding of ports < 1024.
 * Instead of doing it the apache way (do the bind() running as root
 * and then dropping privileges) we run as unprivileged user and
 * use that helper (setuid program) to temporarily elevate privileges
 * for the bind() call. IMHO, this is safer, because that helper is
 * VERY simple and does exactly ONE task: binding. NOTHING else.
 */
class PrivilegedBindHelper {

    public:

        /// Binds a socket to a privileged port/address. Returns true on success
        virtual bool BindPrivilegedPort(int socket, const char *addr, const unsigned short port) = 0;

        virtual ~PrivilegedBindHelper ( ) { }

};

/// Abstracts different socket types
/**
 * this abstracts the differences between normal sockets and ssl sockets
 *   Socket is the standard socket class and SecureSocket is the SSL
 *   implementation
 */
class NetworkAbstraction {

    public:

        /// Registers a PrivilegedBindHelper for use by this instance.
        virtual void RegisterBindHelper(PrivilegedBindHelper *) = 0;

        /// sets the bind address of the socket
        virtual void SetBindAddress ( const char * bindAddress ) = 0;

        /// returns the address of the connection
        virtual std::string GetAddress ( ) = 0;

        /// returns the port of the connection
        virtual int GetPort ( ) = 0;

        /// Enumeration of error results for InitSocketsResults
        enum InitResult { INITSOCKET_INVALID,
            INITSOCKET_SOCKETFAILED,
            INITSOCKET_BINDFAILED,
            INITSOCKET_LISTENFAILED,
            INITSOCKET_FAILED,
            INITSOCKET_CERTIFICATE,
            INITSOCKET_SUCCESS };

        /// Initialize sockets
        virtual InitResult Init ( int inPort ) = 0;

        /// destructor
        virtual ~NetworkAbstraction ( ) {};

        /// returns the FD/Socket for the socket on which we're listening
        virtual int GetFd ( ) = 0;

        /// pure virtual read function to be overloaded in child classes
        virtual int Read ( void * ipBuffer, int ipBufferLength ) = 0;

        /// pure virtual send function to be overloaded in child class
        virtual int Send ( const void * ipMessage, size_t inLength, int inFlags = 0 ) = 0;

        /// pure virtual close function to be overloaded in child class
        virtual void Close ( ) = 0;

        /// pure virtual accept function to be overloaded in child class
        virtual NetworkAbstraction * Accept ( ) = 0;

        /// returns whether the child class connection is considered secure
        virtual int IsSecure ( ) = 0;

};

#endif // NETWORK_ABSTRACTION_H
