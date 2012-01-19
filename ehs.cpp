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

#include <pthread.h>

#include "ehs.h"
#include "networkabstraction.h"
#include "ehsconnection.h"
#include "ehsserver.h"
#include "socket.h"
#include "securesocket.h"
#include "debug.h"
#include "mutexhelper.h"

#include <pcrecpp.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cerrno>

#ifdef COMPILE_WITH_SSL
# include <openssl/opensslv.h>
#endif
static const char * const EHSconfig = "EHS_CONFIG:SSL="
#ifdef COMPILE_WITH_SSL
"\"" OPENSSL_VERSION_TEXT "\""
#else
"DISABLED"
#endif
",DEBUG="
#ifdef EHS_DEBUG
"1"
#else
"0"
#endif
",VERSION=" VERSION ",RELEASE=" SVNREV ",BUILD=" __DATE__ " " __TIME__;

const char * getEHSconfig()
{
    return EHSconfig;
}

using namespace std;

class EHSThreadHandlerHelper
{
    public:
        EHSThreadHandlerHelper(EHS *ehs)
            : m_pEHS(ehs)
        {
            if (m_pEHS) {
                if (!m_pEHS->ThreadInitHandler()) {
                    m_pEHS = NULL;
                }
            }
        }

        ~EHSThreadHandlerHelper()
        {
            if (m_pEHS) {
                m_pEHS->ThreadExitHandler();
                m_pEHS = NULL;
            }
        }

        bool IsOK()
        {
            return (NULL != m_pEHS);
        }

    private:
        EHS *m_pEHS;
};

int EHSServer::CreateFdSet()
{
    // don't lock mutex, as this is only called from within a locked section
    FD_ZERO(&m_oReadFds);
    // add the accepting FD	
    FD_SET(m_poNetworkAbstraction->GetFd(), &m_oReadFds);
    int nHighestFd = m_poNetworkAbstraction->GetFd();
    for (EHSConnectionList::iterator i = m_oEHSConnectionList.begin();
            i != m_oEHSConnectionList.end(); i++) {
        /// skip this one if it's already been used
        if ((*i)->StillReading()) {
            int nCurrentFd = (*i)->GetNetworkAbstraction()->GetFd();
            // EHS_TRACE("Adding %d to FD SET", nCurrentFd);
            FD_SET(nCurrentFd, &m_oReadFds);
            // store the highest FD in the set to return it
            if (nCurrentFd > nHighestFd) {
                nHighestFd = nCurrentFd;
            }
        } else {
            EHS_TRACE("FD %d isn't reading anymore",
                    (*i)->GetNetworkAbstraction()->GetFd());
        }
    }
    return nHighestFd;
}

void EHSServer::ClearIdleConnections()
{
    // don't lock mutex, as this is only called from within locked sections
    for (EHSConnectionList::iterator i = m_oEHSConnectionList.begin();
            i != m_oEHSConnectionList.end(); i++) {

        MutexHelper mh(&(*i)->m_oMutex);
        // if it's been more than N seconds since a response has been
        //   sent and there are no pending requests
        if ((*i)->StillReading() &&
                time(NULL) - (*i)->LastActivity() > m_nIdleTimeout &&
                (*i)->RequestsPending()) {
            EHS_TRACE("Done reading because of idle timeout");
            mh.Unlock();
            (*i)->DoneReading(false);
            mh.Lock();
        }
    }
    RemoveFinishedConnections();
}

void EHSServer::RemoveFinishedConnections ( )
{
    // don't lock mutex, as this is only called from within locked sections
    for (EHSConnectionList::iterator i = m_oEHSConnectionList.begin();
            i != m_oEHSConnectionList.end(); ) {
        if ((*i)->CheckDone()) {
            EHS_TRACE("Found something to delete");
            RemoveEHSConnection(*i);
            i = m_oEHSConnectionList.begin();
        } else {
            i++;
        }
    }
}


EHSConnection::EHSConnection(NetworkAbstraction *ipoNetworkAbstraction,
        EHSServer * ipoEHSServer) :
    m_bDoneReading(false),
    m_bDisconnected(false),
    m_poCurrentHttpRequest(NULL),
    m_poEHSServer(ipoEHSServer),
    m_nLastActivity(0),
    m_nRequests(0),
    m_nResponses(0),
    m_nActiveRequests(0),
    m_poNetworkAbstraction(ipoNetworkAbstraction),
    m_sBuffer(""),
    m_oHttpResponseMap(HttpResponseMap()),
    m_oHttpRequestList(HttpRequestList()),
    m_sAddress(ipoNetworkAbstraction->GetAddress()),
    m_nPort(ipoNetworkAbstraction->GetPort()),
    m_nMaxRequestSize(MAX_REQUEST_SIZE_DEFAULT),
    m_oMutex(pthread_mutex_t())
{
    UpdateLastActivity();
    // initialize mutex for this object
    pthread_mutex_init(&m_oMutex, NULL);
}


