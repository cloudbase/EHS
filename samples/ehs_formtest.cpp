/*

  EHS is a library for adding web server support to a C++ application
  Copyright (C) 2001, 2002 Zac Hansen
  
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; version 2
  of the License only.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  
  Zac Hansen ( xaxxon@slackworks.com )

*/

#include <ehs.h>

#include <iostream>
#include <sstream>
#include <string>


using namespace std;

class FormTester : public EHS {

public:
	FormTester ( ) {}
	ResponseCode HandleRequest ( HttpRequest *, HttpResponse * );
	StringList oNameList;
};


// creates a page based on user input -- either displays data from
//   form or presents a form for users to submit data.
ResponseCode FormTester::HandleRequest ( HttpRequest * request, HttpResponse * response )
{
    ostringstream oss;

    oss << "<html><head><title>StringList</title></head>" << endl << "<body>" << endl;
	// if we got data from the user, show it
	if ( request->oFormValueMap [ "user" ].sBody.length ( ) ||
		 request->oFormValueMap [ "existinguser" ].sBody.length ( ) ) {
			
		string sName = request->oFormValueMap [ "existinguser" ].sBody;
		if ( request->oFormValueMap [ "user" ].sBody.length() ) {
			sName = request->oFormValueMap [ "user" ].sBody;
		}
		cerr << "Got name of " << sName << endl;
			
        oss << "Hi " << sName << "</body></html>";
		oNameList.push_back ( sName );
			
		response->SetBody( oss.str().c_str(), oss.str().length() );
		return HTTPRESPONSECODE_200_OK;

	} else {

		// otherwise, present the form to the user to fill in
		cerr << "Got no form data" << endl;

        oss << "<p>Please log in</p>" << endl << "<form action = \"/\" method=\"GET\">" << endl
            << "User name: <input type=\"text\" name=\"user\"><br />" << endl
            << "<select name=\"existinguser\" width=\"20\">" << endl;
		for ( StringList::iterator i = oNameList.begin(); i != oNameList.end ( ); i++ ) {
            oss << "<option>" << i->substr ( 0, 150 ) << endl;
		}
		oss << "</select> <input type=\"submit\">" << endl << "</form>" << endl;
	}
    oss << "</body>" << endl << "</html>";
    response->SetBody( oss.str().c_str(), oss.str().length() );
    return HTTPRESPONSECODE_200_OK;

}

void PrintUsage ( int argc, char ** argv ) {
    cout << "usage: " << argv[0] << " <port> [<certificate_file> <certificate_passphrase>]" << endl;
	cout << "\tIf you specify the last 2 parameters, it will run in https mode" << endl;
	exit ( 0 );

}

// create a multithreaded EHS object with HTTPS support based on command line
//   options.
int main ( int argc, char ** argv )
{

	if ( argc != 2 && argc != 4 ) {
		PrintUsage ( argc, argv );
	}

	FormTester srv;

	int nUseHttps = 0;
	if ( argc > 2 ) {
		nUseHttps = atoi ( argv [ 2 ] );
	}
	EHSServerParameters oSP;


	oSP["port"] = argv [ 1 ];

	oSP [ "mode" ] = "threadpool";

	// unnecessary because 1 is the default
	oSP["threadcount"] = 1;


	if ( argc == 4 ) {
		cout << "in https mode" << endl;
		oSP["https"] = 1;
		oSP["certificate"] = argv [ 2 ];
		oSP["passphrase"] = argv [ 3 ];

	}	

	srv.StartServer ( oSP );
    cout << "Press RETURN to terminate the server: "; cout.flush();
    cin.get();
	srv.StopServer ( );

    return 0;
}

