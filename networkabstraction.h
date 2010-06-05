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

class PrivilegedBindHelper;

/**
 * Abstracts different socket types.
 * This interface abstracts the differences between normal
 * sockets and SSL sockets. There are two implementations:
 * <ul>
 *  <li>Socket is the standard socket<br>
 *  <li>SecureSocket is the SSL implementation<br>
 * </ul>
 */
class NetworkAbstraction {

    public:

        /**
         * Registers a PrivilegedBindHelper for use by this instance.
         * @param helper The PrivilegedBindHelper to be used by this instance.
         */
        virtual void RegisterBindHelper(PrivilegedBindHelper *helper) = 0;

        /**
         * Sets the bind address of the socket.
         * @param bindAddress The address to bind to in quad-dotted format.
         */
        virtual void SetBindAddress(const char * bindAddress) = 0;

        /**
         * Retrieves the peer address.
         * @return The address of the connected peer in quad-dotted format.
         */
        virtual std::string GetAddress() const = 0;

        /**
         * Retrieves the peer's port of a connection.
         * @return The peer port.
         */
        virtual int GetPort() const = 0;

        /// Enumeration of error results for InitSocketsResults
        enum InitResult {
            INITSOCKET_INVALID,
            INITSOCKET_SOCKETFAILED,
            INITSOCKET_BINDFAILED,
            INITSOCKET_LISTENFAILED,
            INITSOCKET_FAILED,
            INITSOCKET_CERTIFICATE,
            INITSOCKET_SUCCESS
        };

        /**
         * Initializes a listening socket.
         * If listening should be restricted to a specific address, SetBindAddress
         * has to be called in advance.
         * @param port The port to listen on.
         * @return An InitResult, describing the result of the initialization.
         */
        virtual InitResult Init(int port) = 0;

        /// Destructor
        virtual ~NetworkAbstraction() { }

        /**
         * Retrieves the underlying file descriptor.
         * @return The FD/Socket of the listening socket.
         */
        virtual int GetFd() const = 0;

        /**
         * Performs a read from the underlying socket.
         * @param ipBuffer Pointer to a buffer that receives the incoming data.
         * @param ipBufferLength The maximum number of bytes to read.
         * @return The actual number of bytes that have been received or -1 if an error occured.
         */
        virtual int Read(void * ipBuffer, int ipBufferLength) = 0;

        /**
         * Performs a send on the underlying socket.
         * @param ipMessage Pointer to the data to be sent.
         * @param inLength The number of bytes to send.
         * @param inFlags Additional flags for the system call.
         * @return The actual number of byte that have been sent or -1 if an error occured.
         */
        virtual int Send(const void * ipMessage, size_t inLength, int inFlags = 0) = 0;

        /// Closes the underlying socket.
        virtual void Close() = 0;

        /// Waits for an incoming connection.
        /// @return A new NetworkAbstraction instance which represents the client connetion.
        virtual NetworkAbstraction * Accept() = 0;

        /// Determines, whether the underlying socket is socure.
        /// @return true, if SSL is used; false otherwise.
        virtual bool IsSecure() const = 0;

};

#endif // NETWORK_ABSTRACTION_H