EHSConnection::~EHSConnection()
{
    delete m_poCurrentHttpRequest;
    delete m_poNetworkAbstraction;
    pthread_mutex_destroy(&m_oMutex);
}


NetworkAbstraction *EHSConnection::GetNetworkAbstraction()
{
    return m_poNetworkAbstraction;
}


// adds data to the current buffer for this connection
EHSConnection::AddBufferResult
EHSConnection::AddBuffer(char *ipsData, ///< new data to be added
        int inSize ///< size of new data
        )
{
    MutexHelper mh(&m_oMutex);
    // make sure we actually got some data
    if ( inSize <= 0 ) {
        return ADDBUFFER_INVALID;
    }
    // make sure the buffer doesn't grow too big
    if ((m_sBuffer.length() + inSize) > m_nMaxRequestSize) {
        EHS_TRACE("MaxRequestSize (%lu) exceeded.", m_nMaxRequestSize);
        return ADDBUFFER_TOOBIG;
    }
    // this is binary safe -- only the single argument char* constructor looks for NULL
    m_sBuffer += string ( ipsData, inSize );
    // need to run through our buffer until we don't get a full result out
    do {
        // if we need to make a new request object, do that now
        if (NULL == m_poCurrentHttpRequest ||
                m_poCurrentHttpRequest->m_nCurrentHttpParseState == HttpRequest::HTTPPARSESTATE_COMPLETEREQUEST ) {
            // if we have one already, toss it on the list
            if (NULL != m_poCurrentHttpRequest) {
                m_oHttpRequestList.push_back(m_poCurrentHttpRequest);
                m_poEHSServer->IncrementRequestsPending();
                // wake up everyone
                pthread_cond_broadcast(& m_poEHSServer->m_oDoneAccepting);
                if (m_poEHSServer->m_nServerRunningStatus == EHSServer::SERVERRUNNING_ONETHREADPERREQUEST ) {
                    // create a thread if necessary
                    pthread_t oThread;
                    mh.Unlock();
                    if (0 != pthread_create(&oThread, &(m_poEHSServer->m_oThreadAttr),
                                EHSServer::PthreadHandleData_ThreadedStub,
                                (void *)m_poEHSServer)) {
                        return ADDBUFFER_NORESOURCE;
                    }
                    EHS_TRACE("Created thread with TID=0x%x, NULL, func=0x%x, data=0x%x\n",
                            oThread, EHSServer::PthreadHandleData_ThreadedStub, (void *) m_poEHSServer);
                    pthread_detach(oThread);
                    mh.Lock();
                }
            }
            // create the initial request
            m_poCurrentHttpRequest = new HttpRequest(++m_nRequests, this);
            m_poCurrentHttpRequest->m_bSecure = m_poNetworkAbstraction->IsSecure();
        }
        // parse through the current data
        m_poCurrentHttpRequest->ParseData(m_sBuffer);
    } while (m_poCurrentHttpRequest->m_nCurrentHttpParseState ==
            HttpRequest::HTTPPARSESTATE_COMPLETEREQUEST);
    if ( m_poCurrentHttpRequest->m_nCurrentHttpParseState == HttpRequest::HTTPPARSESTATE_INVALIDREQUEST ) {
        return ADDBUFFER_INVALIDREQUEST;
    }
    return ADDBUFFER_OK;
}

/// call when no more reads will be performed on this object.  inDisconnected is true when client has disconnected
void EHSConnection::DoneReading(bool ibDisconnected)
{
    MutexHelper mh(&m_oMutex);
    m_bDoneReading = true;
    m_bDisconnected = ibDisconnected;
}

HttpRequest * EHSConnection::GetNextRequest()
{
    HttpRequest *ret = NULL;
    MutexHelper mh(&m_oMutex);
    if (!m_oHttpRequestList.empty()) {
        ret = m_oHttpRequestList.front();
        m_oHttpRequestList.pop_front();
        ++m_nActiveRequests;
    }
    return ret;
}


int EHSConnection::CheckDone()
{
    // if we're not still reading, we may want to drop this connection
    if ( !StillReading ( ) ) {
        // if we're done with all our responses (-1 because the next (unused) request is already created)
        if (m_nRequests - 1 <= m_nResponses) {
            // if we haven't disconnected, do that now
            if (!m_bDisconnected) {
                EHS_TRACE ("Closing connection");
                m_poNetworkAbstraction->Close();
            }
            return 1;
        }
    }
    return 0;
}


