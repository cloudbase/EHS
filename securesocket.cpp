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

#include "securesocket.h"

#ifdef COMPILE_WITH_SSL
#include <iostream>
#include <sstream>

using namespace std;

SslError g_oSslError;

void SecureSocket::SetBindAddress ( const char *bindAddress ///< address on which to listen
        )
{
    if (bindAddress && strlen(bindAddress))
        m_sBindAddress.assign(bindAddress);
}

void SecureSocket::RegisterBindHelper(PrivilegedBindHelper *helper)
{
    m_poBindHelper = helper;
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

    // no need to ever clean these up

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

    // set up the accept socket
    ostringstream oss;
    if (!m_sBindAddress.empty()) {
        oss << m_sBindAddress << ":";
    }
    oss << inPort;
    // TODO: Invokation of bind helper
    m_poAcceptBio = BIO_new_accept ( (char *)oss.str().c_str() );

    if ( m_poAcceptBio == NULL ) {
#ifdef EHS_DEBUG
        cerr << "couldn't create accept BIO on port " << inPort << endl;
#endif
        assert ( 0 );
    }

    int nAcceptSocketFd = BIO_get_fd ( m_poAcceptBio, NULL );
    u_long MyTrueVariable = 1;
    ioctl ( nAcceptSocketFd, FIONBIO, &MyTrueVariable );

    if ( BIO_do_accept ( m_poAcceptBio ) <= 0 ) {
#ifdef EHS_DEBUG
        cerr << "Couldn't set up accept BIO on port: " << inPort << endl;
#endif
        return NetworkAbstraction::INITSOCKET_BINDFAILED;
    }
    return NetworkAbstraction::INITSOCKET_SUCCESS;
}


int SecureSocket::GetFd ( ) 
{
    // there has got to be a better place for this, but this should work
    if ( m_nFd == -1 ) {
        m_nFd = BIO_get_fd ( m_poAcceptBio, NULL );
    }
    return m_nFd;
}


NetworkAbstraction * SecureSocket::Accept ( ) 
{
#ifdef EHS_DEBUG
    cerr << "Accept called with acceptbio = " << hex << m_poAcceptBio << endl;
#endif

    int nAcceptSocket =  BIO_get_fd ( m_poAcceptBio, NULL );
    assert ( nAcceptSocket >= 0 );

    SecureSocket * poNewSecureSocket = NULL;

    if ( BIO_do_accept ( m_poAcceptBio ) < 0 ) {

#ifdef EHS_DEBUG
        cerr << "Error accepting new connection" << endl;
#endif

    } else {

        BIO * poClientBio = BIO_pop ( m_poAcceptBio );
        assert ( poClientBio != NULL );

        SSL * poClientSsl = SSL_new ( poCtx );
        string sErrorString;

        if ( poClientSsl == NULL ) {
            // cerr << "poClientSsl == NULL" << endl;
        }
        SSL_set_accept_state ( poClientSsl );
        SSL_set_bio ( poClientSsl, poClientBio, poClientBio );

        poNewSecureSocket = new SecureSocket ( poClientSsl, poClientBio );


        socklen_t oInternetSocketAddressLength = sizeof ( poNewSecureSocket->oInternetSocketAddress );

        getpeername ( poNewSecureSocket->GetFd ( ),
                (sockaddr*) &(poNewSecureSocket->oInternetSocketAddress),
                &oInternetSocketAddressLength );

    }

    return poNewSecureSocket;

}

