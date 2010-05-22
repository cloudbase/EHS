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
