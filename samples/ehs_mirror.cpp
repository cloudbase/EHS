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
#include <iostream>
#include <sstream>

using namespace std;

class MyEHS : public EHS { 
	
	ResponseCode HandleRequest ( HttpRequest * request, HttpResponse * response ) {

        ostringstream oss;
        oss << "ehs_mirror: Secure - " << (request->nSecure ? "yes" : "no") << endl
            << request->GetAddress() << ":" << request->GetPort() << endl;
		response->SetBody ( oss.str().c_str(), oss.str().length() );
		return HTTPRESPONSECODE_200_OK;
	}

};

int main ( int argc, char ** argv )
{

	if ( ( argc != 2 ) && ( argc != 5 ) ) {
		cout << "Usage: " << argv [ 0 ] << " <port> [<sslport> <certificate file> <passphrase>]" << endl; 
		exit ( 0 );
	}

	EHS * poMyEHS = new MyEHS;

	EHSServerParameters oSP;
	oSP [ "port" ] = argv [ 1 ];
	oSP [ "mode" ] = "threadpool";
	// unnecessary because 1 is the default
	oSP [ "threadcount" ] = 1;

	poMyEHS->StartServer ( oSP );

    if ( argc == 5 ) {
        // create a default EHS object, but set poMyEHS as its source EHS object
        EHS * poEHS = new EHS;
        oSP [ "port" ] = argv [ 2 ];
        oSP [ "https" ] = 1;
        oSP [ "mode" ] = "threadpool";
        oSP [ "threadcount" ] = 1;
        oSP [ "certificate" ] = argv [ 3 ];
        oSP [ "passphrase" ] = argv [ 4 ];

        poEHS->SetSourceEHS ( *poMyEHS );
        poEHS->StartServer ( oSP );
    }

    cout << "Press RETURN to terminate the server: "; cout.flush();
    cin.get();
    poMyEHS->StopServer ( );

    return 0;
}
