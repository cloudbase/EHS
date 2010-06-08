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

#ifndef EHS_H
#define EHS_H

// Pragma'ing away nasty MS 255-char-name problem.  Otherwise
// you will get warnings on many template names that
//	"identifier was truncated to '255' characters in the debug information".
#ifdef _MSC_VER
# pragma warning(disable : 4786)
#endif

// EHS headers

#include <formvalue.h>
#include <ehstypes.h>
#include <datum.h>
#include <httpresponse.h>
#include <httprequest.h>

#include <memory>
#include <string>

extern "C" {
    const char * getEHSconfig();
}

/**
 * Helper class for binding of sockets to privileged ports.
 * This class abstracts an interface to an external bind helper
 * program which facilitates binding of privileged ports (< 1024).
 * Instead of doing it the apache way (do the bind() while running as <b>root</b>
 * and <b>then</b> dropping privileges), we run as <b>unprivileged</b> user <b>in the
 * first place</b> and later use that helper (an external setuid program) to temporarily
 * elevate privileges for the bind() call. IMHO, this is safer, because that helper is
 * <b>very</b> simple and does exactly <b>one</b> task: binding. <b>Nothing</b> else.
 */
class PrivilegedBindHelper {

    public:

        /**
         * Binds a socket to a privileged port/address.
         * @param socket The socket to be bound.
         * @param addr The address to bind to in dot-qad format.
         * @param port The port number to bind do.
         * @return true on success.
         */
        virtual bool BindPrivilegedPort(int socket, const char *addr, const unsigned short port) = 0;

        virtual ~PrivilegedBindHelper ( ) { }

};

/**
 * This interface describes a handler for retrieving passphrases.
 * When using SSL, this interface is used to fetch passphrases for
 * the server certificates. EHS implements a simple default handler
 * which provides the passphrase that has been provided via the
 * server parameters.
 */
class PassphraseHandler {

    public:

        /**
         * Retrieves a passphrase.
         * @param twice If true, this passphrase should be requested twice
         *   in order to prevent typing errors.
         * @return A std::string holding the passphrase.
         */
        virtual const std::string GetPassphrase(bool twice) = 0;

        /// Destructor
        virtual ~PassphraseHandler ( ) { }
};

/// this is to protect from people being malicious or really stupid
#define MAX_REQUEST_SIZE_DEFAULT (256 * 1024)

/**
 * EHS provides HTTP server functionality to a child class.
 * The child class must inherit from it and then override the
 * HandleRequest method.
 */
class EHS : public PassphraseHandler {

    private:
        /**
         * Disabled copy constructor.
         */
        EHS(const EHS &);

        /**
         * Disabled assignment operator.
         */
        EHS & operator=(const EHS &);

    protected:

        /// Stores path => EHSConnection pairs for path/tree traversal
        EHSMap m_oEHSMap;

        /// Points to the EHS object this object was registered with, NULL if top level
        EHS *m_poParent; 

        /// The string that this EHS object is regestered as
        std::string m_sRegisteredAs;

        /// EHSServer object associated with this EHS object
        EHSServer *m_poEHSServer;

        /// Source EHS object to route requests to for data instead of processing it ourselves
        EHS *m_poSourceEHS;

        /// Our bind helper
        PrivilegedBindHelper *m_poBindHelper;

    public:

        /**
         * Constructs a new instance as child of another EHS instance.
         * @param parent The parent EHS instance.
         * @param registerpath The URI path which the new instance should handle.
         */
        EHS(EHS *parent = NULL, std::string registerpath = "");

        /**
         * Destructor
         */
        virtual ~EHS ();

        /**
         * Default PassphraseHandler implementation. Takes passphrase from
         * server parameters without prompting.
         * @param twice If true, an interactive implementation should ask
         *   twice for a passphrase in order to eliminate typing errors.
         * @return A std::string holding the passphrase.
         */
        virtual const std::string GetPassphrase(bool twice);

        /// Enumeration for error results for RegisterEHSResult
        enum RegisterEHSResult {
            REGISTEREHSINTERFACE_INVALID = 0,
            REGISTEREHSINTERFACE_ALREADYEXISTS,
            REGISTEREHSINTERFACE_SUCCESS
        };

        /// Enumeration for error results for UnregisterEHSResult
        enum UnregisterEHSResult {
            UNREGISTEREHSINTERFACE_INVALID = 0,
            UNREGISTEREHSINTERFACE_NOTREGISTERED,
            UNREGISTEREHSINTERFACE_SUCCESS
        };

        /**
         * Instructs this EHS instance to invoke HandleRequest of another EHS instance
         * whenever a specific URI is requested.
         * @param child The foreign EHS instance which shall handle the requests.
         * @param uripath The relative URI path, that shall be handled by the child.
         * @return RegisterEHSResult reflecting the outcome.
         **/
        RegisterEHSResult RegisterEHS(EHS *child, const char *uripath);

