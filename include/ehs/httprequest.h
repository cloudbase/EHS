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

#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#ifdef _MSC_VER
# pragma warning(disable : 4786)
#endif

/// UNKNOWN must be the last one, as these must match up with RequestMethodStrings *exactly*
enum RequestMethod {
    REQUESTMETHOD_OPTIONS, /* not implemented */
    REQUESTMETHOD_GET,
    REQUESTMETHOD_HEAD,
    REQUESTMETHOD_POST,
    REQUESTMETHOD_PUT, ///< not implemented
    REQUESTMETHOD_DELETE, ///< not implemented
    REQUESTMETHOD_TRACE, ///< not implemented
    REQUESTMETHOD_CONNECT, ///< not implemented
    REQUESTMETHOD_UNKNOWN ///< used until we find the method
};

/**
 * This class represents a clients HTTP request.
 * It contans pre-parsed data like cookies, form data and
 * header information.
 */
class HttpRequest {

    private:

        /**
         * Constructs a ne instance.
         * @param inRequestId A unique Id (normally incremented by EHS).
         * @param ipoSourceEHSConnection The connection on which this request was received.
         * @param irsParseContentType Content type to parse form data for, empty string to always parse
         */
        HttpRequest (int inRequestId, EHSConnection *ipoSourceEHSConnection,
            const std::string & irsParseContentType);

    public:

        /// Destructor
        virtual ~HttpRequest ( );

        /**
         * Retrieves the peer's IP address.
         * @return The IP address of the peer that sent this request as dot-quad.
         */
        std::string RemoteAddress();

        /**
         * Retrieves the peer's port.
         * @return The peer port that sent this request.
         */
        int RemotePort();

        /**
         * Retrieves the local IP address.
         * @return The local IP address where this request was received as dot-quad.
         */
        std::string LocalAddress();

        /**
         * Retrieves the local port.
         * @return The local port where this request was received.
         */
        int LocalPort();

        DEPRECATED("Use RemoteAddress()")
            /**
             * Retrieves the peer's IP address.
             * @return The IP address of the peer that sent this request as dot-quad.
             * @deprecated Use RemoteAddress()
             */
            std::string Address() { return RemoteAddress(); }

        DEPRECATED("Use RemotePort()")
            /**
             * Retrieves the peer's port.
             * @return The peer port that sent this request.
             * @deprecated Use RemotePort()
             */
            int Port() { return RemotePort(); }

        /**
         * Retrieves this request's Id.
         * @return The unique Id that was generated by EHS.
         */
        int Id() const { return m_nRequestId; }

        /**
         * Retrieves the receiving connection.
         * @return The connection on which this request was received.
         */
        EHSConnection *Connection() const { return m_poSourceEHSConnection; }

        /**
         * Retrieves the request method.
         * @return The numeric request method of this request.
         */
        RequestMethod Method() const { return m_nRequestMethod; }

        /**
         * Retrieves the security status.
         * @return true if this request was received via SSL, false otherwise.
         */
        bool Secure() const { return m_bSecure; }

        /**
         * Retrieves the client connection status.
         * @return true if the client is disconnected.
         */
        bool ClientDisconnected();

        /**
         * Retrieves this request's URI
         * @return The request URI of this request.
         */
        const std::string &Uri() const { return m_sUri; }

        /**
         * Retrieves the HTTP version.
         * @return The HTTP version string as received in the request header.
         */
        const std::string &HttpVersion() const { return m_sHttpVersionNumber; }

        /**
         * Retrieves this request's body.
         * @return The body content of this request.
         */
        const std::string &Body() const { return m_sBody; }

        /**
         * Retrieves HTTP headers.
         * @return A StringCaseMap of the HTTP headers from this request.
         */
        StringCaseMap &Headers() { return m_oRequestHeaders; }

        /**
         * Retrieves form values.
         * @return All form values of this request.
         */
        FormValueMap &FormValues() { return m_oFormValueMap; }

        /**
         * Retrieves cookies.
         * @return All cookies of this request.
         */
        CookieMap &Cookies() { return m_oCookieMap; }

        /**
         * Retrieves a specific form value.
         * @param name The name of the form element to be retrieved.
         * @return The value of the specified form element.
         */
        FormValue &FormValues(const std::string & name)
        {
            return m_oFormValueMap[name];
        }

        /**
         * Retrieves a specific HTTP header.
         * @param name The name of the HTTP header to be retrieved.
         * @return The value of the specified header.
         */
        std::string Headers(const std::string & name)
        {
            if (m_oRequestHeaders.find(name) != m_oRequestHeaders.end()) {
                return m_oRequestHeaders[name];
            }
            return std::string();
        }

        /**
         * Sets a single request header.
         * This method is intended for generating synthetic headers (for
         * example when implementing HTTP basic authentication).
         * @param name The name of the HTTP header to be set.
         * @param value The value of the HTTP header to be set.
         */
        void SetHeader(const std::string & name, const std::string & value)
        {
            m_oRequestHeaders[name] = value;
        }