////////////////////////////////////////////////////////////////////
// EHS SERVER
////////////////////////////////////////////////////////////////////

EHSServer::EHSServer (EHS *ipoTopLevelEHS) :
    m_nServerRunningStatus(SERVERRUNNING_NOTRUNNING),
    m_poTopLevelEHS(ipoTopLevelEHS),
    m_bAcceptedNewConnection(false),
    m_oMutex(pthread_mutex_t()),
    m_oDoneAccepting(pthread_cond_t()),
    m_nRequestsPending(0),
    m_bAccepting(false),
    m_sServerName(""),
    m_oReadFds(fd_set()),
    m_oEHSConnectionList(EHSConnectionList()),
    m_poNetworkAbstraction(NULL),
    m_nAcceptThreadId(0),
    m_nIdleTimeout(15),
    m_nThreads(0),
    m_oCurrentRequest(CurrentRequestMap()),
    m_oThreadAttr(pthread_attr_t())
{
    // you HAVE to specify a top-level EHS object
    if (NULL == m_poTopLevelEHS) {
        throw invalid_argument("EHSServer::EHSServer: Pointer to toplevel EHS object is NULL.");
    }

    // grab out the parameters for less typing later on
    EHSServerParameters & params = ipoTopLevelEHS->m_oParams;

    pthread_mutex_init(&m_oMutex, NULL);
    pthread_cond_init(&m_oDoneAccepting, NULL);
    pthread_attr_init(&m_oThreadAttr);
    {
        // Set minimum stack size
        size_t stacksize;
        size_t min_stacksize = (unsigned long)params["stacksize"];
        pthread_attr_getstacksize (&m_oThreadAttr, &stacksize);
        EHS_TRACE ("Default thread stack size is %li", stacksize);
        if (stacksize < min_stacksize) {
            EHS_TRACE ("Setting thread stack size to %li", min_stacksize);
            pthread_attr_setstacksize(&m_oThreadAttr, min_stacksize);
        }
    }
    // whether to run with https support
    int nHttps = params["https"];
    if (nHttps) {
        EHS_TRACE("EHSServer running in HTTPS mode");
    } else {
        EHS_TRACE("EHSServer running in plain-text mode (no HTTPS)");
    }
    try {
        // are we using secure sockets?
        if (nHttps) {
#ifdef COMPILE_WITH_SSL		
            EHS_TRACE("Trying to create secure socket with certificate='%s' and passphrase='%s'",
                    (const char*)params["certificate"],
                    (const char*)params["passphrase"]);
            m_poNetworkAbstraction = new SecureSocket(params["certificate"],
                    reinterpret_cast<PassphraseHandler *>(ipoTopLevelEHS));
#else // COMPILE_WITH_SSL
            throw runtime_error("EHSServer::EHSServer: EHS not compiled with SSL support. Cannot create HTTPS server.");
#endif // COMPILE_WITH_SSL
        } else {
            m_poNetworkAbstraction = new Socket();
        }

        if (NULL == m_poNetworkAbstraction) {
            throw runtime_error("EHSServer::EHSServer: Could not allocate socket.");
        }
        // initialize the socket
        if (params["bindaddress"] != "") {
            m_poNetworkAbstraction->SetBindAddress(params["bindaddress"]);
        }
        m_poNetworkAbstraction->RegisterBindHelper(m_poTopLevelEHS->GetBindHelper());
        m_poNetworkAbstraction->Init(params["port"]); // initialize socket stuff
        if (params["mode"] == "threadpool") {
            // need to set this here because the thread will check this to make
            // sure it's supposed to keep running
            m_nServerRunningStatus = SERVERRUNNING_THREADPOOL;
            // create a pthread
            int nThreadsToStart = params["threadcount"].GetInt();
            if (nThreadsToStart <= 0) {
                nThreadsToStart = 1;
            }
            EHS_TRACE ("Starting %d threads in pool", nThreadsToStart);
            for (int i = 0; i < nThreadsToStart; i++) {
                // create new thread and detach so we don't have to join on it
                if (0 == pthread_create(&m_nAcceptThreadId, &m_oThreadAttr,
                            EHSServer::PthreadHandleData_ThreadedStub, (void *)this)) {
                    EHS_TRACE("Created thread with ID=0x%x, NULL, func=0x%x, this=0x%x",
                            m_nAcceptThreadId, EHSServer::PthreadHandleData_ThreadedStub, this);
                    pthread_detach(m_nAcceptThreadId);
                } else {
                    m_nServerRunningStatus = SERVERRUNNING_NOTRUNNING;
                    throw runtime_error("EHSServer::EHSServer: Unable to create threads");
                }
            }
        } else if (params["mode"] == "onethreadperrequest") {
            m_nServerRunningStatus = SERVERRUNNING_ONETHREADPERREQUEST;
            // spawn off one thread just to deal with basic stuff
            if (0 == pthread_create(&m_nAcceptThreadId, &m_oThreadAttr,
                        EHSServer::PthreadHandleData_ThreadedStub, (void *)this)) {
                EHS_TRACE("Created thread with ID=0x%x, NULL, func=0x%x, this=0x%x",
                        m_nAcceptThreadId, EHSServer::PthreadHandleData_ThreadedStub, this);
                pthread_detach(m_nAcceptThreadId);
            } else {
                m_nServerRunningStatus = SERVERRUNNING_NOTRUNNING;
                throw runtime_error("EHSServer::EHSServer: Unable to create listener thread");
            }
        } else if (params["mode"] == "singlethreaded") {
            // we're single threaded
            m_nServerRunningStatus = SERVERRUNNING_SINGLETHREADED;
        } else {
            throw runtime_error("EHSServer::EHSServer: invalid mode specified");
        }
    } catch (...) {
        delete m_poNetworkAbstraction;
        throw;
    }
    switch (m_nServerRunningStatus) {
        case SERVERRUNNING_THREADPOOL:
            EHS_TRACE("EHS Server running in threadpool mode with %s threads",
                    params["threadcount"] == "" ? "1" :
                    params[ "threadcount"].GetCharString());
            break;
        case SERVERRUNNING_ONETHREADPERREQUEST:
            EHS_TRACE("EHS Server running with one thread per request");
            break;
        case SERVERRUNNING_SINGLETHREADED:
            EHS_TRACE("EHS Server running in singlethreaded mode");
            break;
        default:
            EHS_TRACE("EHS Server not running. Server initialization failed.");
            break;
    }
    return;
}


