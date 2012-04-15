# This Makefile is based on LuaSec's Makefile. Thanks to the LuaSec developers.
# Inform the location to intall the modules
MAPM_INCDIR = -I../mapm_4.9.5a
MAPM_LIBS   = -L../mapm_4.9.5a -lmapm

LUA_PATH   = /usr/share/lua/5.1
LUA_CPATH  = /usr/lib/lua/5.1
LUA_INCDIR = -I/usr/include/lua5.1
LUA_LIBS   = -L/usr/lib -llua

# For Mac OS X: set the system version
MACOSX_VERSION = 10.4

PLAT = none
DEFS =
CMOD = mapm.so
OBJS = lmapm.o

INCLUDES = $(LUA_INCDIR) $(MAPM_INCDIR)
LIBS = $(LUA_LIBS) $(MAPM_LIBS)
WARN = -Wall -pedantic

BSD_CFLAGS  = -O2 -fPIC
BSD_LDFLAGS = -O -shared -fPIC

LNX_CFLAGS  = -O2 -fPIC
LNX_LDFLAGS = -O -shared -fPIC
LNX_LUA_LIBS =

MAC_ENV     = env MACOSX_DEPLOYMENT_TARGET='$(MACVER)'
MAC_CFLAGS  = -O2 -fPIC -fno-common
MAC_LDFLAGS = -bundle -undefined dynamic_lookup -fPIC

MGW_LUA_INCDIR = -Id:/lua/include
MGW_LUA_LIBS = d:/lua/lua51.dll
MGW_CMOD = mapm.dll
MGW_CFLAGS = -O2 -mdll -DLUA_BUILD_AS_DLL
MGW_LDFLAGS = -mdll

CC = gcc
LD = $(MYENV) gcc
CFLAGS  = $(MYCFLAGS) $(WARN) $(INCLUDES) $(DEFS)
LDFLAGS = $(MYLDFLAGS)

.PHONY: test clean install none linux bsd macosx

none:
	@echo "Usage: $(MAKE) <platform>"
	@echo "  * linux"
	@echo "  * bsd"
	@echo "  * macosx"
	@echo "  * mingw"

install: $(CMOD)
	cp $(CMOD) $(LUACPATH)

uninstall:
	-rm $(LUACPATH)/$(CMOD)

linux:
	@$(MAKE) $(CMOD) PLAT=linux MYCFLAGS="$(LNX_CFLAGS)" MYLDFLAGS="$(LNX_LDFLAGS) LUA_LIBS="$(LNX_LUA_LIBS)

bsd:
	@$(MAKE) $(CMOD) PLAT=bsd MYCFLAGS="$(BSD_CFLAGS)" MYLDFLAGS="$(BSD_LDFLAGS)"

macosx:
	@$(MAKE) $(CMOD) PLAT=macosx MYCFLAGS="$(MAC_CFLAGS)" MYLDFLAGS="$(MAC_LDFLAGS)" MYENV="$(MAC_ENV)"

mingw:
	@$(MAKE) $(MGW_CMOD) PLAT=mingw MYCFLAGS="$(MGW_CFLAGS)" MYLDFLAGS="$(MGW_LDFLAGS)" LUA_INCDIR="$(MGW_LUA_INCDIR)" LUA_LIBS="$(MGW_LUA_LIBS)" CMOD="$(MGW_CMOD)"

test:
	lua ./test.lua

clean:
	-rm -f $(OBJS) $(CMOD) $(MGW_CMOD)

.c.o:
	$(CC) $(CFLAGS) $< -c -o $@

$(CMOD): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -o $@