        /**
         * Retrieves a specific cookie value.
         * @param name The name of the cookie to be retrieved.
         * @return The value of the specified cookie.
         */
        std::string Cookies(const std::string & name)
        {
            if (m_oCookieMap.find(name) != m_oCookieMap.end()) {
                return m_oCookieMap[name];
            }
            return std::string();
        }

    private:

        /// Disable copy constructor
        HttpRequest(const HttpRequest &);

        /// Disable = operator
        HttpRequest & operator=(const HttpRequest &);

        /// Interprets the given string as if it's name=value pairs and puts them into oFormElements
        void GetFormDataFromString(const std::string &irsString);

        /// Enumeration for the state of the current HTTP parsing
        enum HttpParseStates {
            HTTPPARSESTATE_INVALID = 0,
            HTTPPARSESTATE_REQUEST,
            HTTPPARSESTATE_HEADERS,
            HTTPPARSESTATE_BODY,
            HTTPPARSESTATE_BODYCHUNK,
            HTTPPARSESTATE_BODYTRAILER,
            HTTPPARSESTATE_BODYTRAILER2,
            HTTPPARSESTATE_COMPLETEREQUEST,
            HTTPPARSESTATE_INVALIDREQUEST
        };

        /// Enumeration of error codes for ParseMultipartFormDataResult
        enum ParseMultipartFormDataResult { 
            PARSEMULTIPARTFORMDATA_INVALID = 0,
            PARSEMULTIPARTFORMDATA_SUCCESS,
            PARSEMULTIPARTFORMDATA_FAILED 
        };

        /// Enumeration of error codes for ParseSubbody
        enum ParseSubbodyResult {
            PARSESUBBODY_INVALID = 0,
            PARSESUBBODY_SUCCESS,
            PARSESUBBODY_INVALIDSUBBODY, // no blank line?
            PARSESUBBODY_FAILED // other reason
        };

        /// treats the body as if it's a multipart form data as specified in RFC not sure what the number is and I'll probably forget to look it up
        ParseMultipartFormDataResult ParseMultipartFormData();

        /**
         * Goes through a subbody and parses out elements.
         * @param sSubBody The string in which to look for subbody stuff
         * @return A ParseSubbodyResult reflecting the result.
         */
        ParseSubbodyResult ParseSubbody(std::string sSubBody);

        /// this function is given data that is read from the client and it deals with it
        HttpParseStates ParseData(std::string & irsData);

        /// takes the cookie header and breaks it down into usable chunks -- returns number of name/value pairs found
        int ParseCookieData (std::string & irsData);

        /// the current parse state -- where we are in looking at the data from the client
        HttpParseStates m_nCurrentHttpParseState;

        /// this is the request method from the client
        RequestMethod m_nRequestMethod;

        /// the clients requested URI
        std::string m_sUri;

        /// holds the original requested URI, not changed by any path routing
        std::string m_sOriginalUri;

        /// the HTTP version of the request
        std::string m_sHttpVersionNumber;

        /// Binary data, not NULL terminated
        std::string m_sBody; 

        /// Name of last HTTP header seen.
        std::string m_sLastHeaderName;

        /// whether or not this came over secure channels
        bool m_bSecure;

        /// headers from the client request
        StringCaseMap m_oRequestHeaders;

        /// Data specified by the client.  The 'name' field is mapped to a FormValue object which has the value and any metadata
        FormValueMap m_oFormValueMap;

        /// cookies that come in from the client
        CookieMap m_oCookieMap;

        /// request id for this connection
        int m_nRequestId;

        /// connection object from which this request came
        EHSConnection * m_poSourceEHSConnection;

        bool m_bChunked;

        size_t m_nChunkLen;

        /// content-type to parse form data for. if empty, always parse
        std::string m_sParseContentType;

        friend class EHSConnection;
        friend class EHS;
};


// GLOBAL HELPER FUNCTIONS

/**
 * Extracts the next line (separated by CRLF) from a buffer.
 * @param buffer The input buffer to work on. Upon return, the extracted
 *   line is removed from this buffer.
 * @return The extracted line (including the CRLF delimiter) or an empty
 * string if no more lines ar available.
 */
std::string GetNextLine(std::string & buffer);

/**
 * Retrieves the enum equivalent of a supplied HTTP request method.
 * @param method The HTTP request method to lookup
 * @return The corresponding numeric value, or REQUESTMETHOD_UNKNOW, if there was no match.
 */
RequestMethod GetRequestMethodFromString(const std::string & method);

/**
 * Classifies HTTP headers by multi-value definition.
 * @param header A header name to be classified.
 * @return true, if the header is defined in RFC to
 *   allow multiple comma-separated values.
 */
bool IsMultivalHeader(const std::string &header);

/**
 * Find a single value in a multi-value (comma separated) header.
 * @param header The comma separated multi-value header.
 * @param value The value to find
 * @return true, if value was found (case insensitive).
 */
bool MultivalHeaderContains(const std::string &header, const std::string &value);

#endif // HTTPREQUEST_H