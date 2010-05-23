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

#include <pme.h>
#include <cassert>
#include <iostream>

using namespace std;

void HttpRequest::GetFormDataFromString ( const string & irsString ///< string to parse for form data
        )
{
    PME oNameValueRegex ( "[?]?([^?=]*)=([^&]*)&?", "g" );
    while ( oNameValueRegex.match ( irsString ) ) {
        ContentDisposition oContentDisposition;
        string sName = oNameValueRegex [ 1 ];
        string sValue = oNameValueRegex [ 2 ];
#ifdef EHS_DEBUG
        cerr << "[EHS_DEBUG] Info: Got form data: '" << sName << "' => '" << sValue << "'" << endl;
#endif
        oFormValueMap [ sName ] =
            FormValue ( sValue, oContentDisposition );
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
    int nMatchResult = 0;
    PME oContentDispositionRegex ( "Content-Disposition:[ ]?([^;]+);[ ]?(.*)" );
    nMatchResult = oContentDispositionRegex.match ( sHeaders );
    if ( nMatchResult == 3 ) {

        string sContentDisposition ( oContentDispositionRegex [ 1 ] );
        string sNameValuePairs ( oContentDispositionRegex [ 2 ] );
        StringMap oStringMap;
        PME oContentDispositionNameValueRegex ( "[ ]?([^= ]+)=\"([^\"]+)\"[;]?", "g" );

        // go through the sNameValuePairs string and grab out the pieces
        nMatchResult = 3;

        while ( nMatchResult == 3 ) {

            nMatchResult = oContentDispositionNameValueRegex.match ( sNameValuePairs );
            if ( nMatchResult == 3 ) {

                string sName = oContentDispositionNameValueRegex [ 1 ];
                string sValue = oContentDispositionNameValueRegex [ 2 ];

#ifdef EHS_DEBUG
                cerr
                    << "[EHS_DEBUG] Info: Subbody header found: '"
                    << sName << "' => '" << sValue << "' with "
                    << nMatchResult << " matches" << endl;
#endif				

                oStringMap [ sName ] = sValue;

            } else if ( nMatchResult == 0 ) {
                ; // this is okay -- just done with name value pairs
            } else {
                assert ( 0 );				
            }
        }

        // take oStringMap and actually fill the right object with its data
        FormValue & roFormValue = oFormValueMap [ oStringMap [ "name" ] ];

        // copy the headers in now that we know the name
        roFormValue.oContentDisposition.oContentDispositionHeaders = oStringMap;

        // grab out the body
        roFormValue.sBody = isSubbody.substr ( nBlankLinePosition + 4);

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
    assert ( !oRequestHeaders [ "content-type" ].empty ( ) );

    // find the boundary string
    int nMatchResult = 0;

    PME oMultipartFormDataContentTypeValueRegex ( "multipart/[^;]+;[ ]*boundary=([^\"]+)$" );

#ifdef EHS_DEBUG
    cerr << "looking for boundary in '" << oRequestHeaders [ "content-type" ] << "'" << endl;
#endif	

    if ( ( nMatchResult = oMultipartFormDataContentTypeValueRegex.match ( oRequestHeaders [ "content-type" ] ) ) ) {

        assert ( nMatchResult == 2 );
        string sBoundaryString = oMultipartFormDataContentTypeValueRegex [ 1 ];

        // the actual boundary has two dashes prepended to it
        string sActualBoundary = string ("--") + sBoundaryString;

#ifdef EHS_DEBUG
        cerr
            << "[EHS_DEBUG] Info: Found boundary of '" << sBoundaryString << "'" << endl
            << "[EHS_DEBUG] Info: Looking for boundary to match (" << sBody.length ( )
            << ") '" << sBody.substr ( 0, sActualBoundary.length ( ) ) << "'" << endl;
#endif

        // check to make sure we started at the boundary
        if ( sBody.substr ( 0, sActualBoundary.length ( ) ) != sActualBoundary ) {
#ifdef EHS_DEBUG
            cerr << "[EHS_DEBUG] Error: Misparsed multi-part form data for unknown reason - first bytes weren't the boundary string" << endl;
#endif			
            return PARSEMULTIPARTFORMDATA_FAILED;
        }

        // go past the initial boundary
        string sRemainingBody = sBody.substr ( sActualBoundary.length ( ) );

        // while we're at a boundary after we grab a part, keep going
        string::size_type nNextPartPosition;
        while ( ( nNextPartPosition = sRemainingBody.find ( string ( "\r\n" ) + sActualBoundary ) ) != 
                string::npos ) {

#ifdef EHS_DEBUG
            cerr << "[EHS_DEBUG] Info: Found subbody from pos 0 to " << nNextPartPosition << endl;
#endif
            assert ( (unsigned int) sRemainingBody.length ( ) >= sActualBoundary.length ( ) );
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
    PME oCookieRegex ( "\\s*([^=]+)=([^;]+)(;|$)*", "g" );
    int nNameValuePairsFound = 0;
    while ( oCookieRegex.match ( irsData ) ) {
        oCookieMap [ oCookieRegex [ 1 ] ] = oCookieRegex [ 2 ];
        nNameValuePairsFound++;
    }
    return nNameValuePairsFound;
}



// takes data and tries to figure out what it is.  it will loop through 
//   irsData as many times as it can as long as it gets a full line each time.
//   Depending on how much data it's gotten already, it will handle a line 
//   differently.. 
HttpRequest::HttpParseStates HttpRequest::ParseData ( string & irsData ///< buffer to look in for more data
        )
{
    string sLine;
    int nDoneWithCurrentData = 0;
    PME oHeaderRegex ( "^([^:]*):\\s+(.*)\\r\\n$" );

    while ( ! nDoneWithCurrentData && 
            nCurrentHttpParseState != HTTPPARSESTATE_INVALIDREQUEST &&
            nCurrentHttpParseState != HTTPPARSESTATE_COMPLETEREQUEST &&
            nCurrentHttpParseState != HTTPPARSESTATE_INVALID ) {

        switch ( nCurrentHttpParseState ) {

            case HTTPPARSESTATE_REQUEST:

                // get the request line
                GetNextLine ( sLine, irsData );

                // if we got a line, parse out the data..
                if ( sLine.length ( ) == 0 ) {
                    nDoneWithCurrentData = 1;
                } else {
                    // if we got a line, look for a request line

                    // everything must be uppercase according to rfc2616
                    PME oRequestLineRegex ( "^(OPTIONS|GET|HEAD|POST|PUT|DELETE|TRACE|CONNECT) ([^ ]*) HTTP/([^ ]+)\\r\\n$" );
                    if ( oRequestLineRegex.match ( sLine ) ) {

                        // get the info from the request line
                        nRequestMethod = GetRequestMethodFromString ( oRequestLineRegex [ 1 ] );
                        sUri = oRequestLineRegex [ 2 ];
                        sOriginalUri = oRequestLineRegex [ 2 ];
                        sHttpVersionNumber = oRequestLineRegex [ 3 ];

                        // check to see if the uri appeared to have form data in it
                        GetFormDataFromString ( sUri );

                        // on to the headers
                        nCurrentHttpParseState = HTTPPARSESTATE_HEADERS;

                    } else {
                        // if the regex failed
                        nCurrentHttpParseState = HTTPPARSESTATE_INVALIDREQUEST;

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
                    if ( oRequestHeaders.find ( "content-length" ) !=
                            oRequestHeaders.end ( ) ) {
                        nCurrentHttpParseState = HTTPPARSESTATE_BODY;
                    } else {
                        nCurrentHttpParseState = HTTPPARSESTATE_COMPLETEREQUEST;
                    }

                    // if this is an HTTP/1.1 request, then it had better have a Host: header
                    if ( sHttpVersionNumber == "1.1" && 
                            oRequestHeaders [ "host" ].length ( ) == 0 ) {
                        nCurrentHttpParseState = HTTPPARSESTATE_INVALIDREQUEST;
                    }

                } else if ( oHeaderRegex.match ( sLine ) ) {
                    // else if there is still data

                    string sName = oHeaderRegex [ 1 ];
                    string sValue = oHeaderRegex [ 2 ];

                    if ( sName == "Transfer-Encoding" &&
                            sValue == "chunked" ) {
                        // TODO: Implement chunked encoding    
                        cerr << "EHS DOES NOT SUPPORT CHUNKED ENCODING" << endl;
                        nCurrentHttpParseState = HTTPPARSESTATE_INVALIDREQUEST;
                    }

                    sName = mytolower ( sName );
                    if ( sName == "cookie" ) {
                        ParseCookieData ( sValue );
                    }
                    oRequestHeaders [ sName ] = sValue;
                } else {
                    // else we had some sort of error -- bail out

#ifdef EHS_DEBUG
                    cerr << "[EHS_DEBUG] Error: Invalid header line: '" << sLine << "'" << endl;
#endif
                    nCurrentHttpParseState = HTTPPARSESTATE_INVALIDREQUEST;
                    nDoneWithCurrentData = 1;
                }
                break;


            case HTTPPARSESTATE_BODY:
                {

                    // if a content length wasn't specified, we can't be here (we 
                    //   don't know chunked encoding)
                    if ( oRequestHeaders.find ( "content-length" ) == 
                            oRequestHeaders.end ( ) ) {

                        nCurrentHttpParseState = HTTPPARSESTATE_INVALIDREQUEST;
                        continue;

                    }

                    // get the content length
                    unsigned int nContentLength = 
                        atoi ( oRequestHeaders [ "content-length" ].c_str ( ) );

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
                        sBody = irsData.substr ( 0, nContentLength );
                        irsData = irsData.substr ( nContentLength );

                        // if we're dealing with multi-part form attachments
                        if ( oRequestHeaders [ "content-type" ].substr ( 0, 9 ) == 
                                "multipart" ) {
                            // handle the body as if it's multipart form data
                            if ( ParseMultipartFormData ( ) == 
                                    PARSEMULTIPARTFORMDATA_SUCCESS ) {
                                nCurrentHttpParseState = HTTPPARSESTATE_COMPLETEREQUEST;
                            } else {
#ifdef EHS_DEBUG						
                                cerr << "[EHS_DEBUG] Error: Mishandled multi-part form data for unknown reason" << endl;
#endif
                                nCurrentHttpParseState = HTTPPARSESTATE_INVALIDREQUEST;
                            }
                        } else {
                            // else the body is just one piece

                            // check for any form data
                            GetFormDataFromString ( sBody );
#ifdef EHS_DEBUG
                            cerr << "Done with body, done with entire request" << endl;
#endif
                            nCurrentHttpParseState = HTTPPARSESTATE_COMPLETEREQUEST;
                        }

                    }
                    nDoneWithCurrentData = 1;
                }
                break;


            case HTTPPARSESTATE_INVALID:
            default:
#ifdef EHS_DEBUG
                cerr << "[EHS_DEBUG] Critical error: Invalid internal state: " << nCurrentHttpParseState << endl;
#endif
                assert ( 0 );
                break;
        }
    }
    return nCurrentHttpParseState;

}

HttpRequest::HttpRequest ( int inRequestId,
        EHSConnection * ipoSourceEHSConnection ) :
    nCurrentHttpParseState  ( HTTPPARSESTATE_REQUEST ),
    nRequestMethod ( REQUESTMETHOD_UNKNOWN ),
    sUri ( "" ),
    sHttpVersionNumber ( "" ),
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

// returns 1 if it got a line, 0 otherwise
int GetNextLine ( string & irsLine, ///< line removed from *ippsBuffer
        string & irsBuffer ///< buffer from which to remove a line
        )
{
    int nResult = 0;

    // can't use split, because we lose our \r\n
    PME oLineRegex ( "^([^\\r]*\\r\\n)(.*)$", "sm" );
    if ( oLineRegex.match ( irsBuffer ) == 3 ) {
        irsLine = oLineRegex [ 1 ];
        irsBuffer = oLineRegex [ 2 ];
    } else {
        irsLine = "";
    }
    return nResult;
}

/// List of possible HTTP request methods
const char * RequestMethodStrings [] = {
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

string & mytolower (string & s ///< string to make lowercase
        ) 
{ 
    transform ( s.begin(), s.end(), s.begin(), ptr_fun<int, int>(tolower));
    return s;
}


string HttpRequest::GetAddress ( )
{
    return m_poSourceEHSConnection->GetAddress ( );
}

int HttpRequest::GetPort ( )
{
    return m_poSourceEHSConnection->GetPort ( );
}


int HttpRequest::ClientDisconnected ( )
{ 
    return m_poSourceEHSConnection->Disconnected ( ); 
}
