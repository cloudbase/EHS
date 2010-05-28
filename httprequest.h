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
enum RequestMethod { REQUESTMETHOD_OPTIONS, /* not implemented */
    REQUESTMETHOD_GET,
    REQUESTMETHOD_HEAD,
    REQUESTMETHOD_POST,
    REQUESTMETHOD_PUT, ///< not implemented
    REQUESTMETHOD_DELETE, ///< not implemented
    REQUESTMETHOD_TRACE, ///< not implemented
    REQUESTMETHOD_CONNECT, ///< not implemented
    REQUESTMETHOD_LAST, ///< must be the last valid entry
    REQUESTMETHOD_UNKNOWN, ///< used until we find the method
    REQUESTMETHOD_INVALID ///< must be the last entry
};

/// this holds a list of strings corresponding to the order of the RequestMethod enumeration
extern const char * RequestMethodStrings [];

/// This is what the client sent in a more organized form
/**
 * This is what the client sent in a more organized form.  It has any form
 * data,  header information, the request type
 */
class HttpRequest {

    private:

        HttpRequest ( const HttpRequest & );

        HttpRequest & operator = ( const HttpRequest & );

    public:

        /// constructor
        HttpRequest ( int inRequestId,
                EHSConnection * ipoSourceEHSConnection );

        /// destructor
        virtual ~HttpRequest ( );

        /// Enumeration for the state of the current HTTP parsing
        enum HttpParseStates { HTTPPARSESTATE_INVALID = 0,
            HTTPPARSESTATE_REQUEST,
            HTTPPARSESTATE_HEADERS,
            HTTPPARSESTATE_BODY,
            HTTPPARSESTATE_COMPLETEREQUEST,
            HTTPPARSESTATE_INVALIDREQUEST,
        };

        /// Enumeration of error codes for ParseMultipartFormDataResult
        enum ParseMultipartFormDataResult { 
            PARSEMULTIPARTFORMDATA_INVALID = 0,
            PARSEMULTIPARTFORMDATA_SUCCESS,
            PARSEMULTIPARTFORMDATA_FAILED 
        };

        /// treats the body as if it's a multipart form data as specified in RFC not sure what the number is and I'll probably forget to look it up
        ParseMultipartFormDataResult ParseMultipartFormData ( );

        /// Enumeration of error codes for ParseSubbody
        enum ParseSubbodyResult {
            PARSESUBBODY_INVALID = 0,
            PARSESUBBODY_SUCCESS,
            PARSESUBBODY_INVALIDSUBBODY, // no blank line?
            PARSESUBBODY_FAILED // other reason

        };

        /// goes through a subbody and parses out elements
        ParseSubbodyResult ParseSubbody ( std::string sSubBody );

        /// this function is given data that is read from the client and it deals with it
        HttpParseStates ParseData ( std::string & irsData );

        /// takes the cookie header and breaks it down into usable chunks -- returns number of name/value pairs found
        int ParseCookieData ( std::string & irsData );

        /// interprets the given string as if it's name=value pairs and puts them into oFormElements
        void GetFormDataFromString ( const std::string & irsString );

        /// returns the address this request came from
        std::string GetAddress ( );

        /// returns the port this request came from
        int GetPort ( );

        /// returns this request's method
        int Method ( ) { return m_nRequestMethod; }

        /// returns true if this request was received via HTTPS
        bool Secure ( ) { return m_bSecure; }

        /// returns true if the client is disconnected
        bool ClientDisconnected ( );

        /// returns this request's URI
        const std::string & Uri ( ) { return m_sUri; }

        /// returns this request's HTTP version
        const std::string & HttpVersion ( ) { return m_sHttpVersionNumber; }

        /// returns this request's body
        const std::string & Body ( ) { return m_sBody; }

        /// retrieves the request headers of this instance.
        StringMap & Headers ( ) { return m_oRequestHeaders; }

        /// retrieves the form value map of this instance.
        FormValueMap  & FormValues ( ) { return m_oFormValueMap; }

        /// retrieves the cookie map of this instance.
        CookieMap  & Cookies ( ) { return m_oCookieMap; }

        /// retrieves a single form value of this instance.
        FormValue & FormValues ( const std::string & name )
        {
            return m_oFormValueMap[ name ];
        }

        /// retrieves a single request header of this instance.
        std::string Headers ( const std::string & name )
        {
            return m_oRequestHeaders[ name ];
        }

        /// sets a single request header of this instance.
        void SetHeader ( const std::string & name , const std::string & value )
        {
            m_oRequestHeaders[ name ] = value;
        }

        /// retrieves a single cookie value of this instance.
        std::string Cookies ( const std::string & name )
        {
            return m_oCookieMap[ name ];
        }

    private:

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

        /// whether or not this came over secure channels
        int m_bSecure;

        /// headers from the client request
        StringMap m_oRequestHeaders;

        /// Data specified by the client.  The 'name' field is mapped to a FormValue object which has the value and any metadata
        FormValueMap m_oFormValueMap;

        /// cookies that come in from the client
        CookieMap m_oCookieMap;

        /// request id for this connection
        int m_nRequestId;

        /// connection object from which this request came
        EHSConnection * m_poSourceEHSConnection;

        friend class EHSConnection;
        friend class EHS;
};


// GLOBAL HELPER FUNCTIONS

/// removes the next line from irsBuffer and returns it in irsLine
int GetNextLine ( std::string & irsLine, std::string & irsBuffer );

/// gets the RequestMethod enumeration based on isRequestMethod
RequestMethod GetRequestMethodFromString ( const std::string & isRequestMethod );

/// makes the string lowercase
std::string & mytolower (std::string & s );

#endif // HTTPREQUEST_H
