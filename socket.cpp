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

#include "ehs.h"
#include "socket.h"

#ifndef _WIN32
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
#include <cerrno>
#include <stdexcept>

using namespace std;

    Socket::Socket()
  : m_fd(-1),
    m_peer(sockaddr_in()),
    m_bindaddr(sockaddr_in()),
    m_pBindHelper(NULL)
{
    memset(&m_peer, 0, sizeof(m_peer));
    memset(&m_bindaddr, 0, sizeof(m_bindaddr));
}

Socket::Socket(int fd, sockaddr_in * peer) :
    m_fd(fd),
    m_peer(*peer),
    m_bindaddr(sockaddr_in()),
    m_pBindHelper(NULL)
{
    memcpy(&m_peer, peer, sizeof(m_peer));
    memset(&m_bindaddr, 0, sizeof(m_bindaddr));
}

Socket::~Socket()
{
    Close();
}

void Socket::RegisterBindHelper(PrivilegedBindHelper *helper)
{
    m_pBindHelper = helper;
}

void Socket::SetBindAddress(const char *bindAddress)
{
    in_addr_t baddr =
        (bindAddress && strlen(bindAddress)) ? inet_addr(bindAddress) : INADDR_ANY;
    memcpy(&(m_bindaddr.sin_addr), &baddr, sizeof(m_bindaddr.sin_addr));
}

void Socket::Init(int port) 
{
    string sError;
#ifdef _WIN32
    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(2, 2);

    // Found a usable winsock dll?
    if (0 != WSAStartup(wVersionRequested, &wsaData)) {
        throw runtime_error("Socket::Init: WSAStartup() returned an error");
    }

    /* Confirm that the WinSock DLL supports 2.2.*/
    /* Note that if the DLL supports versions greater    */
    /* than 2.2 in addition to 2.2, it will still return */
    /* 2.2 in wVersion since that is the version we      */
    /* requested.                                        */

    if (2 != LOBYTE(wsaData.wVersion) || 2 != HIBYTE(wsaData.wVersion)) {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */
        WSACleanup();
        throw runtime_error("Socket::Init: Could not find a suitable (v 2.2) Winsock DLL");
    }

    /* The WinSock DLL is acceptable. Proceed. */

#endif // End WIN32-specific network initialization code

    // need to create a socket for listening for new connections
    if (m_fd != -1) {
#ifdef _WIN32
        WSACleanup();
#endif
        throw runtime_error("Socket::Init: Socket already initialized");
    }

    m_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == m_fd) {
#ifdef _WIN32
        WSACleanup();
#endif
        sError.assign("socket: ");
        throw runtime_error(sError.append(strerror(errno)));
    }

#ifdef _WIN32
    u_long one = 1;
    ioctlsocket(m_fd, FIONBIO, &one);
#else
    int one = 1;
    ioctl(m_fd, FIONBIO, &one);
    setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const void *>(&one), sizeof(int));
#endif

    // bind the socket to the appropriate port
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    int nResult = -1;

    sa.sin_family = AF_INET;
    memcpy(&(sa.sin_addr), &m_bindaddr.sin_addr, sizeof(sa.sin_addr));
    sa.sin_port = htons(port);

    if ((1024 > port) && m_pBindHelper) {
        struct in_addr in;
        memcpy(&in, &m_bindaddr.sin_addr, sizeof(in));
        string ba(inet_ntoa(in));
        nResult = m_pBindHelper->BindPrivilegedPort(m_fd, ba.c_str(), port) ? 0 : -1;
    } else {
        nResult = bind(m_fd, reinterpret_cast<sockaddr *>(&sa), sizeof(sa));
    }

    if (-1 == nResult) {
#ifdef _WIN32
        WSACleanup();
#endif
        sError.assign("bind: ");
        throw runtime_error(sError.append(strerror(errno)));
    }

    // listen 
    nResult = listen(m_fd, 20);
    if (0 != nResult) {
#ifdef _WIN32
        WSACleanup();
#endif
        sError.assign("listen: ");
        throw runtime_error(sError.append(strerror(errno)));
    }
}


int Socket::Read(void *buf, int bufsize)
{
    return recv(m_fd, 
#ifdef _WIN32
            reinterpret_cast<char *>(buf),
#else
            buf, 
#endif
            bufsize, 0);
}

int Socket::Send(const void *buf, size_t buflen, int flags)
{
    return send(m_fd, 
#ifdef _WIN32
            reinterpret_cast<const char *>(buf),
#else
            buf, 
#endif	
            buflen, flags|MSG_NOSIGNAL);
}

void Socket::Close()
{
    if (-1 == m_fd)
        return;
#ifdef _WIN32
    closesocket(m_fd);
#else
    close(m_fd);
#endif
}

NetworkAbstraction *Socket::Accept()
{
    string sError;
    socklen_t addrlen = sizeof(m_peer);
retry:
    int fd = accept(m_fd, reinterpret_cast<sockaddr *>(&m_peer),
#ifdef _WIN32
            reinterpret_cast<int *>(&addrlen) 
#else
            &addrlen 
#endif
            );

#ifdef EHS_DEBUG
    cerr
        << "Got a connection from " << GetPeer() << endl;
#endif
    if (-1 == fd) {
#ifndef _WIN32
        switch (errno) {
            case EAGAIN:
            case EINTR:
                goto retry;
                break;
        }
#endif
        sError.assign("accept: ");
        throw runtime_error(sError.append(strerror(errno)));
        return NULL;
    }
    return new Socket(fd, &m_peer);
}

string Socket::GetPeer() const
{
    char buf[20];
    string ret(GetAddress());
    snprintf(buf, 20, ":%d", GetPort());
    ret.append(buf);
    return ret;
}

string Socket::GetAddress() const
{
    struct in_addr in;
    memcpy (&in, &m_peer.sin_addr.s_addr, sizeof(in));
    return string(inet_ntoa(in));
}


int Socket::GetPort() const
{
    return ntohs(m_peer.sin_port);
}
