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

#include "ehs.h"
#include "ehsconnection.h"

#include <pcrecpp.h>
#include <string>
#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace std;

void HttpRequest::GetFormDataFromString ( const string & irsString ///< string to parse for form data
        )
{
    pcrecpp::RE re("[?]?([^?=]*)=([^&]*)&?");
    string name;
    string value;
    pcrecpp::StringPiece input(irsString);
    while ( re.FindAndConsume ( &input, &name, &value ) ) {
#ifdef EHS_DEBUG
        cerr << "[EHS_DEBUG] Info: Got form data: '" << name << "' => '" << value << "'" << endl;
#endif
        ContentDisposition oContentDisposition;
        m_oFormValueMap [ name ] =
            FormValue ( value, oContentDisposition );
    }
}

// this parses a single piece of a multipart form body
// here are two examples -- first is an uploaded file, second is a
//   standard text box html form
/*
   -----------------------------7d2248207500d4
   Content-Disposition: form-data; name="DFWFilename2"; filename="C:\Documents and Settings\administrator\Desktop\contacts.dat"
   Content-Type: application/octet-stream

   Default Screen Name
   -----------------------------7d2248207500d4


   -----------------------------7d2248207500d4
   Content-Disposition: form-data; name="foo"

   asdfasdf
   -----------------------------7d2248207500d4
   */


HttpRequest::ParseSubbodyResult HttpRequest::ParseSubbody ( string isSubbody ///< string in which to look for subbody stuff
        )
{
    // find the spot after the headers in the body
    string::size_type nBlankLinePosition = isSubbody.find ( "\r\n\r\n" );

    // if there's no double blank line, then this isn't a valid subbody
    if ( nBlankLinePosition == string::npos ) {

#ifdef EHS_DEBUG
        cerr << "[EHS_DEUBG] Invalix subbody, couldn't find double blank line" << endl;
#endif
        return PARSESUBBODY_INVALIDSUBBODY;

    }

    // create a string from the beginning to the blank line -- OFF BY ONE?
    string sHeaders ( isSubbody, 0, nBlankLinePosition - 1 );

    // first line MUST be the content-disposition header line, so that
    //   we know what the name of the field is.. otherwise, we're in trouble
    string sContentDisposition;
    string sNameValuePairs;
    pcrecpp::RE re ( "Content-Disposition:[ ]?([^;]+);[ ]?(.*)" );
    if ( re.PartialMatch( sHeaders, &sContentDisposition, &sNameValuePairs ) ) {

        StringMap oStringMap;
        string sName;
        string sValue;
        pcrecpp::StringPiece nvp( sNameValuePairs );
        pcrecpp::RE nvre ( "[ ]?([^= ]+)=\"([^\"]+)\"[;]?" );

        while ( nvre.FindAndConsume( &nvp, &sName, &sValue ) )
        {
#ifdef EHS_DEBUG
            cerr
                << "[EHS_DEBUG] Info: Subbody header found: '"
                << sName << "' => '" << sValue << "'" << endl;
#endif				
            oStringMap [ sName ] = sValue;

        }

        // take oStringMap and actually fill the right object with its data
        FormValue & roFormValue = m_oFormValueMap [ oStringMap [ "name" ] ];

        // copy the headers in now that we know the name
        roFormValue.m_oContentDisposition.m_oContentDispositionHeaders = oStringMap;

        // grab out the body
        roFormValue.m_sBody = isSubbody.substr ( nBlankLinePosition + 4);

#ifdef EHS_DEBUG		
        // cerr << "[EHS_DEBUG] Info: Subbody body (in binary):" << endl
        // << "---" << endl << roFormValue.sBody << endl << "---" << endl;
#endif

    } else {
        // couldn't find content-disposition line -- FATAL ERROR

#ifdef EHS_DEBUG
        cerr << "[EHS_DEBUG] Error: Couldn't find content-disposition line" << endl;
#endif
        return PARSESUBBODY_INVALIDSUBBODY;
    }

    return PARSESUBBODY_SUCCESS;

}

    HttpRequest::ParseMultipartFormDataResult