// SSL-only function for initializing special random numbers
int SecureSocket::SeedRandomNumbers ( int inBytes ) {

    if ( !RAND_load_file ( "/dev/random", inBytes ) ) {
        return 0;
    }

    return 1;
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

    // if no callback is specified, use the default one
    if ( m_pfOverridePassphraseCallback == NULL ) {
#ifdef EHS_DEBUG
        cerr << "setting passphrase callback to default";
#endif
        m_pfOverridePassphraseCallback = SecureSocket::DefaultCertificatePassphraseCallback;
    } else {
#ifdef EHS_DEBUG
        cerr << "NOT setting passphrase callback to default" << endl;
#endif
    }

    if ( m_sServerCertificatePassphrase != "" ) {
#ifdef EHS_DEBUG
        cerr << "default callback is at " << hex
            << reinterpret_cast<void *>(SecureSocket::DefaultCertificatePassphraseCallback) << endl;

        cerr
            << "setting callback to '"
            << hex << m_pfOverridePassphraseCallback
            << "' and passphrase = '" << m_sServerCertificatePassphrase << "'" << endl;
#endif
        // set the callback
        SSL_CTX_set_default_passwd_cb ( poCtx, 
                m_pfOverridePassphraseCallback );
        // set the data
        SSL_CTX_set_default_passwd_cb_userdata ( poCtx, 
                (void *) &m_sServerCertificatePassphrase );
    }

    if ( m_sServerCertificate != "" ) {
        if ( SSL_CTX_use_certificate_chain_file ( poCtx, m_sServerCertificate.c_str ( ) ) != 1 ) {
            string sError;
            g_oSslError.GetError ( sError );
#ifdef EHS_DEBUG
            cerr
                << "Error loading server certificate '" << m_sServerCertificate
                << "': '" << sError << "'" << endl;
#endif
            delete poCtx;
            poCtx = NULL;
            return NULL;
        }
    }

    if ( m_sServerCertificate != "" ) {
        if ( SSL_CTX_use_PrivateKey_file ( poCtx, m_sServerCertificate.c_str ( ), SSL_FILETYPE_PEM ) != 1 ) {
#ifdef EHS_DEBUG
            cerr << "Error loading private key" << endl;
#endif
            delete poCtx;
            poCtx = NULL;
            return NULL;
        }
    }

    SSL_CTX_set_verify ( poCtx, 0//SSL_VERIFY_PEER 
            //| SSL_VERIFY_FAIL_IF_NO_PEER_CERT
            ,
            PeerCertificateVerifyCallback );

    //SSL_CTX_set_verify_depth ( poCtx, 4 );

    // set all workarounds for buggy clients and turn off SSLv2 because it's insecure
    SSL_CTX_set_options ( poCtx, SSL_OP_ALL | SSL_OP_NO_SSLv2 );

    if ( SSL_CTX_set_cipher_list ( poCtx, CIPHER_LIST ) != 1 ) {
#ifdef EHS_DEBUG
        cerr << "Error setting ciper list (no valid ciphers)" << endl;
#endif
        delete poCtx;
        poCtx = NULL;
        return NULL;
    }

    return poCtx;
}


int SecureSocket::Read ( void * ipBuffer, int ipBufferLength )
{
    int nReadResult = SSL_read (m_poAcceptSsl, ipBuffer, ipBufferLength );
    // TODO: should really handle errors here
    return nReadResult;
}

int SecureSocket::Send ( const void * ipMessage, size_t inLength, int )
{
    int nWriteResult = SSL_write ( m_poAcceptSsl, ipMessage, inLength );
    // TODO: should really handle errors here
    return nWriteResult;
}

void SecureSocket::Close ( )
{
    m_nClosed = 1;
    BIO_free_all ( m_poAcceptBio );
}

    int 
SecureSocket::DefaultCertificatePassphraseCallback ( char * ipsBuffer,
        int ,
        int ,
        void * ipUserData )
{
#ifdef EHS_DEBUG
    cerr << "Using default certificate passphrase callback function" << endl;
#endif
    strcpy ( ipsBuffer, ((string *)ipUserData)->c_str ( ) );
    int nLength = ((string *)ipUserData)->length ( );
    return nLength;
}

    void 
SecureSocket::SetPassphraseCallback ( int ( * ipfOverridePassphraseCallback ) ( char *, int, int, void * ) )
{
    m_pfOverridePassphraseCallback = ipfOverridePassphraseCallback;
}

string SecureSocket::GetAddress ( )
{
    struct in_addr in;
    memcpy (&in, &oInternetSocketAddress.sin_addr.s_addr, sizeof(in));
    return string(inet_ntoa(in));
}

int SecureSocket::GetPort ( )
{
    return ntohs ( oInternetSocketAddress.sin_port );
}

#endif // COMPILE_WITH_SSL