EHSServer::~EHSServer ( )
{
    delete m_poNetworkAbstraction;
    // Delete all elements in our connection list
    while ( ! m_oEHSConnectionList.empty() ) {
        delete m_oEHSConnectionList.front ( );
        m_oEHSConnectionList.pop_front ( );
    }
    pthread_mutex_destroy(&m_oMutex);
}

HttpRequest * EHSServer::GetNextRequest()
{
    // don't lock because this is only called from within locked sections
    HttpRequest * poNextRequest = NULL;
    // pick a random connection if the list isn't empty
    if (!m_oEHSConnectionList.empty()) {
        // pick a random connection, so no one takes too much time
        int nWhich = (int)(((double)m_oEHSConnectionList.size()) * rand() / (RAND_MAX + 1.0));
        // go to that element
        EHSConnectionList::iterator i = m_oEHSConnectionList.begin();
        int nCounter = 0;
        for (nCounter = 0; nCounter < nWhich; nCounter++) {
            i++;
        }
        // now get the next available request treating the list as circular
        EHSConnectionList::iterator iStartPoint = i;
        int nFirstTime = 1;
        while (poNextRequest == NULL && !(iStartPoint == i && nFirstTime == 0)) {
            // check this one to see if it has anything
            poNextRequest = (*i)->GetNextRequest();
            if (++i == m_oEHSConnectionList.end()) {
                i = m_oEHSConnectionList.begin();
            }
            nFirstTime = 0;
            // decrement the number of pending requests
            if (poNextRequest != NULL) {
                m_nRequestsPending--;
            }
        }
    }
    if (poNextRequest == NULL) {
        //		EHS_TRACE ("No request found\n");
    } else {
        EHS_TRACE ("Found request");
    }
    return poNextRequest;
}

void EHSServer::RemoveEHSConnection(EHSConnection * ipoEHSConnection)
{
    // don't lock as this is only called from within locked sections
    if (NULL == ipoEHSConnection) {
        throw invalid_argument("EHSServer::RemoveEHSConnection: argument is NULL");
    }
    bool removed = false;
    EHS_TRACE("Look for something to delete within a list of %d connections",
            m_oEHSConnectionList.size ( ) );
    // go through the list and find all occurances of ipoEHSConnection
    for (EHSConnectionList::iterator i = m_oEHSConnectionList.begin();
            i != m_oEHSConnectionList.end(); /* no third part */) {
        if (*i == ipoEHSConnection) {
            if (removed) {
                throw runtime_error("EHSServer::RemoveEHSConnection: Deleting a second element");
            }
            removed = true;
            // destroy the connection and remove it from the list
            // erase() returns an iterator pointing to the following element.
            delete *i;
            i = m_oEHSConnectionList.erase(i);
        } else {
            i++;
        }
    }
}

