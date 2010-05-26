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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/*
 * Solaris 2.6 troubles
 */

#ifdef sun
#ifndef BSD_COMP 
#define BSD_COMP 1
#endif
#ifndef _XPG4_2
#define _XPG4_2 1
#endif
#ifndef __EXTENSIONS__
#define __EXTENSIONS__ 1
#endif
#ifndef __PRAGMA_REDEFINE_EXTNAME
#define __PRAGMA_REDEFINE_EXTNAME 1
#endif
#endif // sun

#include "socket.h"

#ifndef _WIN32
#include <assert.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0 // no support
#endif // MSG_NOSIGNAL

#include <iostream>
#include <sstream>
#include <cstring>

using namespace std;

    Socket::Socket ( )
  : m_nAcceptSocket ( 0 ),
    m_poBindHelper ( NULL )
{
    memset ( &m_oInternetSocketAddress, 0, sizeof ( m_oInternetSocketAddress ) );
    memset ( &m_oBindAddress, 0, sizeof ( m_oBindAddress ) );
}

Socket::Socket ( int inAcceptSocket,
        sockaddr_in * ipoInternetSocketAddress ) :
    m_nAcceptSocket ( inAcceptSocket ),
    m_poBindHelper ( NULL )
{
    memcpy ( &m_oInternetSocketAddress, 
            ipoInternetSocketAddress,
            sizeof ( m_oInternetSocketAddress ) );

}

Socket::~Socket ( )
{
}

void Socket::RegisterBindHelper(PrivilegedBindHelper *helper)
{
    m_poBindHelper = helper;
}

void Socket::SetBindAddress ( const char *bindAddress ///< address on which to listen
        )
{
    in_addr_t baddr =
        (bindAddress && strlen(bindAddress)) ? inet_addr(bindAddress) : INADDR_ANY;
    memcpy ( & ( m_oBindAddress.sin_addr ), & baddr, sizeof ( m_oBindAddress.sin_addr ) );
}

NetworkAbstraction::InitResult
Socket::Init ( int inPort ///< port on which to listen
        ) 
{

#ifdef _WIN32

    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    wVersionRequested = MAKEWORD( 2, 2 );

    err = WSAStartup( wVersionRequested, &wsaData );

    // Found a usable winsock dll?
    assert( err == 0 );

    /* Confirm that the WinSock DLL supports 2.2.*/
    /* Note that if the DLL supports versions greater    */
    /* than 2.2 in addition to 2.2, it will still return */
    /* 2.2 in wVersion since that is the version we      */
    /* requested.                                        */

    if ( 
            LOBYTE( wsaData.wVersion ) != 2 
            ||	HIBYTE( wsaData.wVersion ) != 2
       ) {

        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */
        WSACleanup( );

#ifdef EHS_DEBUG
        cerr << "[EHS_DEBUG] Critical Error: Couldn't find useable Winsock DLL.  Must be at least 2.2.  Aborting" << endl;
#endif

        assert( false ); 
    }

    /* The WinSock DLL is acceptable. Proceed. */

#endif // End WIN32-specific network initialization code

    // need to create a socket for listening for new connections
#ifdef EHS_DEBUG
    if ( m_nAcceptSocket != 0 ) {
        cerr << "[EHS_DEBUG] m_nAcceptSocket = " << m_nAcceptSocket << endl;
        assert ( m_nAcceptSocket == 0 );
    }
#endif

    m_nAcceptSocket = socket ( AF_INET, SOCK_STREAM, 0 );
    if ( m_nAcceptSocket == -1 ) {

#ifdef EHS_DEBUG
        cerr << "[EHS_DEBUG] Error: socket() failed" << endl;
#endif

        return INITSOCKET_SOCKETFAILED;
    }

#ifdef _WIN32

    u_long MyTrueVar = 1;
    ioctlsocket ( m_nAcceptSocket, FIONBIO, &MyTrueVar );

#else
    int MyTrueVar = 1;
    ioctl ( m_nAcceptSocket, FIONBIO, &MyTrueVar );
    MyTrueVar = 1; // not sure if it was changed in ioctl, so re-set it
    setsockopt ( m_nAcceptSocket, SOL_SOCKET, SO_REUSEADDR, (const void *) &MyTrueVar, sizeof ( int ) );
#endif

    // bind the socket to the appropriate port
    struct sockaddr_in oSocketInfo;
    memset ( &oSocketInfo, 0, sizeof ( oSocketInfo ) );
    int nResult = -1;

    oSocketInfo.sin_family = AF_INET;
    memcpy ( & ( oSocketInfo.sin_addr ), & m_oBindAddress.sin_addr, sizeof ( oSocketInfo.sin_addr ) );
    oSocketInfo.sin_port = htons ( inPort );

    if (( inPort < 1024 ) && m_poBindHelper ) {
        struct in_addr in;
        memcpy (&in, &m_oBindAddress.sin_addr, sizeof(in));
        string ba(inet_ntoa(in));
        nResult = m_poBindHelper->BindPrivilegedPort( m_nAcceptSocket, ba.c_str(), inPort ) ? 0 : -1;
    } else {
        nResult = bind ( m_nAcceptSocket, (sockaddr *)&oSocketInfo, sizeof ( sockaddr_in ) );
    }

    if ( nResult == -1 ) {
#ifdef EHS_DEBUG
        cerr << "[EHS_DEBUG] Error: bind() failed" << endl;
#endif
        return INITSOCKET_BINDFAILED;
    }

    // listen 
    nResult = listen ( m_nAcceptSocket, 20 );
    if ( nResult != 0 ) {
#ifdef EHS_DEBUG
        cerr << "[EHS_DEBUG] Error: listen() failed" << endl;
#endif
        return INITSOCKET_LISTENFAILED;
    }

    return INITSOCKET_SUCCESS;
}


