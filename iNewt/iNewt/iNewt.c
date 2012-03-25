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

void iNewt_Setup(void) {
  NewtInit(0, NULL, 0);
  protoFILE_install();
  protoREGEX_install();
  NativeCalls_install();
}

void iNewt_Cleanup(void) {
  NewtCleanup();
}