bool EHS::ThreadInitHandler()
{
    EHS_TRACE("called");
    return true;
}

void EHS::ThreadExitHandler()
{
    EHS_TRACE("called");
}

const std::string EHS::GetPassphrase(bool /* twice */)
{
    return m_oParams["passphrase"];
}

void EHS::StartServer(EHSServerParameters &params)
{
    m_oParams = params;
    if (m_poEHSServer != NULL) {
        throw runtime_error("EHS::StartServer: already running");
    } else {
        // associate a EHSServer object to this EHS object
        m_bNoRouting = (m_oParams.find("norouterequest") != m_oParams.end());
        try {
            m_poEHSServer = new EHSServer(this);
        } catch (...) {
            delete m_poEHSServer;
            throw;
        }
    }
}

// this is the function specified to pthread_create under UNIX
//   because you can't start a thread directly into a class method
void * EHSServer::PthreadHandleData_ThreadedStub(void * ipParam ///< EHSServer object cast to a void pointer
        )
{
    EHSServer *self = reinterpret_cast<EHSServer *>(ipParam);
    MutexHelper mh(&self->m_oMutex);
    self->m_nThreads++;
    mh.Unlock();
    self->HandleData_Threaded();
    mh.Lock();
    self->m_nThreads--;
    return NULL;
}

void EHS::StopServer()
{
    // make sure we're in a sane state
    if ((NULL == m_poParent) && (NULL == m_poEHSServer)) {
        throw runtime_error("EHS::StopServer: Invalid state");
    }

    if (m_poParent) {
        m_poParent->StopServer();
    } else if (m_poEHSServer) {
        m_poEHSServer->EndServerThread();
        delete m_poEHSServer;
        m_poEHSServer = NULL;
    }
}

bool EHS::ShouldTerminate() const
{
    // make sure we're in a sane state
    if ((NULL == m_poParent) && (NULL == m_poEHSServer)) {
        throw runtime_error("EHS::StopServer: Invalid state");
    }

    bool ret = false;
    if (m_poParent) {
        ret = m_poParent->ShouldTerminate();
    } else if (m_poEHSServer) {
        ret = (EHSServer::SERVERRUNNING_SHOULDTERMINATE == m_poEHSServer->RunningStatus());
    }
    return ret;
}

void EHSServer::HandleData_Threaded()
{
    EHSThreadHandlerHelper thh(m_poTopLevelEHS);
    if (thh.IsOK()) {
        pthread_t self = pthread_self();
        do {
            bool catched = false;
            HttpResponse *eResponse = NULL;

            try {
                HandleData(1000, self); // 1000ms select timeout
            } catch (exception &e) {
                catched = true;
                eResponse = m_poTopLevelEHS->HandleThreadException(self, m_oCurrentRequest[self], e);
            } catch (...) {
                catched = true;
                runtime_error e("unspecified");
                eResponse = m_poTopLevelEHS->HandleThreadException(self, m_oCurrentRequest[self], e);
            }
            if (catched) {
                if (NULL != eResponse) {
                    eResponse->m_poEHSConnection->AddResponse(eResponse);
                    MutexHelper mutex(&m_oMutex);
                    delete m_oCurrentRequest[self];
                    m_oCurrentRequest[self] = NULL;
                } else {
                    m_nServerRunningStatus = SERVERRUNNING_SHOULDTERMINATE;
                    m_nAcceptThreadId = 0;
                    MutexHelper mutex(&m_oMutex);
                    delete m_oCurrentRequest[self];
                    m_oCurrentRequest[self] = NULL;
                }
            }
        } while (m_nServerRunningStatus == SERVERRUNNING_THREADPOOL ||
                self == m_nAcceptThreadId);
        m_poNetworkAbstraction->ThreadCleanup();
    }
}