HttpRequest::ParseMultipartFormData ( )
{
    if ( m_oRequestHeaders [ "Content-Type" ].empty ( ) ) {
        throw runtime_error ( "HttpRequest::ParseMultipartFormData: No Content-Type" );
    }

    // find the boundary string
    pcrecpp::RE re ( "multipart/[^;]+;[ ]*boundary=([^\"]+)$" );

#ifdef EHS_DEBUG
    cerr << "looking for boundary in '" << m_oRequestHeaders [ "Content-Type" ] << "'" << endl;
#endif	

    // if ( ( nMatchResult = oMultipartFormDataContentTypeValueRegex.match ( oRequestHeaders [ "content-type" ] ) ) )
    string sBoundaryString;
    if ( re.FullMatch( m_oRequestHeaders [ "Content-Type" ], &sBoundaryString ) )
    {
        // the actual boundary has two dashes prepended to it
        string sActualBoundary = string ("--") + sBoundaryString;

#ifdef EHS_DEBUG
        cerr
            << "[EHS_DEBUG] Info: Found boundary of '" << sBoundaryString << "'" << endl
            << "[EHS_DEBUG] Info: Looking for boundary to match (" << m_sBody.length ( )
            << ") '" << m_sBody.substr ( 0, sActualBoundary.length ( ) ) << "'" << endl;
#endif

        // check to make sure we started at the boundary
        if ( m_sBody.substr ( 0, sActualBoundary.length ( ) ) != sActualBoundary ) {
#ifdef EHS_DEBUG
            cerr << "[EHS_DEBUG] Error: Misparsed multi-part form data for unknown reason - first bytes weren't the boundary string" << endl;
#endif			
            return PARSEMULTIPARTFORMDATA_FAILED;
        }

        // go past the initial boundary
        string sRemainingBody = m_sBody.substr ( sActualBoundary.length ( ) );

        // while we're at a boundary after we grab a part, keep going
        string::size_type nNextPartPosition;
        while ( ( nNextPartPosition = sRemainingBody.find ( string ( "\r\n" ) + sActualBoundary ) ) != 
                string::npos ) {

#ifdef EHS_DEBUG
            cerr << "[EHS_DEBUG] Info: Found subbody from pos 0 to " << nNextPartPosition << endl;
#endif
            if ( sRemainingBody.length ( ) >= sActualBoundary.length ( ) ) {
                throw runtime_error ( "HttpRequest::ParseMultipartFormData: Subpart body exceeds main body length" );
            }
            ParseSubbody ( sRemainingBody.substr ( 0, nNextPartPosition ) );
            // skip past the boundary at the end and look for the next one
            nNextPartPosition += sActualBoundary.length ( );
            sRemainingBody = sRemainingBody.substr ( nNextPartPosition );
        }
    } else {
#ifdef EHS_DEBUG
        cerr << "[EHS_DEBUG] Error: Couldn't find boundary specification in content-type header" << endl;
#endif
        return PARSEMULTIPARTFORMDATA_FAILED;
    }

    return PARSEMULTIPARTFORMDATA_SUCCESS;

}

// A cookie header looks like: Cookie: username=xaxxon; password=comoesta
//   everything after the : gets passed in in irsData
int HttpRequest::ParseCookieData ( string & irsData )
{
#ifdef EHS_DEBUG
    cerr << "looking for cookies in '" << irsData << "'" << endl;
#endif
    int ccount = 0;

    pcrecpp::RE re ( "\\s*([^=]+)=([^;]+)(;|$)*" );
    pcrecpp::StringPiece input ( irsData );
    string name;
    string value;
    while ( re.FindAndConsume ( &input, &name, &value ) ) {
        m_oCookieMap [ name ] = value;
        ccount++;
    }
    return ccount;
}

