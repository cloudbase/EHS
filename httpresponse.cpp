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

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "httpresponse.h"
#include "formvalue.h"
#include "httprequest.h"
#include "datum.h"
#include <ctime>
#include <clocale>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace std;

string HttpResponse::HttpTime ( time_t stamp )
{
    char buf[30];
    char *ol = setlocale(LC_TIME, "C"); // We want english (C)
    strftime ( buf, 30, "%a, %d %b %Y %H:%M:%S GMT", gmtime( &stamp ));
    setlocale(LC_TIME, ol); // restore locale
    return string(buf);
}

////////////////////////////////////////////////////////////////////
//  HTTP RESPONSE
//
////////////////////////////////////////////////////////////////////

HttpResponse::HttpResponse ( int inResponseId,
        EHSConnection * ipoEHSConnection ) :
    m_nResponseCode ( HTTPRESPONSECODE_INVALID ),	
    m_oResponseHeaders ( StringCaseMap ( ) ),
    m_oCookieList ( StringList ( ) ),
    m_poEHSConnection ( ipoEHSConnection ),
    m_nResponseId ( inResponseId ),
    m_psBody ( NULL )

{
    // General Header Fields (HTTP 1.1 Section 4.5)
    time_t t = time ( NULL );
    SetDate(t);
    SetLastModified(t);
    m_oResponseHeaders [ "Cache-Control" ] = "no-cache";
    m_oResponseHeaders [ "Content-Type" ] = "text/html";
    m_oResponseHeaders [ "Content-Length" ] = "0";
}

HttpResponse::~HttpResponse ( )
{
    delete [] m_psBody;
}

///< HTTP response code to get text version of
const char *HttpResponse::GetPhrase(ResponseCode code)
{
    static map<int, const char *> responsePhrase;
    static bool init = true;
    if (init) {
        init = false;
        responsePhrase[HTTPRESPONSECODE_200_OK] = "OK";
        responsePhrase[HTTPRESPONSECODE_301_MOVEDPERMANENTLY] = "Moved Permanently";
        responsePhrase[HTTPRESPONSECODE_302_FOUND] = "FOUND";
        responsePhrase[HTTPRESPONSECODE_400_BADREQUEST] = "Bad request";
        responsePhrase[HTTPRESPONSECODE_401_UNAUTHORIZED] = "Unauthorized";
        responsePhrase[HTTPRESPONSECODE_403_FORBIDDEN] = "Forbidden";
        responsePhrase[HTTPRESPONSECODE_404_NOTFOUND] = "Not Found";
        responsePhrase[HTTPRESPONSECODE_500_INTERNALSERVERERROR] = "Internal Server Error";
    }
    map<int, const char *>::const_iterator i = responsePhrase.find(code);
    return (responsePhrase.end() == i) ? "INVALID" : i->second;
}

HttpResponse *HttpResponse::Error(ResponseCode code, int inResponseId, EHSConnection * ipoEHSConnection)
{
    HttpResponse *ret = new HttpResponse(inResponseId, ipoEHSConnection);
    ret->SetResponseCode(code);
    ostringstream oss;
    oss << "<html><head><title>" << code << " " << GetPhrase(code)
        << "</title><body>"  << code << " " << GetPhrase(code) << "</body></html>";
    ret->SetBody(oss.str().c_str(), oss.str().length());
    return ret;
}

HttpResponse *HttpResponse::Error (ResponseCode code, HttpRequest *request)
{
    return Error(code, request->Id(), request->Connection());
}

void HttpResponse::SetDate ( time_t stamp )
{
    m_oResponseHeaders [ "Date" ] = HttpTime( stamp );
}

void HttpResponse::SetLastModified ( time_t stamp )
{
    m_oResponseHeaders [ "Last-Modified" ] = HttpTime( stamp );
}

// sets information regarding the body of the HTTP response
//   sent back to the client
void HttpResponse::SetBody ( const char * ipsBody, ///< body to return to user
        int inBodyLength ///< length of the body
        )
{
    if ( NULL != m_psBody ) {
        delete [] m_psBody;
    }
    m_psBody = new char [ inBodyLength + 1 ];
    if ( NULL == m_psBody ) {
        throw runtime_error ( "HttpResponse::SetBody: Could not allocate m_psBody" ); 
    }
    memset ( m_psBody, 0, inBodyLength + 1 );
    memcpy ( m_psBody, ipsBody, inBodyLength );

    ostringstream oss;
    oss << inBodyLength;
    m_oResponseHeaders [ "Content-Length" ] = oss.str();
}

// this will send stuff if it's not valid.. 
void HttpResponse::SetCookie ( CookieParameters & iroCookieParameters )
{
    // name and value must be set
    if ( iroCookieParameters [ "name" ] != "" &&
            iroCookieParameters [ "value" ] != "" ) {

        ostringstream ssBuffer;
        ssBuffer << iroCookieParameters["name"].GetCharString ( ) <<
            "=" << iroCookieParameters["value"].GetCharString ( );

        // Version should have capital V according to RFC 2109
        if ( iroCookieParameters [ "Version" ] == "" ) {
            iroCookieParameters [ "Version" ] = "1";
        }

        for ( CookieParameters::iterator i = iroCookieParameters.begin ( );
                i != iroCookieParameters.end ( ); i++ ) {
            if ( i->first != "name" && i->first != "value" ) {
                ssBuffer << "; " << i->first << "=" << i->second.GetCharString ( );
            }
        }
        m_oCookieList.push_back ( ssBuffer.str ( ) );
    } else {
#ifdef EHS_DEBUG
        cerr << "Cookie set with insufficient data -- requires name and value" << endl;
#endif
    }
}