void EHSServer::HandleData (int inTimeoutMilliseconds, pthread_t tid)
{
    MutexHelper mutex(&m_oMutex);
    // determine if there are any jobs waiting if this thread should --
    //   if we're running one-thread-per-request and this is the accept thread
    //   we don't look for requests
    m_oCurrentRequest[tid] = NULL;
    if (m_nServerRunningStatus != SERVERRUNNING_ONETHREADPERREQUEST ||
            tid != m_nAcceptThreadId ) {
        m_oCurrentRequest[tid] = GetNextRequest();
    }
    // if we got a request to handle
    if (NULL != m_oCurrentRequest[tid]) {
        HttpRequest *req = m_oCurrentRequest[tid];
        // handle the request and post it back to the connection object
        mutex.Unlock();
        // route the request
        HttpResponse *response = m_poTopLevelEHS->RouteRequest(req).release();
        response->m_poEHSConnection->AddResponse(response);
        delete req;
        mutex.Lock();
        m_oCurrentRequest[tid] = NULL;
    } else {
        // otherwise, no requests are pending

        // if something is already accepting, sleep
        if (m_bAccepting) {
            // wait until something happens
            // it's ok to not recheck our condition here, as we'll come back in the same way and recheck then
            EHS_TRACE("Waiting on m_oDoneAccepting condition TID=%p", pthread_self());
            pthread_cond_wait(&m_oDoneAccepting, &m_oMutex);
            EHS_TRACE("Done waiting on m_oDoneAccepting condition TID=%p", pthread_self());
        } else {
            // if no one is accepting, we accept
            m_bAcceptedNewConnection = false;
            // we're now accepting
            m_bAccepting = true;
            mutex.Unlock();
            // set up the timeout and normalize
            timeval tv = { 0, inTimeoutMilliseconds * 1000 };
            tv.tv_sec = tv.tv_usec / 1000000;
            tv.tv_usec %= 1000000;
            // create the FD set for select
            int nHighestFd = CreateFdSet();
            // call select
            int nSocketCount = select(nHighestFd + 1, &m_oReadFds, NULL, NULL, &tv);
            // handle select error
            if (nSocketCount ==
#ifdef _WIN32
                    SOCKET_ERROR
#else // NOT _WIN32
                    -1
#endif // _WIN32
               )
            {
#ifdef _WIN32
                throw runtime_error("EHSServer::HandleData: select() failed.");
#else // NOT _WIN32
                if (errno != EINTR) {
                    throw runtime_error("EHSServer::HandleData: select() failed.");
                }
#endif // _WIN32
            }
            // if no sockets have data to read, clear accepting flag and return
            if (nSocketCount > 0) {
                // Check the accept socket for a new connection
                CheckAcceptSocket();
                // check client sockets for data
                CheckClientSockets();
            }
            mutex.Lock();
            ClearIdleConnections();
            m_bAccepting = false;
        } // END ACCEPTING
    } // END NO REQUESTS PENDING
}

void EHSServer::CheckAcceptSocket ( )
{
    // see if we got data on this socket
    if (FD_ISSET(m_poNetworkAbstraction->GetFd(), &m_oReadFds)) {
        NetworkAbstraction *poNewClient = NULL;
        try {
            poNewClient = m_poNetworkAbstraction->Accept();
        } catch (runtime_error &e) {
            string emsg(e.what());
            // SSL handshake errors are acceptable.
            if (emsg.find("SSL") == string::npos) {
                throw;
            }
            cerr << emsg << endl;
            return;
        }
        // create a new EHSConnection object and initialize it
        EHSConnection * poEHSConnection = new EHSConnection ( poNewClient, this );
        if (m_poTopLevelEHS->m_oParams.find("maxrequestsize") !=
                m_poTopLevelEHS->m_oParams.end()) {
            unsigned long n = m_poTopLevelEHS->m_oParams["maxrequestsize"];
            EHS_TRACE("Setting connections MaxRequestSize to %lu\n", n);
            poEHSConnection->SetMaxRequestSize ( n );
        }
        {
            MutexHelper mutex(&m_oMutex);
            m_oEHSConnectionList.push_back(poEHSConnection);
            m_bAcceptedNewConnection = true;
        }
    } // end FD_ISSET ( )
}

