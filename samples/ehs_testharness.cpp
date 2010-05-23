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

#include <ehs.h>
#include <sstream>
#include <iostream>

using namespace std;

// subclass of EHS that defines a custom HTTP response.
class TestHarness : public EHS
{
    // generates a page for each http request
    ResponseCode HandleRequest ( HttpRequest * request, HttpResponse * response )
    {
        ostringstream oss;
        oss
            << "request-method:         " << request->nRequestMethod << endl
            << "uri:                    " << request->sUri << endl
            << "http-version:           " << request->sHttpVersionNumber << endl
            << "body-length:            " << request->sBody.length ( ) << endl
            << "number-request-headers: " << request->oRequestHeaders.size ( ) << endl
            << "number-form-value-maps: " << request->oFormValueMap.size ( ) << endl
            << "client-address:         " << request->GetAddress ( ) << endl
            << "client-port:            " << request->GetPort ( ) << endl;

        for ( StringMap::iterator i = request->oRequestHeaders.begin ( );
                i != request->oRequestHeaders.end ( ); i++ ) {
            oss << "Request Header:         " << i->first << " => " << i->second << endl;
        }

        for ( CookieMap::iterator i = request->oCookieMap.begin ( );
                i != request->oCookieMap.end ( ); i++ ) {
            oss << "Cookie:                 " << i->first << " => " << i->second << endl;
        }

        response->SetBody ( oss.str().c_str(), oss.str().length() );
        return HTTPRESPONSECODE_200_OK;
    }
};

// basic main that creates a threaded EHS object and then
//   sleeps forever and lets the EHS thread do its job.
int main ( int argc, char ** argv )
{
    if ( argc != 2 ) {
        cerr << "Usage: " << argv[0] << " [port]" << endl;
        exit ( 0 );
    }

    cerr << "binding to " << atoi ( argv [ 1 ] ) << endl;
    TestHarness * srv = new TestHarness;

    EHSServerParameters oSP;
    oSP [ "port" ] = argv [ 1 ];
    oSP [ "mode" ] = "threadpool";
    // oSP [ "bindaddress" ] = "127.0.0.1";

    // unnecessary because 1 is the default
    oSP [ "threadcount" ] = 1;

    srv->StartServer ( oSP );

    cout << "Press RETURN to terminate the server: "; cout.flush();
    cin.get();

    srv->StopServer ( );

    return 0;
}
