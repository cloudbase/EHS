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
#include <cstring>
#include <cstdio>
#include <cerrno>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

class MyHelper : public PrivilegedBindHelper
{
    public:
        MyHelper()
        {
            pthread_mutex_init(&mutex, NULL);
        }

        virtual bool BindPrivilegedPort(int socket, const char *addr, const unsigned short port)
        {
            bool ret = false;
            pid_t pid;
            int status;
            char buf[32];
            pthread_mutex_lock(&mutex);
            switch (pid = fork()) {
                case 0:
                    sprintf(buf, "%08x%08x%04x", socket, inet_addr(addr), port);
                    execl("bindhelper", buf, NULL);
                    exit(errno);
                    break;
                case -1:
                    break;
                default:
                    if (waitpid(pid, &status, 0) != -1) {
                        ret = (0 == status);
                        if (0 != status)
                            cerr << "bind: " << strerror(WEXITSTATUS(status)) << endl;
                    }
                    break;
            }
            pthread_mutex_unlock(&mutex);
            return ret;
        }

    private:
        pthread_mutex_t mutex;
};

int main ( int argc, char ** argv )
{

	if ( argc != 4 ) {
		cout << "Usage: " << argv [ 0 ] << " <port> <certificate file> <passphrase>" << endl; 
		exit ( 0 );
	}

	EHS * poEHS = new EHS;
    PrivilegedBindHelper *h = new MyHelper;
    poEHS->SetBindHelper(h);

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
