
#ifndef CONTENTDISPOSITION_H
#define CONTENTDISPOSITION_H

#include <string>
#ifdef EHS_MEMORY
# include <iostream>
#endif

#include "ehstypes.h"

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
