//
//  iNewt.c
//  iNewt
//
//  Created by Steve White on 3/25/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "iNewt.h"

extern void NativeCalls_install(void);
extern void protoFILE_install(void);
extern void protoREGEX_install(void);

void iNewt_RegisterLibrary(char *library) {
	newtRefVar requires = NcGetGlobalVar(NSSYM0(requires));
  if (! NewtRefIsFrame(requires))
	{
		requires = NcMakeFrame();
		NcDefGlobalVar(NSSYM0(requires), requires);
	}

  NcSetSlot(requires, NewtMakeSymbol(library), NewtMakeString(library, true));
}

void iNewt_Setup(void) {
  NewtInit(0, NULL, 0);

  protoFILE_install();
  iNewt_RegisterLibrary("protoFILE");
  
  protoREGEX_install();
  iNewt_RegisterLibrary("protoREGEX");
  
  NativeCalls_install();
  iNewt_RegisterLibrary("NativeCalls");
}

void iNewt_Cleanup(void) {
  NewtCleanup();
}