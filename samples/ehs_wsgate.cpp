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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <ehs.h>
#include <debug.h>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <typeinfo>
#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <boost/algorithm/string.hpp>
#include "btexception.h"
#include "base64.h"
#include "sha1.h"

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "common.h"

using namespace std;
using boost::algorithm::to_lower_copy;

static const char * const ws_magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

typedef union {
    unsigned u[5];
    char buf[20];
} sha1_result;

// subclass of EHS that defines a custom HTTP response.
class WsGate : public EHS
{
    public:

    WsGate(EHS *parent = NULL, std::string registerpath = "")
        : EHS(parent, registerpath)
        , m_sHostname("localhost")
    {
    }

    HttpResponse *HandleThreadException(ehs_threadid_t, HttpRequest *request, exception &ex)
    {
        HttpResponse *ret = NULL;
        string msg(ex.what());
        cerr << "##################### Catched " << msg << endl;
        cerr << "request: " << hex << request << dec << endl;
        tracing::exception *btx =
            dynamic_cast<tracing::exception*>(&ex);
        if (NULL != btx) {
            string tmsg = btx->where();
            cerr << "Backtrace:" << endl << tmsg;
            if (0 != msg.compare("fatal")) {
                ret = HttpResponse::Error(HTTPRESPONSECODE_500_INTERNALSERVERERROR, request);
                string body(ret->GetBody());
                tmsg.insert(0, "<br>\n<pre>").append("</pre><p><a href=\"/\">Back to main page</a>");
                body.insert(body.find("</body>"), tmsg);
                ret->SetBody(body.c_str(), body.length());
            }
        }
        return ret;
    }

    // generates a page for each http request
    ResponseCode HandleRequest(HttpRequest *request, HttpResponse *response)
    {
        response->SetBody("", 0);
        if (!request->HttpVersion().compare("1.1")) {
            return HTTPRESPONSECODE_400_BADREQUEST;
        }
        string wshost(to_lower_copy(request->Headers("Host")));
        string wsconn(to_lower_copy(request->Headers("Connection")));
        string wsupg(to_lower_copy(request->Headers("Upgrade")));
        string wsver(request->Headers("Sec-WebSocket-Version"));
        string wskey(request->Headers("Sec-WebSocket-Key"));
        if (!wsconn.compare("upgrade")) {
            return HTTPRESPONSECODE_400_BADREQUEST;
        }
        if (!wsupg.compare("websocket")) {
            return HTTPRESPONSECODE_400_BADREQUEST;
        }
        if (!wshost.compare(m_sHostname)) {
            return HTTPRESPONSECODE_400_BADREQUEST;
        }
        string wskey_decoded(base64_decode(wskey));
        if (16 != wskey_decoded.length()) {
            return HTTPRESPONSECODE_400_BADREQUEST;
        }
        SHA1 sha1;
        sha1_result sha1r;
        sha1 << wskey.c_str() << ws_magic;
        if (!sha1.Result(sha1r.u)) {
            return HTTPRESPONSECODE_500_INTERNALSERVERERROR;
        }
        // FIXME: endianess?
        string sha1str(sha1r.buf, 20);

        response->RemoveHeader("Content-Type");
        response->RemoveHeader("Content-Length");
        response->RemoveHeader("Last-Modified");

        if (!wsver.compare("13")) {
            response->SetHeader("Sec-WebSocket-Version", "13");
            return HTTPRESPONSECODE_426_UPGRADE_REQUIRED;
        }
        response->SetHeader("Sec-WebSocket-Accept",
                base64_encode(reinterpret_cast<const unsigned char *>(sha1str.c_str()), sha1str.length()));
        return HTTPRESPONSECODE_101_SWITCHING_PROTOCOLS;
    }

    void SetHostname(const string &name) {
        m_sHostname = name;
    }

    private:
        string m_sHostname;
};

#ifndef _WIN32
// Bind helper is not needed on win32, because win32 does not
// have a concept of privileged ports.
class MyBindHelper : public PrivilegedBindHelper
{
    public:
        MyBindHelper() : mutex(pthread_mutex_t())
        {
            pthread_mutex_init(&mutex, NULL);
        }

        virtual bool BindPrivilegedPort(int socket, const char *addr, const unsigned short port)
        {
            cerr << "BindPrivilegedPort" << port;

            bool ret = false;
            pid_t pid;
            int status;
            char buf[32];
            pthread_mutex_lock(&mutex);
            switch (pid = fork()) {
                case 0:
                    sprintf(buf, "%08x%08x%04x", socket, inet_addr(addr), port);
                    execl("bindhelper", buf, ((void *)NULL));
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
#endif

class MyRawSocketHandler : public RawSocketHandler
{
    public:
        MyRawSocketHandler() { }

        virtual bool OnData(EHSConnection &conn, std::string data)
        {
            // dummy for now
            return true;
        }

};

// basic main that creates a threaded EHS object and then
//   sleeps forever and lets the EHS thread do its job.
int main (int argc, char **argv)
{
    cout << getEHSconfig() << endl;
    if (argc != 2) {
        cerr << "Usage: " << basename(argv[0]) << " [port]" << endl;
        return 0;
    }

    cerr << "binding to " << atoi(argv[1]) << endl;
    WsGate srv;

#ifndef _WIN32
    MyBindHelper h;
    srv.SetBindHelper(&h);
#endif
    MyRawSocketHandler sh;
    srv.SetRawSocketHandler(&sh);

    EHSServerParameters oSP;
    oSP["port"] = argv[1];
    oSP["mode"] = "onethreadperrequest";

    try {
        srv.StartServer(oSP);

        kbdio kbd;
        cout << "Press q to terminate ..." << endl;
        while (!(srv.ShouldTerminate() || kbd.qpressed())) {
            srv.HandleData(1000);
        }

        srv.StopServer();
    } catch (exception &e) {
        cerr << "ERROR: " << e.what() << endl;
    }

    return 0;
}
