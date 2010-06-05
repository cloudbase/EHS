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
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cerrno>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

// subclass of EHS that defines a custom HTTP response.
class TestHarness : public EHS
{
    // generates a page for each http request
    ResponseCode HandleRequest ( HttpRequest * request, HttpResponse * response )
    {
        ostringstream oss;
        oss
            << "<html><head><title>TestHarness</title></head><body><table><tr>"
            << "<tr><td>request-method:</td><td>" << request->Method() << "</td></tr>" << endl
            << "<tr><td>uri:</td><td>" << request->Uri() << "</td></tr>" << endl
            << "<tr><td>http-version:</td><td>" << request->HttpVersion() << "</td></tr>" << endl
            << "<tr><td>body-length:</td><td>" << request->Body().length ( ) << "</td></tr>" << endl
            << "<tr><td>number-request-headers:</td><td>" << request->Headers().size ( ) << "</td></tr>" << endl
            << "<tr><td>number-form-value-maps:</td><td>" << request->FormValues().size ( ) << "</td></tr>" << endl
            << "<tr><td>client-address:</td><td>" << request->Address ( ) << "</td></tr>" << endl
            << "<tr><td>client-port:</td><td>" << request->Port ( ) << "</td></tr>" << endl;

        for ( StringMap::iterator i = request->Headers().begin ( );
                i != request->Headers().end ( ); i++ ) {
            oss << "<tr><td>Request Header:</td><td>" << i->first << " => " << i->second << "</td></tr>" << endl;
        }

        for ( CookieMap::iterator i = request->Cookies().begin ( );
                i != request->Cookies().end ( ); i++ ) {
            oss << "<tr><td>Cookie:</td><td>" << i->first << " => " << i->second << "</td></tr>" << endl;
        }
        oss << "</body></html>";

        response->SetBody ( oss.str().c_str(), oss.str().length() );
        return HTTPRESPONSECODE_200_OK;
    }
};

class MyHelper : public PrivilegedBindHelper
{
    public:
        MyHelper() : mutex(pthread_mutex_t())
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

//   waits for RETURN while the EHS thread does its job.
int main ( int argc, char ** argv )
{
    if ( argc != 2 ) {
        cerr << "Usage: " << argv[0] << " [port]" << endl;
        exit ( 0 );
    }

    cerr << "binding to " << atoi ( argv [ 1 ] ) << endl;
    TestHarness * srv = new TestHarness;
    PrivilegedBindHelper *h = new MyHelper;
    srv->SetBindHelper(h);

    EHSServerParameters oSP;
    oSP [ "port" ] = argv [ 1 ];
    oSP [ "mode" ] = "threadpool";
    // oSP [ "bindaddress" ] = "127.0.0.1";

    srv->StartServer ( oSP );

    cout << "Press RETURN to terminate the server: "; cout.flush();
    cin.get();

    srv->StopServer ( );
    delete srv;
    delete h;

    return 0;
}
