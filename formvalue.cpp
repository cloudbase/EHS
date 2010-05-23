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

#include <string>

#include "formvalue.h"
#include "contentdisposition.h"

FormValue::FormValue ( ) {
#ifdef EHS_MEMORY
    std::cerr << "[EHS_MEMORY] Allocated: FormValue" << std::endl;
#endif		
}

FormValue::FormValue ( std::string & irsBody, ///< body for the form value
        ContentDisposition & ioContentDisposition ///< content disposition type string
        ) :
    oContentDisposition ( ioContentDisposition ),
    sBody ( irsBody ) {
#ifdef EHS_MEMORY
        std::cerr << "[EHS_MEMORY] Allocated: FormValue" << std::endl;
#endif		
    }

FormValue::FormValue ( const FormValue & iroFormValue ) {
    *this = iroFormValue;
#ifdef EHS_MEMORY
    std::cerr << "[EHS_MEMORY] Allocated: FormValue (Copy Constructor)" << std::endl;
#endif		
}

FormValue::~FormValue ( ) {
#ifdef EHS_MEMORY
    std::cerr << "[EHS_MEMORY] Deallocated: FormValue" << std::endl;
#endif		
}
