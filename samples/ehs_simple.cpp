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
#include <cstdlib>
#include "common.h"

using namespace std;

int main(int argc, char ** argv)
{

	if (argc != 2 && argc != 3) {
        cout << "Usage: " << basename(argv[0]) << " <port> [threaded]" << endl;
        return 0;
	}

	EHS srv;

	EHSServerParameters oSP;
	oSP["port"] = argv[1];

	// start in thread pool mode
	if (3 == argc) {
		cout << "Starting in threaded mode" << endl;
		oSP["mode"] = "threadpool";
		oSP["threadcount"] = 1;
		srv.StartServer(oSP);
        kbdio kbd;
        cout << "Press q to terminate ..." << endl;
        while (!(srv.ShouldTerminate() || kbd.qpressed())) {
            usleep(300000);
        }
    } else {
        // start in single threaded mode

        cout << "Starting in single threaded mode" << endl;
        oSP["mode"] = "singlethreaded";
        srv.StartServer(oSP);
        kbdio kbd;
        cout << "Press q to terminate ..." << endl;
        while (!(srv.ShouldTerminate() || kbd.qpressed())) {
            srv.HandleData(1000); // waits for 1 second
        }
    }
    srv.StopServer();
    return 0;
}
