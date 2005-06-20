# ==============================
# Fichier:			Makefile
# Projet:			objc-runtime-x
# Ecrit par:		Paul Guyot (pguyot@kallisys.net)
# 
# CrŽŽ le:			20/3/2005
# Tabulation:		4 espaces
# 
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1
# 
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
# 
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
# 
# The Original Code is Makefile.
# 
# The Initial Developer of the Original Code is Paul Guyot.
# Portions created by the Initial Developer are Copyright (C) 2005 the
# Initial Developer. All Rights Reserved.
# 
# Contributor(s):
#   Paul Guyot <pguyot@kallisys.net> (original author)
# 
# ***** END LICENSE BLOCK *****
# ===========
# $Id$
# ===========

# Makefile to build objc-runtime-x

DYLIB = objc-runtime-x.dylib
OBJ = objc-runtime-x.o
CFLAGS = -Wall
LDFLAGS = -dynamic -dylib
LD = ld

all: $(DYLIB)

%.o: %.s
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

$(DYLIB): $(OBJ)
	$(LD) $(LDFLAGS) $^ -o $@

clean:
	rm -rf $(DYLIB) $(OBJ)

## ==================================================================== ##
## Be careful when a loop exits to the same place from side and bottom. ##
## ==================================================================== ##