int Socket::Read ( void * ipBuffer, int ipBufferLength )
{

    //return read ( m_nAcceptSocket, ipBuffer, ipBufferLength );
    return recv ( m_nAcceptSocket, 
#ifdef _WIN32
            (char *) ipBuffer,
#else
            ipBuffer, 
#endif
            ipBufferLength, 0 );
}

int Socket::Send ( const void * ipMessage, size_t inLength, int inFlags )
{
    return send ( m_nAcceptSocket, 
#ifdef _WIN32
            (const char *) ipMessage,
#else
            ipMessage, 
#endif	
            inLength, inFlags | MSG_NOSIGNAL );
}

void Socket::Close ( )
{
#ifdef _WIN32
    closesocket ( m_nAcceptSocket );
#else
    close ( m_nAcceptSocket );
#endif
}

NetworkAbstraction * Socket::Accept ( )
{
    size_t oInternetSocketAddressLength = sizeof ( m_oInternetSocketAddress );
    int nNewFd = accept ( m_nAcceptSocket, 
            (sockaddr *) &m_oInternetSocketAddress,
#ifdef _WIN32
            (int *) &oInternetSocketAddressLength 
#else
            &oInternetSocketAddressLength 
#endif
            );

#ifdef EHS_DEBUG
    cerr
        << "Got a connection from " << GetAddress() << ":"
        << ntohs(m_oInternetSocketAddress.sin_port) << endl;
#endif
    if ( nNewFd == -1 ) {
        return NULL;
    }
    Socket * poSocket = new Socket ( nNewFd, &m_oInternetSocketAddress );
#ifdef EHS_DEBUG
    cerr
        << "[EHS_DEBUG] Allocated new socket object with fd="
        << nNewFd << " and socket @" << hex << poSocket << endl;
#endif

    return poSocket;

}

string Socket::GetAddress ( )
{
    struct in_addr in;
    memcpy (&in, &m_oInternetSocketAddress.sin_addr.s_addr, sizeof(in));
    return string(inet_ntoa(in));
}


int Socket::GetPort ( )
{
    return ntohs ( m_oInternetSocketAddress.sin_port );
}
