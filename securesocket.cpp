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

#include "ehs.h"
#include "securesocket.h"

#ifdef COMPILE_WITH_SSL
#include <iostream>
#include <sstream>

using namespace std;

static SslError g_oSslError;

// static class variables
DynamicSslLocking * SecureSocket::poDynamicSslLocking = NULL;
StaticSslLocking * SecureSocket::poStaticSslLocking = NULL;
SslError * SecureSocket::poSslError = NULL;
SSL_CTX * SecureSocket::poCtx = NULL;
int SecureSocket::refcount = 0;

SecureSocket::SecureSocket (std::string isServerCertificate,
        PassphraseHandler *handler) : 
    Socket(),
    m_poAcceptSsl(NULL),
    m_sServerCertificate (isServerCertificate),
    m_poPassphraseHandler(handler)
{ 
    refcount++;
#ifdef EHS_DEBUG
    std::cerr << "calling SecureSocket constructor A" << std::endl;
#endif
}

SecureSocket::SecureSocket (SSL * ipoAcceptSsl, 
        int inAcceptSocket, 
        sockaddr_in * ipoInternetSocketAddress) :
    Socket(inAcceptSocket, ipoInternetSocketAddress),
    m_poAcceptSsl(ipoAcceptSsl),
    m_sServerCertificate(""),
    m_poPassphraseHandler(NULL)
{
    refcount++;
#ifdef EHS_DEBUG
    std::cerr << "calling SecureSocket constructor B" << std::endl;
#endif
}

SecureSocket::~SecureSocket ( )
{
#ifdef EHS_DEBUG
    std::cerr << "calling SecureSocket destructor" << std::endl;
#endif
    Close();
    if ( m_poAcceptSsl ) {
        SSL_free( m_poAcceptSsl );
    }
    if (0 == --refcount) {
#ifdef EHS_DEBUG
        std::cerr << "Deleting static members" << std::endl;
#endif
        if ( NULL != poCtx ) {
#ifdef EHS_DEBUG
            std::cerr << "  poCtx" << std::endl;
#endif
            SSL_CTX_free ( poCtx );
        }
        if ( NULL != poSslError ) {
#ifdef EHS_DEBUG
            std::cerr << "  poSslError" << std::endl;
#endif
            delete poSslError;
        }
        if ( NULL != poStaticSslLocking ) {
#ifdef EHS_DEBUG
            std::cerr << "  poStaticSslLocking" << std::endl;
#endif
            delete poStaticSslLocking;
        }
        if ( NULL != poDynamicSslLocking ) {
#ifdef EHS_DEBUG
            std::cerr << "  poDynamicSslLocking" << std::endl;
#endif
            delete poDynamicSslLocking;
        }

        ERR_free_strings();
        ERR_remove_state(0);
        EVP_cleanup();
        CRYPTO_cleanup_all_ex_data();

    }
}

NetworkAbstraction::InitResult 
SecureSocket::Init ( int inPort ///< port on which to listen
        ) {

    // registers available cyphers and digests.  Absolutely required.
    SSL_library_init ( );

    // check and see if we need to initialize one-time data structures
    pthread_mutex_t oMutex;
    pthread_mutex_init ( &oMutex, NULL );
    pthread_mutex_lock ( &oMutex );

    if ( poDynamicSslLocking == NULL ) {
        poDynamicSslLocking = new DynamicSslLocking;
    }

    if ( poStaticSslLocking == NULL ) {
        poStaticSslLocking = new StaticSslLocking;
    }

    // not sure this is needed?
    if ( poSslError == NULL ) {
        poSslError = new SslError;
    }

    if ( poCtx == NULL ) {
        InitializeCertificates ( );
    }

    if ( poCtx == NULL ) {
        return NetworkAbstraction::INITSOCKET_CERTIFICATE;
    }
    pthread_mutex_unlock ( &oMutex );

    return Socket::Init( inPort );
}

