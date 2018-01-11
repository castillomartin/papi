# Modified by Philip Mucci, <mucci@icl.utk.edu> for PAPI HL

#
# Copyright (c) 2002-2006 Hewlett-Packard Development Company, L.P.
# Contributed by Stephane Eranian <eranian@hpl.hp.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
# PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
# CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
# OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
TOPDIR  := $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)/..

include $(TOPDIR)/config.mk
include $(TOPDIR)/rules.mk

#
# Common files
#
SRCS=papi_hl.c

CFLAGS+=-D_REENTRANT -I. -fvisibility=hidden

ifeq ($(SYS),Linux)
SLDFLAGS=$(LDFLAGS) -shared -Wl,-soname -Wl,$(VLIBPAPIHL)
SLIBPAPIHL=libpapihl.so.$(VERSION).$(REVISION).$(AGE)
VLIBPAPIHL=libpapihl.so.$(VERSION)
SOLIBEXT=so
endif

CFLAGS+=-I.
ALIBPAPIHL=libpapihl.a

TARGETS=$(ALIBPAPIHL)

ifeq ($(CONFIG_PAPIHLLIB_SHARED),y)
TARGETS += $(SLIBPAPIHL)
endif

OBJS=$(SRCS:.c=.o)
SOBJS=$(OBJS:.o=.lo)

INC_COMMON= $(PAPIHLINCDIR)/papihl.h papi_hl_priv.h

INCDEP=$(INC_COMMON) $(INCARCH)

all: $(TARGETS)

$(OBJS) $(SOBJS): $(TOPDIR)/config.mk $(TOPDIR)/rules.mk Makefile $(INCDEP)

libpapihl.a:  $(OBJS)
	$(RM) $@
	$(AR) cq $@ $(OBJS)

$(SLIBPAPIHL):  $(SOBJS)
	$(CC) $(CFLAGS) $(SLDFLAGS)  -o $@ $(SOBJS)
	$(LN) $@ $(VLIBPAPIHL)
	$(LN) $@ libpapihl.$(SOLIBEXT)

clean:
	$(RM) -f *.o *.lo *.a *.so* *~ *.$(SOLIBEXT)

distclean: clean

depend:
	$(MKDEP) $(CFLAGS) $(SRCS)

install: $(TARGETS)

install:
	@echo building: $(TARGETS)
	-mkdir -p $(DESTDIR)$(LIBDIR)
	$(INSTALL) -m 644 $(ALIBPAPIHL) $(DESTDIR)$(LIBDIR)
ifeq ($(CONFIG_PAPIHLLIB_SHARED),y)
	$(INSTALL) $(SLIBPAPIHL) $(DESTDIR)$(LIBDIR)
	cd $(DESTDIR)$(LIBDIR); $(LN) $(SLIBPAPIHL) $(VLIBPAPIHL)
	cd $(DESTDIR)$(LIBDIR); $(LN) $(SLIBPAPIHL) libpapihl.$(SOLIBEXT)
	-$(LDCONFIG)
endif

tags:
	$(CTAGS) -o $(TOPDIR)/tags --tag-relative=yes $(SRCS) $(INCDEP)
