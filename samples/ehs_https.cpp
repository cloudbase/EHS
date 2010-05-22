
#include <ehs.h> 
#include <iostream> 

using namespace std;

int main ( int argc, char ** argv )
{

	if ( argc != 4 ) {
		cout << "Usage: " << argv [ 0 ] << " <port> <certificate file> <passphrase>" << endl; 
		exit ( 0 );
	}

	EHS * poEHS = new EHS;

	EHSServerParameters oSP;
	oSP [ "port" ] = argv [ 1 ];
	oSP [ "https" ] = 1;
	oSP [ "certificate" ] = argv [ 2 ];
	oSP [ "passphrase" ] = argv [ 3 ];
	oSP [ "mode" ] = "threadpool";

	// unnecessary because 1 is the default
	oSP [ "threadcount" ] = 1;

	poEHS->StartServer ( oSP );
    cout << "Press RETURN to terminate the server: "; cout.flush();
    cin.get();
	poEHS->StopServer ( );

    return 0;
}
