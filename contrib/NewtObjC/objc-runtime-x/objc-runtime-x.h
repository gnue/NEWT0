#include <objc/objc-runtime.h>

/* call a method with arbitraty parameters in a marg_list */
id objc_methodCallv(id self, SEL _cmd, IMP method, unsigned arg_size, marg_list arg_frame);

/* call a method returning a structure with arbitraty parameters in a marg_list */
/* cf the warning about objc_msgSend_stret in objc/objc-runtime.h */
#if defined(__cplusplus)
	id objc_methodCallv_stret(
				id self,
				SEL _cmd,
				IMP method,
				unsigned arg_size,
				marg_list arg_frame);
#else
	void objc_methodCallv_stret(
				void* stretAddr,
				id self,
				SEL _cmd,
				IMP method,
				unsigned arg_size,
				marg_list arg_frame);
#endif