void EHSServer::CheckClientSockets ( )
{
    // go through all the sockets from which we're still reading
    for (EHSConnectionList::iterator i = m_oEHSConnectionList.begin();
            i != m_oEHSConnectionList.end(); i++) {
        if (FD_ISSET((*i)->GetNetworkAbstraction()->GetFd(), &m_oReadFds)) {
            EHS_TRACE("$$$$$ Got data on client connection");
            // do the actual read
            char buf[8192];
            int nBytesReceived = (*i)->GetNetworkAbstraction()->Read(buf, sizeof(buf));
            // if we received a disconnect
            if (nBytesReceived <= 0) {
                // we're done reading and we received a disconnect
                (*i)->DoneReading(true);
            } else {
                // otherwise we got data
                // take the data we got and append to the connection's buffer
                EHSConnection::AddBufferResult nAddBufferResult =
                    (*i)->AddBuffer(buf, nBytesReceived);
                // if add buffer failed, don't read from this connection anymore
                switch (nAddBufferResult) {
                    case EHSConnection::ADDBUFFER_INVALIDREQUEST:
                    case EHSConnection::ADDBUFFER_TOOBIG:
                        {
                            // Immediately send a 400 response, then close the connection
                            auto_ptr<HttpResponse> tmp(HttpResponse::Error(HTTPRESPONSECODE_400_BADREQUEST, 0, *i));
                            (*i)->SendHttpResponse(tmp);
                            (*i)->DoneReading(false);
                            EHS_TRACE("Done reading because we got a bad request");
                        }
                        break;
                    case EHSConnection::ADDBUFFER_NORESOURCE:
                        {
                            // Immediately send a 500 response, then close the connection
                            auto_ptr<HttpResponse> tmp(HttpResponse::Error(HTTPRESPONSECODE_500_INTERNALSERVERERROR, 0, *i));
                            (*i)->SendHttpResponse(tmp);
                            (*i)->DoneReading(false);
                            EHS_TRACE("Done reading because we are out of ressources");
                        }
                        break;
                    default:
                        break;
                }
            } // end nBytesReceived
        } // FD_ISSET
    } // for loop through connections
}

void EHSConnection::AddResponse(HttpResponse *response)
{
    MutexHelper mutex(&m_oMutex);
    // push the object on to the list
    m_oHttpResponseMap[response->m_nResponseId] = response;
    // go through the list until we can't find the next response to send
    bool found;
    do {
        found = false;
        HttpResponseMap::iterator i = m_oHttpResponseMap.find(m_nResponses + 1);
        if (m_oHttpResponseMap.end() != i) {
            found = true;
            --m_nActiveRequests;
            SendHttpResponse(auto_ptr<HttpResponse>(i->second));
            m_oHttpResponseMap.erase(i);
            ++m_nResponses;
            // set last activity to the current time for idle purposes
            UpdateLastActivity();
            EHS_TRACE("Sending %d response(s) to %x", m_nResponses, this);
        }
    } while (found);
}

void EHSConnection::SendHttpResponse(auto_ptr<HttpResponse> response)
{
    // only send it if the client isn't disconnected
    if (Disconnected()) {
        return;
    }

    ostringstream oss;
    // add in the response code
    oss << "HTTP/1.1 " << response->m_nResponseCode
        << " " << HttpResponse::GetPhrase(response->m_nResponseCode) << "\r\n";

    // now go through all the entries in the responseheaders string map
    StringMap::iterator ith = response->m_oResponseHeaders.begin();
    while (ith != response->m_oResponseHeaders.end()) {
        oss << ith->first << ": " << ith->second << "\r\n";
        ith++;
    }

    // now push out all the cookies
    StringList::iterator itl = response->m_oCookieList.begin ( );
    while (itl != response->m_oCookieList.end()) {
        oss << "Set-Cookie: " << *itl << "\r\n";
        itl++;
    }

    // extra line break signalling end of headers
    oss << "\r\n";
    m_poNetworkAbstraction->Send(
            reinterpret_cast<const void *>(oss.str().c_str()), oss.str().length());

    // now send the body
    int blen = atoi(response->m_oResponseHeaders["content-length"].c_str());
    if (blen > 0) {
        EHS_TRACE("Sending %d bytes in thread %08x", blen, pthread_self());
        m_poNetworkAbstraction->Send(response->GetBody(), blen);
        EHS_TRACE("Done sending %d bytes in thread %08x", blen, pthread_self());
    }
}

void EHSServer::EndServerThread()
{
    pthread_mutex_lock(&m_oMutex);
    m_nServerRunningStatus = SERVERRUNNING_NOTRUNNING;
    m_nAcceptThreadId = 0;
    pthread_mutex_unlock(&m_oMutex);
    while (m_nThreads > 0) {
        EHS_TRACE ("Waiting for %d threads to terminate", m_nThreads);
        pthread_cond_broadcast(&m_oDoneAccepting);
        sleep(1);
    }
    EHS_TRACE ("all threads terminated");
}

EHS::EHS (EHS *ipoParent, string isRegisteredAs) :
    m_oEHSMap(EHSMap()),
    m_poParent(ipoParent),
    m_sRegisteredAs(isRegisteredAs),
    m_poEHSServer(NULL),
    m_poSourceEHS(NULL),
    m_poBindHelper(NULL),
    m_bNoRouting(false),
    m_oParams(EHSServerParameters())
{
    EHS_TRACE("TID=%p", pthread_self());
}

