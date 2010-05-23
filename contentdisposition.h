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

#ifndef CONTENTDISPOSITION_H
#define CONTENTDISPOSITION_H

#include <string>
#ifdef EHS_MEMORY
# include <iostream>
#endif

#include <ehstypes.h>

/// This stores the content disposition of a subbody
/**
 * This stores the content disposition of a subbody.  This is used for
 *   multi-part form data.
 */
class ContentDisposition {

    public:

        /// constructor
        ContentDisposition ( ) {
#ifdef EHS_MEMORY
            std::cerr << "[EHS_MEMORY] Allocated: ContentDisposition" << std::endl;
#endif		
        }

        /// destructor
        ~ContentDisposition ( ) {
#ifdef EHS_MEMORY
            std::cerr << "[EHS_MEMORY] Deallocated: ContentDisposition" << std::endl;
#endif		
        }

#ifdef EHS_MEMORY
        /// this is only for watching memory allocation
        ContentDisposition ( const ContentDisposition & iroContentDisposition ) {
            *this = iroContentDisposition;
            std::cerr << "[EHS_MEMORY] Allocated: ContentDisposition (Copy Constructor)" << std::endl;
        }
#endif		

        StringMap oContentDispositionHeaders; ///< map of headers for this content disposition
        std::string sContentDisposition; ///< content disposition string for this object

};


#endif // CONTENTDISPOSITION_H
