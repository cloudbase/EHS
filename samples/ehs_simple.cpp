
#include <ehs.h>
#include <iostream>

using namespace std;

int main ( int argc, char ** argv )
{

	if ( argc != 2 && argc != 3 ) {
        cout << "Usage: " << argv [ 0 ] << " <port> [threaded]" << endl;
		exit ( 0 );
	}

	EHS * poEHS = new EHS;

	EHSServerParameters oSP;
	oSP [ "port" ] = argv [ 1 ];

	// start in thread pool mode
	if ( argc == 3 ) {
		cout << "Starting in threaded mode" << endl;
		oSP [ "mode" ] = "threadpool";
		oSP [ "threadcount" ] = 1;
		
		poEHS->StartServer ( oSP );
		
		while ( 1 ) {
			sleep ( 1 );
		}
		
	} 
	// start in single threaded mode
	else {
		cout << "Starting in single threaded mode" << endl;
		oSP [ "mode" ] = "singlethreaded";
		
		poEHS->StartServer ( oSP );
		
		while ( 1 ) {
			poEHS->HandleData ( 1000 ); // waits for 1 second
		}
		
	}
}