// takes data and tries to figure out what it is.  it will loop through 
//   irsData as many times as it can as long as it gets a full line each time.
//   Depending on how much data it's gotten already, it will handle a line 
//   differently.. 
HttpRequest::HttpParseStates HttpRequest::ParseData ( string & irsData ///< buffer to look in for more data
        )
{
    string sLine;
    string sName;
    string sValue;
    int nDoneWithCurrentData = 0;
    pcrecpp::RE reHeader ( "^([^:]*):\\s+(.*)\\r\\n$" );

    while ( ! nDoneWithCurrentData && 
            m_nCurrentHttpParseState != HTTPPARSESTATE_INVALIDREQUEST &&
            m_nCurrentHttpParseState != HTTPPARSESTATE_COMPLETEREQUEST &&
            m_nCurrentHttpParseState != HTTPPARSESTATE_INVALID ) {

        switch ( m_nCurrentHttpParseState ) {

            case HTTPPARSESTATE_REQUEST:

                // get the request line
                GetNextLine ( sLine, irsData );

                // if we got a line, parse out the data..
                if ( sLine.length ( ) == 0 ) {
                    nDoneWithCurrentData = 1;
                } else {
                    // if we got a line, look for a request line

                    // everything must be uppercase according to rfc2616
                    pcrecpp::RE_Options opt(PCRE_DOTALL);
                    pcrecpp::RE re ( "^(OPTIONS|GET|HEAD|POST|PUT|DELETE|TRACE|CONNECT) ([^ ]+) HTTP/(\\d+\\.\\d+)\\r\\n$" , opt);
                    string muri;
                    string mver;
                    string mreq;
                    if ( re.FullMatch ( sLine, &mreq, &muri, &mver ) ) {

                        // get the info from the request line
                        m_nRequestMethod = GetRequestMethodFromString ( mreq );
                        m_sUri = muri;
                        m_sOriginalUri = muri;
                        m_sHttpVersionNumber = mver;

                        // check to see if the uri appeared to have form data in it
                        GetFormDataFromString ( m_sUri );

                        // on to the headers
                        m_nCurrentHttpParseState = HTTPPARSESTATE_HEADERS;

                    } else {
                        // if the regex failed
                        m_nCurrentHttpParseState = HTTPPARSESTATE_INVALIDREQUEST;

                    } // end match on request line

                } // end whether we got a line

                break;

            case HTTPPARSESTATE_HEADERS:

                // get the next line
                GetNextLine ( sLine, irsData );

                // if we didn't get a full line of data
                if ( sLine.length ( ) == 0 ) {
                    nDoneWithCurrentData = 1;

                } else if ( sLine == "\r\n" ) {
                    // check to see if we're done with headers

                    // if content length is found
                    if ( m_oRequestHeaders.find ( "Content-Length" ) !=
                            m_oRequestHeaders.end ( ) ) {
                        m_nCurrentHttpParseState = HTTPPARSESTATE_BODY;
                    } else {
                        m_nCurrentHttpParseState = HTTPPARSESTATE_COMPLETEREQUEST;
                    }

                    // if this is an HTTP/1.1 request, then it had better have a Host: header
                    if ( m_sHttpVersionNumber == "1.1" && 
                            m_oRequestHeaders [ "Host" ].length ( ) == 0 ) {
                        m_nCurrentHttpParseState = HTTPPARSESTATE_INVALIDREQUEST;
                    }

                    ParseCookieData ( m_oRequestHeaders [ "cookie" ] );

                } else if ( reHeader.FullMatch ( sLine, &sName, &sValue ) ) {
                    // else if there is still data

                    if ( sName == "Transfer-Encoding" &&
                            sValue == "chunked" ) {
                        // TODO: Implement chunked encoding    
                        cerr << "EHS DOES NOT SUPPORT CHUNKED ENCODING" << endl;
                        m_nCurrentHttpParseState = HTTPPARSESTATE_INVALIDREQUEST;
                    }
                    m_oRequestHeaders [ sName ] = sValue;
                } else {
                    // else we had some sort of error -- bail out

#ifdef EHS_DEBUG
                    cerr << "[EHS_DEBUG] Error: Invalid header line: '" << sLine << "'" << endl;
#endif
                    m_nCurrentHttpParseState = HTTPPARSESTATE_INVALIDREQUEST;
                    nDoneWithCurrentData = 1;
                }
                break;


            case HTTPPARSESTATE_BODY:
                {

                    // if a content length wasn't specified, we can't be here (we 
                    //   don't know chunked encoding)
                    if ( m_oRequestHeaders.find ( "Content-Length" ) == 
                            m_oRequestHeaders.end ( ) ) {

                        m_nCurrentHttpParseState = HTTPPARSESTATE_INVALIDREQUEST;
                        continue;

                    }

                    // get the content length
                    unsigned int nContentLength = 
                        atoi ( m_oRequestHeaders [ "Content-Length" ].c_str ( ) );

                    // else if we haven't gotten all the data we're looking for,
                    //   just hold off and try again when we get more
                    if ( irsData.length ( ) < nContentLength ) {
#ifdef EHS_DEBUG
                        cerr
                            << "[EHS_DEBUG] Info: Not enough data yet -- "
                            << irsData.length ( ) << " < " << nContentLength << endl;
#endif
                        nDoneWithCurrentData = 1;
                    } else {
                        // otherwise, we've gotten enough data from the client, handle it now

                        // grab out the actual body from the request and leave the rest
                        m_sBody = irsData.substr ( 0, nContentLength );
                        irsData = irsData.substr ( nContentLength );

                        // if we're dealing with multi-part form attachments
                        if ( m_oRequestHeaders [ "Content-Type" ].substr ( 0, 9 ) == 
                                "multipart" ) {
                            // handle the body as if it's multipart form data
                            if ( ParseMultipartFormData ( ) == 
                                    PARSEMULTIPARTFORMDATA_SUCCESS ) {
                                m_nCurrentHttpParseState = HTTPPARSESTATE_COMPLETEREQUEST;
                            } else {
#ifdef EHS_DEBUG						
                                cerr << "[EHS_DEBUG] Error: Mishandled multi-part form data for unknown reason" << endl;
#endif
                                m_nCurrentHttpParseState = HTTPPARSESTATE_INVALIDREQUEST;
                            }
                        } else {
                            // else the body is just one piece

                            // check for any form data
                            GetFormDataFromString ( m_sBody );
#ifdef EHS_DEBUG
                            cerr << "Done with body, done with entire request" << endl;
#endif
                            m_nCurrentHttpParseState = HTTPPARSESTATE_COMPLETEREQUEST;
                        }

                    }
                    nDoneWithCurrentData = 1;
                }
                break;


            case HTTPPARSESTATE_INVALID:
            default:
#ifdef EHS_DEBUG
                cerr << "[EHS_DEBUG] Critical error: Invalid internal state: " << m_nCurrentHttpParseState << endl;
#endif
                throw runtime_error ( "HttpRequest::ParseData: Invalid parsing state" );
                break;
        }
    }
    return m_nCurrentHttpParseState;

}

