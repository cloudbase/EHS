
#include <ehs.h>
#include <iostream>
#include <sstream>

using namespace std;

class MyEHS : public EHS { 
	
	ResponseCode HandleRequest ( HttpRequest * request, HttpResponse * response ) {

        ostringstream oss;
        oss << "ehs_mirror: Secure - " << (request->nSecure ? "yes" : "no")
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
