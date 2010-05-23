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
#include "datum.h"
#include <cassert>
#include <ctime>
#include <clocale>
#include <cstring>
#include <iostream>
#include <sstream>

using namespace std;

string CreateHttpTime ( )
{
    // 30 is a magic number that is the length of the http
    //   time format + 1 for NULL terminator

    char buf[30];
    time_t t = time ( NULL );
    char *ol = setlocale(LC_TIME, "C"); // We want english (C)
    strftime ( buf, 30, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&t));
    setlocale(LC_TIME, ol);
    return string(buf);

}

////////////////////////////////////////////////////////////////////
//  HTTP RESPONSE
//
////////////////////////////////////////////////////////////////////

HttpResponse::HttpResponse ( int inResponseId,
        EHSConnection * ipoEHSConnection ) :
    m_nResponseCode ( HTTPRESPONSECODE_INVALID ),	
    m_poEHSConnection ( ipoEHSConnection ),
    m_nResponseId ( inResponseId ),
    psBody ( NULL ),
    nBodyLength ( -1 )

{
#ifdef EHS_MEMORY
    cerr << "[EHS_MEMORY] Allocated: HttpResponse" << endl;
#endif   

    string httpTime = CreateHttpTime ( );
    // General Header Fields (HTTP 1.1 Section 4.5)
    oResponseHeaders [ "date" ] = httpTime;
    oResponseHeaders [ "cache-control" ] = "no-cache";
    oResponseHeaders [ "last-modified" ] = httpTime;
    oResponseHeaders [ "content-type" ] = "text/html";
    oResponseHeaders [ "content-length" ] = "0";
}

HttpResponse::~HttpResponse ( )
{
#ifdef EHS_MEMORY
    cerr << "[EHS_MEMORY] Deallocated: HttpResponse" << endl;
#endif
    delete [] psBody;
}

// sets informatino regarding the body of the HTTP response
//   sent back to the client
void HttpResponse::SetBody ( const char * ipsBody, ///< body to return to user
        int inBodyLength ///< length of the body
        )
{
    assert ( psBody == NULL );

    psBody = new char [ inBodyLength + 1 ];
    assert ( psBody != NULL );
    memset ( psBody, 0, inBodyLength + 1 );
    memcpy ( psBody, ipsBody, inBodyLength );

    ostringstream oss;
    oss << inBodyLength;
    oResponseHeaders [ "content-length" ] = oss.str();
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
        oCookieList.push_back ( ssBuffer.str ( ) );
    } else {
#ifdef EHS_DEBUG
        cerr << "Cookie set with insufficient data -- requires name and value" << endl;
#endif
    }
}
