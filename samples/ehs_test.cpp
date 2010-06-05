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
#include <fstream>
#include <cassert>
#include <cstring>
#include <cstdlib>

using namespace std;

class tester : public EHS
{
    protected:
        /**
         * Our new request handler.
         */
        virtual ResponseCode HandleRequest(HttpRequest *, HttpResponse *);

    public:

        /**
         * Constructor
         * @param file The file to serve.
         */
        tester(string file, int delay = 0):
            EHS(), m_nDelay(delay), m_oInfile(file.c_str())
        {
            cout << "loading file '" << file << "'" << endl;
            assert (m_oInfile);
        }

    private:
        // delay
        int m_nDelay;

        // our source file
        ifstream m_oInfile;
};

static int nRequests = 0;
static pthread_mutex_t oRequestMutex = PTHREAD_MUTEX_INITIALIZER;

ResponseCode tester::HandleRequest(HttpRequest *request, HttpResponse *response)
{

    pthread_mutex_lock(&oRequestMutex);
    int nMyRequest = ++nRequests;
    pthread_mutex_unlock(&oRequestMutex);

    if (0 == (nMyRequest % 1000)) {
        cerr << "[" << nMyRequest << "]" << endl;
    }

    char buf[5000];

    sleep (m_nDelay);
    m_oInfile.getline(buf, 4999);
    if (m_oInfile.eof()) {
        // At end of file start from beginning
        m_oInfile.seekg(0);
    }

    string sCopy(buf);
    string sBody(buf);

    sBody.append("<br>previous line: ").append(request->Cookies("ehs_test_cookie"));
    response->SetBody(sBody.c_str(), sBody.length());

    // throw in a cookie here, just to show how it's done
    CookieParameters oCP;
    oCP["name"] = "ehs_test_cookie";
    oCP["value"] = sCopy;
    response->SetCookie(oCP);

    // more attributes can go here

    return HTTPRESPONSECODE_200_OK;
}

string basename(const string & s)
{
    string ret(s);
    size_t pos = ret.rfind("/");
    if (pos != ret.npos)
        ret.erase(0, pos);
    return ret;
}

int main(int argc, char **argv)
{

    if (argc < 4) {
        string cmd = basename(argv[0]);
        cerr << "Usage: " << cmd << " [mode] [port] [file] <delay> <threadcount> <norouterequest>)" << endl;
        cerr << "\tModes: 1 - Single Threaded (last parameter ignored)" << endl;
        cerr << "\tModes: 2 - Multithreaded, fixed number of threads" << endl;
        cerr << "\tModes: 3 - Multithreaded, one thread per request (last parameter ignored)" << endl;
        cerr << "\tnorouterequest: if anything is specified, requests will not be routed" << endl;
        exit(0);
    }

    int nDelay = 0;
    int nThreadCount = 1;
    int nMode = atoi(argv[1]);

    if (argc >= 5) {
        nDelay = atoi(argv[4]);
    }
    if (argc >= 6) {
        nThreadCount = atoi(argv[5]);
    }

    EHSServerParameters oSP;
    oSP["port"] = argv[2];
    if (argc >= 7) {
        oSP["norouterequest"] = 1;
    }
    switch (nMode) {
        case 1:
            cout << "Running in single threaded mode" << endl;
            oSP["mode"] = "singlethreaded";
        case 2:
            cout << "Running with a thread pool of " << nThreadCount << " threads" << endl;
            oSP["mode"] = "threadpool";
            oSP["threadcount"] = nThreadCount;
        case 3:
            cout << "Running with one thread per request" << endl;
            oSP["mode"] = "onethreadperrequest";
        default:
            cerr << "Invalid mode specified: must be 1, 2, or 3" << endl;
            exit(0);
    }
    cout << "binding to " << argv[2] << endl;
    cout << "Delay set to " << nDelay << " seconds" << endl;


    tester srv(argv[3], nDelay);
    srv.StartServer(oSP);

    tester a(argv[3]);
    // Handles URIs like http://localhost/a
    srv.RegisterEHS(&a, "a");

    tester b(argv[3]);
    // Handles URIs like http://localhost/b
    srv.RegisterEHS(&b, "b");

    tester aa(argv[3]);
    // Handles URIs like http://localhost/a/a
    a.RegisterEHS(&aa, "a");

    tester ab(argv[3]);
    // Handles URIs like http://localhost/a/b
    a.RegisterEHS(&ab, "b");


    // if in single threaded mode,
    // we must handle data explicitly.
    if (1 == nMode) {
        while (true) {
            // normally your program would be doing useful things here...
            sleep (1);
            srv.HandleData();
        }
    } else {
        cout << "Press RETURN to terminate the server: "; cout.flush();
        cin.get();
    }

    srv.StopServer();
    return 0;
}