EHS::~EHS ()
{
    try {
        if (m_poParent) {
            // need to clean up all its registered interfaces
            m_poParent->UnregisterEHS(m_sRegisteredAs.c_str());
        }
    } catch (...) {
        delete m_poEHSServer;
        throw;
    }
    delete m_poEHSServer;
}

void EHS::RegisterEHS(EHS *ipoEHS, const char *ipsRegisterPath)
{
    ipoEHS->m_poParent = this;
    ipoEHS->m_sRegisteredAs = ipsRegisterPath;
    if (m_oEHSMap[ipsRegisterPath]) {
        throw runtime_error("EHS::RegisterEHS: Already registered");
    }
    m_oEHSMap[ipsRegisterPath] = ipoEHS;
}

void EHS::UnregisterEHS (const char *ipsRegisterPath)
{
    if (!m_oEHSMap[ipsRegisterPath]) {
        throw runtime_error("EHS::UnregisterEHS: Not registered");
    }
    m_oEHSMap.erase(ipsRegisterPath);
}

void EHS::HandleData(int inTimeoutMilliseconds)
{
    // make sure we're in a sane state
    if ((NULL == m_poParent) && (NULL == m_poEHSServer)) {
        throw runtime_error("EHS::HandleData: Invalid state");
    }

    if (m_poParent) {
        m_poParent->HandleData(inTimeoutMilliseconds);
    } else {
        // if we're in single threaded mode, handle data until there are no more jobs left
        if (m_poEHSServer->RunningStatus() == EHSServer::SERVERRUNNING_SINGLETHREADED) {
            do {
                m_poEHSServer->HandleData(inTimeoutMilliseconds, pthread_self());
            } while (m_poEHSServer->RequestsPending() ||
                    m_poEHSServer->AcceptedNewConnection());
        }
    }
}

string GetNextPathPart(string &irsUri)
{
    string ret;
    string newuri;
    pcrecpp::RE re("^/{0,1}([^/]+)/(.*)$");
    if (re.FullMatch(irsUri, &ret, &newuri)) {
        irsUri = newuri;
        return ret;
    }
    return string("");
}

auto_ptr<HttpResponse>
EHS::RouteRequest(HttpRequest *request ///< request info for service
        )
{
    // get the next path from the URI
    string sNextPathPart;
    if (!m_bNoRouting) {
        sNextPathPart = GetNextPathPart(request->m_sUri);
    }
    // We use an auto_ptr here, so that in case of an exception, the
    // target gets deleted.
    auto_ptr<HttpResponse> response(0);
    // if there is no more path, call HandleRequest on this EHS object with
    //   whatever's left - or if we're not routing
    if (m_bNoRouting || sNextPathPart.empty()) {
        // create an HttpRespose object for the client
        response = auto_ptr<HttpResponse>(new HttpResponse(request->m_nRequestId,
                    request->m_poSourceEHSConnection));
        // get the actual response and return code
        response->SetResponseCode(HandleRequest(request, response.get()));
    } else {
        EHS_TRACE ("Trying to route: '%s'", sNextPathPart.c_str());
        // if the path exists, check it against the map of EHSs
        if (m_oEHSMap[sNextPathPart]) {
            // if it exists, call its RouteRequest with the new shortened path
            response = m_oEHSMap[sNextPathPart]->RouteRequest(request);
        } else {
            // if it doesn't exist, send an error back up saying resource doesn't exist
            EHS_TRACE("Routing failed. Most likely caused by an invalid URL, not internal error");
            // send back a 404 response
            response = auto_ptr<HttpResponse>(HttpResponse::Error(HTTPRESPONSECODE_404_NOTFOUND, request));
        }
    }
    return response;
}

// default handle request returns time as text
ResponseCode EHS::HandleRequest(HttpRequest *request,
        HttpResponse * response)
{
    // if we have a source EHS specified, use it
    if (m_poSourceEHS != NULL) {
        return m_poSourceEHS->HandleRequest(request, response);
    }
    // otherwise, just send back the current time
    ostringstream oss;
    oss << time(NULL);
    response->SetBody(oss.str().c_str(), oss.str().length());
    response->SetHeader("Content-Type", "text/plain");
    return HTTPRESPONSECODE_200_OK;
}

void EHS::SetSourceEHS(EHS & iroSourceEHS)
{
    m_poSourceEHS = &iroSourceEHS;
}

HttpResponse *EHS::HandleThreadException(pthread_t tid, HttpRequest *, exception &ex)
{
    cerr << "Caught an exception in thread "
        << hex << tid << ": " << ex.what() << endl;
    return NULL;
}
