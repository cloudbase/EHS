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

/*
  This is a sample EHS program that allows you to upload
  a file to the server 
*/
#include <ehs.h>

#include <fstream>
#include <iostream>

#include <pme.h>

using namespace std;

class FileUploader : public EHS {
    public:
        FileUploader ( ) {}
        ResponseCode HandleRequest ( HttpRequest *, HttpResponse *);
};

ResponseCode FileUploader::HandleRequest ( HttpRequest * request, HttpResponse * response )
{
    string sUri = request->sUri;
    cerr << "Request-URI: " << sUri << endl;

    if ( sUri == "/" ) {
        string sBody = "<html><head><title>ehs uploader</title></head><body>";
        sBody += "<form method=\"POST\" action=";
        sBody += "/upload.html enctype=\"multipart/form-data\">Upload file:<br />";
        sBody += "<input type=\"file\" name=\"file\"><br />";
        sBody += "<input type=\"submit\" value=\"submit\"></form></body></html>";

        response->SetBody ( sBody.c_str(), sBody.length ( ) );
        return HTTPRESPONSECODE_200_OK;
    }

    if ( sUri == "/upload.html" ) {
        int nFileSize = request->oFormValueMap [ "file" ].sBody.length ( );
        string sFileName = request->oFormValueMap [ "file" ].
            oContentDisposition.oContentDispositionHeaders [ "filename" ];
        cerr << "nFileSize = " << nFileSize << endl;
        cerr << "sFileName = '" << sFileName << "'" << endl;
        if ( 0 != nFileSize ) {
            size_t lastSlash = sFileName.find_last_of("/\\");
            sFileName = sFileName.substr(lastSlash + 1);
            cerr << "stripped sFileName = '" << sFileName << "'" << endl;
            if ( !sFileName.empty ( ) ) {
                cerr << "Writing " << nFileSize << " bytes to file" << endl;
                ofstream outfile ( sFileName.c_str(), ios::out | ios::trunc | ios::binary );
                outfile.write( request->oFormValueMap [ "file" ].sBody.c_str(), nFileSize);
                outfile.close ( );
            } else {
                cerr << "NO FILENAME FOUND" << endl;
            }
            response->SetBody ( "", 0 );
            return HTTPRESPONSECODE_200_OK;
        } else {
            string msg = "<http><body>Must upload file</body></http>";
            // there was nothing sent in as a file
            response->SetBody ( msg.c_str(), msg.length() );
            return HTTPRESPONSECODE_200_OK;
        }
    }

    return HTTPRESPONSECODE_404_NOTFOUND;
}

int main ( int argc, char ** argv )
{
    if ( argc < 2 ) {
        cout << "usage: " << argv[0] << " <port>" << endl;
        exit ( 0 );
    }

    FileUploader srv;
    EHSServerParameters oSP;
    oSP["port"] = argv [ 1 ];
    oSP [ "mode" ] = "threadpool";
    oSP [ "maxrequestsize" ] = 1024 * 1024 * 10;
    srv.StartServer ( oSP );
    cout << "Press RETURN to terminate the server: "; cout.flush();
    cin.get();
    srv.StopServer ( );
    return 0;
}
