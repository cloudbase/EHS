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

#ifndef SECURE_SOCKET_H
#define SECURE_SOCKET_H

#ifdef COMPILE_WITH_SSL

#include <openssl/ssl.h>
#include <openssl/rand.h>


#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#include <string>
#include <iostream>

#include "socket.h"
#include "dynamicssllocking.h"
#include "staticssllocking.h"
#include "sslerror.h"


/** use all cipers except adh=anonymous ciphers, low=64 or 54-bit cipers,
 * exp=export crippled ciphers, or md5-based ciphers that have known weaknesses
 * @STRENGTH means order by number of bits
 */
#define CIPHER_LIST "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH"


/// Secure socket implementation used for HTTPS
class SecureSocket : public Socket 
{
    private:

        SecureSocket ( const SecureSocket & );

        SecureSocket & operator = ( const SecureSocket & );

    public:

        /// initializes OpenSSL and this socket object
        virtual InitResult Init ( int inPort );

        /// Constructor for listener socket
        SecureSocket ( std::string isServerCertificate = "",
                std::string isServerCertificatePassphrase = "" );

        /// destructor
        virtual ~SecureSocket ( );

        /// accepts on secure socket
        virtual NetworkAbstraction * Accept ( );

        /// returns true because this socket is considered secure
        virtual bool IsSecure ( ) { return true; }

        /// does OpenSSL read
        virtual int Read ( void * ipBuffer, int ipBufferLength );

        /// does OpenSSL send
        virtual int Send ( const void * ipMessage, size_t inLength, int inFlags = 0 );

        /// closes OpenSSL connectio
        virtual void Close ( );

        /// default callback that just uses m_sServerCertificatePassphrase member
        static int DefaultCertificatePassphraseCallback ( char * ipsBuffer,
                int inSize,
                int inRWFlag,
                void * ipUserData );

        /// sets a callback for loading a certificate
        void SetPassphraseCallback ( int ( * m_ipfOverridePassphraseCallback ) ( char *, int, int, void * ) );

    private:

        /// Constructor for accepted socket
        SecureSocket ( SSL * ipoAcceptSsl, int inAcceptSocket, sockaddr_in * );

        /// Initializes the SSL context and provides it with certificates.
        SSL_CTX * InitializeCertificates ( );

    protected:

        /// the SSL object associated with this SSL connectio
        SSL * m_poAcceptSsl;

        /// filename for certificate file
        std::string m_sServerCertificate;

        /// passphrase for certificate
        std::string m_sServerCertificatePassphrase; 

        /// pointer to callback function
        int (*m_pfOverridePassphraseCallback)(char*, int, int, void*);

    private:

        /// dynamic portion of SSL locking mechanism
        static DynamicSslLocking * poDynamicSslLocking;

        /// static portion of SSL locking mechanism
        static StaticSslLocking * poStaticSslLocking;

        /// error object for getting openssl error messages
        static SslError * poSslError;

        /// certificate information
        static SSL_CTX * poCtx;

        static int refcount;
};

/// global error object
//extern SslError g_oSslError;

#endif

#endif // SECURE_SOCKET_H
