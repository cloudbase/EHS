#ifndef FORMVALUE_H
#define FORMVALUE_H

#include <string>

#include "ehstypes.h"
#include "contentdisposition.h"

/// This is how data is passed in from the client has to be more complicated because of metadata associated with MIME-style attachments
/**
 * This is how data is passed in from the client has to be more complicated because of metadata 
 *   associated with MIME-style attachments.  Each element of a form is put into a ContentDisposition
 *   object which can be looked at in HandleRequest to see what data might have been sent in.
 */
class FormValue {
	
  public:
	
	/// for MIME attachments only, normal header information like content-type -- everything except content-disposition, which is in oContentDisposition
	StringMap oFormHeaders;

	/// everything in the content disposition line
	ContentDisposition oContentDisposition; 
	
	/// the body of the value.  For non-MIME-style attachments, this is the only part used.
	std::string sBody; 
	
	/// Default constructor
	FormValue ( );
	
	/// Constructor 
	FormValue ( std::string & irsBody, ///< body for the form value
				ContentDisposition & ioContentDisposition ///< content disposition type string
		);

	/// Copy constructor
	FormValue ( const FormValue & iroFormValue );

	/// destructor
	virtual ~FormValue ( );
};

#endif // FORMVALUE_H
