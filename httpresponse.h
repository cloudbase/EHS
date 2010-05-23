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

#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <ehstypes.h>

/// different response codes and their corresponding phrases -- defined in EHS.cpp
enum ResponseCode { HTTPRESPONSECODE_INVALID = 0,
    HTTPRESPONSECODE_200_OK = 200,
    HTTPRESPONSECODE_301_MOVEDPERMANENTLY = 301,
    HTTPRESPONSECODE_302_FOUND = 302,
    HTTPRESPONSECODE_401_UNAUTHORIZED = 401,
    HTTPRESPONSECODE_403_FORBIDDEN = 403,
    HTTPRESPONSECODE_404_NOTFOUND = 404,
    HTTPRESPONSECODE_500_INTERNALSERVERERROR = 500 };

/// Holds strings corresponding to the items in the ResponseCode enumeration
extern const char * ResponsePhrase [ ]; 


/// This defines what is sent back to the client
/**
 * This defines what is sent back to the client.  It contains the actual body, any
 *   headers specified, and the response code.
 */
class HttpResponse {

    public:

        /// constructor
        HttpResponse ( int inResponseId, EHSConnection * ipoEHSConnection );

        /// destructor
        ~HttpResponse ( );

        /// sets information about the body of the response being sent back to the client.
        void SetBody ( const char * ipsBody, ///< body to be sent to client
                int inBodyLength ///< length of body to be sent to client
                );

        /// sets cookies for the response
        void SetCookie ( CookieParameters & iroCookieParameters );

        /// Returns the body of the response
        char * GetBody ( ) { return psBody; };

        /// the response code to be sent back
        ResponseCode m_nResponseCode;

        /// these are the headers sent back to the client in the http response.  Things like content-type and content-length
        StringMap oResponseHeaders;

        /// cookies waiting to be sent
        StringList oCookieList;

        /// ehs connection object this response goes back on
        EHSConnection * m_poEHSConnection;

        /// response id for making sure we send responses in the right order
        int m_nResponseId;

    protected:

        /// request id for this request's connection object
        int m_nRequestId;

        /// the actual body to be sent back -- set by SetBody
        char * psBody;

        /// the size of the body to be sent back -- set by SetBody
        int nBodyLength;

};

#endif // HTTPRESPONSE_H
