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

#ifndef EHSTYPES_H
#define EHSTYPES_H

#include <string>
#include <map>
#include <list>

class EHSConnection;
class EHS;
class Datum;
class FormValue;
class HttpResponse;
class HttpRequest;

/// generic std::string => std::string map used by many things
typedef std::map < std::string, std::string > StringMap;

/// generic list of std::strings
typedef std::list < std::string > StringList;

/// Define a list of EHSConnection objects to handle all current connections
typedef std::list < EHSConnection * > EHSConnectionList;

// map for registered EHS objects on a path
typedef std::map < std::string, EHS * > EHSMap;

/// map type for storing EHSServer parameters
typedef std::map < std::string, Datum > EHSServerParameters;

/// cookies that come in from the client, mapped by name
typedef std::map < std::string, std::string > CookieMap;

/// describes a form value that came in from a client
typedef std::map < std::string, FormValue > FormValueMap;

/// describes a cookie to be sent back to the client
typedef std::map < std::string, Datum > CookieParameters;

/// holds respose objects not yet ready to send
typedef std::map < int, HttpResponse * > HttpResponseMap;

/// holds a list of pending requests
typedef std::list < HttpRequest * > HttpRequestList;

#endif