HttpRequest::HttpRequest ( int inRequestId,
        EHSConnection * ipoSourceEHSConnection ) :
    m_nCurrentHttpParseState  ( HTTPPARSESTATE_REQUEST ),
    m_nRequestMethod ( REQUESTMETHOD_UNKNOWN ),
    m_sUri ( "" ),
    m_sOriginalUri ( "" ),
    m_sHttpVersionNumber ( "" ),
    m_sBody ( "" ),
    m_bSecure ( false ),
    m_oRequestHeaders ( StringCaseMap ( ) ),
    m_oFormValueMap ( FormValueMap ( ) ),
    m_oCookieMap ( CookieMap ( ) ),
    m_nRequestId ( inRequestId ),
    m_poSourceEHSConnection ( ipoSourceEHSConnection )
{
#ifdef EHS_MEMORY
    cerr << "[EHS_MEMORY] Allocated: HttpRequest" << endl;
#endif
    if ( m_poSourceEHSConnection == NULL ) {
#ifdef EHS_DEBUG
        cerr << "Not allowed to have null source connection" << endl;
#endif
        throw runtime_error("Not allowed to have null source connection");
    }
}

HttpRequest::~HttpRequest ( )
{
#ifdef EHS_MEMORY
    cerr << "[EHS_MEMORY] Deallocated: HttpRequest" << endl;
#endif		
}


// HELPER FUNCTIONS

// Takes a char* buffer and grabs a line off it, puts the new line in irsLine
//   and shrinks the buffer by the size of the line and sets ipnBufferLength to 
//   the new size

void GetNextLine ( string & irsLine, ///< line removed from *ippsBuffer
        string & irsBuffer ///< buffer from which to remove a line
        )
{
    pcrecpp::RE_Options opt(PCRE_MULTILINE|PCRE_DOTALL);
    pcrecpp::RE re( "^([^\\r]*\\r\\n)(.*)$", opt );
    string mbuf;
    if ( re.FullMatch ( irsBuffer, &irsLine, &mbuf ) ) {
        irsBuffer = mbuf;
    } else {
        irsLine.clear();
    }
}

/// List of possible HTTP request methods
static const char * RequestMethodStrings [] = {
    "OPTIONS", "GET", "HEAD", "POST", 
    "PUT", "DELETE", "TRACE", "CONNECT", "*"
};

RequestMethod GetRequestMethodFromString ( const string & isRequestMethod  ///< determine the request type enumeration from the request string
        )
{
    int i = 0;
    for ( i = 0; i < REQUESTMETHOD_LAST; i++ ) {
        if ( isRequestMethod == RequestMethodStrings [ i ] ) {
            break;
        }
    }
    return ( RequestMethod ) i;
}

string HttpRequest::Address()
{
    return m_poSourceEHSConnection->GetAddress();
}

int HttpRequest::Port()
{
    return m_poSourceEHSConnection->GetPort();
}


bool HttpRequest::ClientDisconnected()
{ 
    return m_poSourceEHSConnection->Disconnected(); 
}