        /**
         * Removes a previously registered path registration.
         * @param uripath The URI path to be removed.
         * @return UnregisterEHSResult reflecting the outcome.
         */
        UnregisterEHSResult UnregisterEHS (char *uripath);

        /**
         * Routes a request to the appropriate instance.
         * @param request The HTTP request to be routed.
         * @return A pointer to the HttpResponse object, to be sent back to the client.
         */
        std::auto_ptr<HttpResponse> RouteRequest(HttpRequest *request);

        /**
         * Main request handler.
         * Reimplement this method in a derived class in order to implement the
         * actual functionality.
         * @param request Pointer to the HTTP request that triggered invocation of this method.
         * @param response Pointer to the HTTP response that is going to be sent to the client.
         *   In derived methods, populate the HttpResponse with your data.
         * @returns The HTTP response code to be sent to the client.
         */
        virtual ResponseCode HandleRequest(HttpRequest *request, HttpResponse *response);

        /**
         * Establishes this EHS instance as request handler of another EHS instance.
         * Any HTTP request received by the other EHS instance will be handled by this
         * instance's HandleRequest method.
         * @param source A reference to the source EHS instance.
         */
        void SetSourceEHS (EHS & source);

        /**
         * Result codes for StartServer
         */
        enum StartServerResult {
            STARTSERVER_INVALID = 0,
            STARTSERVER_SUCCESS,
            STARTSERVER_NODATASPECIFIED,
            STARTSERVER_ALREADYRUNNING,
            STARTSERVER_SOCKETSNOTINITIALIZED,
            STARTSERVER_THREADCREATIONFAILED,
            STARTSERVER_FAILED
        };

        /// Stores a map with server parameters
        EHSServerParameters m_oEHSServerParameters;

        /**
         * Starts up this instance. If configured to run in threaded mode,
         * the main listener thread as well as all worker threads are started.
         * @param params An EHSServerParameters map used for configuring this instance.
         * @return A StartServerResult reflecting the outcome.
         */
        StartServerResult StartServer(EHSServerParameters &params);

        /**
         * Shuts down this instance. If running in threaded mode, all associated threads
         * are stopped. Stopping threads is a blocking operation. This method returns,
         * if all relevant threads are terminated.
         */
        void StopServer();

        /**
         * Dispatches incoming data.
         */ 
        void HandleData(int timeout = 0);

        /**
         * Hook for thread startup.
         * Called at the start of a thread routine.
         * Reimplement this method in a derived class, if you need to
         * perform any task (like setting up thread-local storage) at
         * the start of a thread.
         * @return true, if everything is OK, false otherwise.
         */
        virtual bool ThreadInitHandler();

        /**
         * Hook for thread shutdown.
         * Called at exit of a thread routine.
         * Reimplement this method in a derived class, if you need to
         * perform any task (like some cleanup) just before the thread
         * is terminating.
         */
        virtual void ThreadExitHandler();

        /**
         * Hook for handling exceptions in threads.
         * Called, when an exception was thrown from within a thread.
         * Reimplement this method in a derived class, if you want to handle your
         * own exceptions. The default implementation prints the exception's
         * message on cerr and returns NULL. If returning NULL, you still should
         * invoke StopServer in order to terminate the server's remaining threads.
         * @param tid The thread ID of the thread that has thrown the exception.
         * @param request The current HTTP request that was active wile the exception was thrown.
         * @param ex The exception that was thrown. Any C-Style exceptions are
         *   thrown as runtime_exception("unspecified").
         * @return NULL if the thread should exit; an appropriate
         *   HTTP response, if the thread should continue.
         */
        virtual HttpResponse *HandleThreadException(pthread_t tid, HttpRequest *request, std::exception & ex);

        /**
         * Retrieve the server's exception status.
         * @return true if the server should terminate.
         */
        bool ShouldTerminate() const;

        /**
         * Sets a PrivilegedBindHelper for use by the network abstraction layer.
         * @param helper A pointer to a PrivilegedBindHelper instance, implementing
         *   the bind operation to a privileged port. Without this helper, an instance
         *   cannot bind to privileged ports (ports < 1024) unless running as root.
         *   The BindPrivilegedPort method will only be called for ports < 1024.
         */
        void SetBindHelper(PrivilegedBindHelper *helper)
        {
            m_poBindHelper = helper;
        }

        /**
         * Retieves our PrivilegedBindHelper.
         * @return The current helper, or NULL if no helper was set.
         */
        PrivilegedBindHelper * GetBindHelper() const
        {
            return m_poBindHelper;
        }

};

#endif // EHS_H