NetworkAbstraction * SecureSocket::Accept ( ) 
{
    size_t oInternetSocketAddressLength = sizeof ( m_oInternetSocketAddress );
    int fd = accept ( m_nAcceptSocket, 
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
    if ( fd == -1 ) {
        return NULL;
    }

    // TCP connection is ready. Do server side SSL.
    SSL * ssl = SSL_new ( poCtx );
    if ( NULL == ssl ) {
#ifdef EHS_DEBUG
        cerr << "SSL_new failed" << endl;
#endif
        return NULL;
    }
    SSL_set_fd ( ssl, fd );
    int ret = SSL_accept ( ssl );
    if ( 1 != ret ) {
#ifdef EHS_DEBUG
        string sError;
        g_oSslError.GetError ( sError );
        cerr << "SSL_accept failed: " << sError << endl;
#endif
        SSL_free ( ssl );
        return NULL;
    }

    return new SecureSocket ( ssl, fd, &m_oInternetSocketAddress );
}

int PeerCertificateVerifyCallback ( int inOk,
        X509_STORE_CTX * ipoStore ) {

#ifdef EHS_DEBUG
    if ( !inOk ) {
        char psBuffer [ 256 ];
        X509 * poCertificate = X509_STORE_CTX_get_current_cert ( ipoStore );
        int nDepth = X509_STORE_CTX_get_error_depth ( ipoStore );
        int nError = X509_STORE_CTX_get_error ( ipoStore );

        cerr << "Error in certificate at depth: " << nDepth << endl;
        X509_NAME_oneline ( X509_get_issuer_name ( poCertificate ), psBuffer, 256 );
        cerr << "  issuer  = '" << psBuffer << "'" << endl;
        X509_NAME_oneline ( X509_get_subject_name ( poCertificate ), psBuffer, 256 );
        cerr << "  subject = '" << psBuffer << "'" << endl;
        cerr << "  error " << nError << "," <<  X509_verify_cert_error_string ( nError ) << endl;
    }
#endif

    return inOk;
}

SSL_CTX * 
SecureSocket::InitializeCertificates ( ) {

    // set up the CTX object in compatibility mode.
    //   We'll remove SSLv2 compatibility later for security reasons
    poCtx = SSL_CTX_new ( SSLv23_method ( ) );

    if ( poCtx == NULL ) {
        string sError;
        g_oSslError.GetError ( sError );
#ifdef EHS_DEBUG
        cerr << "Error creating CTX object: '" << sError << "'" << endl;
#endif
        return NULL;
    }

#ifdef EHS_DEBUG
    if ( m_sServerCertificate == "" ) {
        cerr << "No filename for server certificate specified" << endl;
    } else {
        cerr << "Using '" << m_sServerCertificate << "' for certificate" << endl;
    }
#endif


#if 0
    // this sets default locations for trusted CA certificates.  This is not
    //   necessary since we're not dealing with client-side certificat
    if ( ( m_sCertificateFile != "" ||
                m_sCertificateDirectory != "" ) ) {
        if ( SSL_CTX_load_verify_locations ( poCtx, 
                    m_sCertificateFile.c_str ( ),
                    m_sCertificateDirectory != "" ? m_sCertificateDirectory.c_str ( ) : NULL ) != 1 ) {
            string sError;
            g_oSslError.GetError ( sError );
            cerr << "Error loading custom certificate file and/or directory: '" << sError << "'" << endl;
            delete poCtx;
            return NULL;

        }
    }

    // Unknown what this does
    if ( SSL_CTX_set_default_verify_paths ( poCtx ) != 1 ) {
        cerr << "Error loading default certificate file and/or directory" << endl;
        return NULL;
    }
#endif

    // set the callback
    SSL_CTX_set_default_passwd_cb(poCtx, PassphraseCallback);
    // set the data
    SSL_CTX_set_default_passwd_cb_userdata(poCtx, reinterpret_cast<void *>(this));

    if (m_sServerCertificate != "") {
        if (SSL_CTX_use_certificate_chain_file(poCtx, m_sServerCertificate.c_str()) != 1) {
            string sError;
            g_oSslError.GetError(sError);
#ifdef EHS_DEBUG
            cerr
                << "Error loading server certificate '" << m_sServerCertificate
                << "': '" << sError << "'" << endl;
#endif
            delete poCtx;
            poCtx = NULL;
            return NULL;
        }
        if (SSL_CTX_use_PrivateKey_file(poCtx, m_sServerCertificate.c_str(), SSL_FILETYPE_PEM) != 1) {
#ifdef EHS_DEBUG
            cerr << "Error loading private key" << endl;
#endif
            delete poCtx;
            poCtx = NULL;
            return NULL;
        }
    }

    SSL_CTX_set_verify( poCtx, 0//SSL_VERIFY_PEER 
            //| SSL_VERIFY_FAIL_IF_NO_PEER_CERT
            ,
            PeerCertificateVerifyCallback);

    //SSL_CTX_set_verify_depth(poCtx, 4);

    // set all workarounds for buggy clients and turn off SSLv2 because it's insecure
    SSL_CTX_set_options(poCtx, SSL_OP_ALL | SSL_OP_NO_SSLv2);

    if (SSL_CTX_set_cipher_list(poCtx, CIPHER_LIST) != 1) {
#ifdef EHS_DEBUG
        cerr << "Error setting ciper list (no valid ciphers)" << endl;
#endif
        delete poCtx;
        poCtx = NULL;
        return NULL;
    }

    return poCtx;
}


int SecureSocket::Read(void * ipBuffer, int ipBufferLength)
{
    int nReadResult = SSL_read(m_poAcceptSsl, ipBuffer, ipBufferLength);
    // TODO: should really handle errors here
    return nReadResult;
}

int SecureSocket::Send (const void * ipMessage, size_t inLength, int)
{
    int nWriteResult = SSL_write(m_poAcceptSsl, ipMessage, inLength);
    // TODO: should really handle errors here
    return nWriteResult;
}

void SecureSocket::Close()
{
    Socket::Close();
}

    int 
SecureSocket::PassphraseCallback(char * buf, int bufsize, int rwflag, void * userdata)
{
#ifdef EHS_DEBUG
    cerr << "Invoking certificate passphrase callback" << endl;
#endif
    SecureSocket *s = reinterpret_cast<SecureSocket *>(userdata);
    int ret = 0;
    if (NULL != s->m_poPassphraseHandler) {
        string passphrase = s->m_poPassphraseHandler->GetPassphrase((1 == rwflag));
        ret = passphrase.length();
        if (ret > 0) {
            ret = ((bufsize - 1) < ret) ? (bufsize - 1): ret;
            strncpy ( buf, passphrase.c_str ( ), ret );
            buf[ret + 1] = 0;
        }
    }
    return ret;
}

#endif // COMPILE_WITH_SSL
