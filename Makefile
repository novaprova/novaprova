#
#  Copyright 2011-2012 Gregory Banks
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
PACKAGE=	novaprova
VERSION=	1.0

prefix=		/usr/local
exec_prefix=	$(prefix)
includedir=	$(prefix)/include
libdir=		$(exec_prefix)/lib
mandir=		$(prefix)/share/man
pkgdocdir=	$(prefix)/share/doc/$(PACKAGE)
configdir=	$(libdir)/pkgconfig

CC=		g++
CXX=		g++
CDEBUGFLAGS=	-g
COPTFLAGS=	-O0
CDEFINES=	-I. \
		$(shell ./platform.sh --cflags) \
		$(shell pkg-config --cflags libxml++-2.6)
CWARNFLAGS=	-Wall -Wextra
CFLAGS=		$(CDEBUGFLAGS) $(COPTFLAGS) $(CWARNFLAGS) $(CDEFINES)
CXXFLAGS=	$(CFLAGS)
INSTALL=	/usr/bin/install -c
RANLIB=		ranlib
depdir=		.deps

SUBDIRS_PRE=
SUBDIRS_POST=	tests doc/get-start

all clean distclean check install docs:
	@for dir in $(SUBDIRS_PRE) ; do $(MAKE) -C $$dir $@ ; done
	@$(MAKE) $@-local
	@for dir in $(SUBDIRS_POST) ; do $(MAKE) -C $$dir $@ ; done

install check: all

all-local: .config-ok libnovaprova.a novaprova.pc

.config-ok:
	@./platform.sh --verify
	@pkg-config --cflags libxml++-2.6 > /dev/null
	@touch $@

libnovaprova_SOURCE= \
		np.c \
		isyslog.c iassert.c icunit.c iexit.c uasserts.c \
		np/child.cxx \
		np/classifier.cxx \
		np/event.cxx \
		np/job.cxx \
		np/junit_listener.cxx \
		np/plan.cxx \
		np/proxy_listener.cxx \
		np/runner.cxx \
		np/spiegel/dwarf/abbrev.cxx \
		np/spiegel/dwarf/compile_unit.cxx \
		np/spiegel/dwarf/entry.cxx \
		np/spiegel/dwarf/enumerations.cxx \
		np/spiegel/dwarf/reference.cxx \
		np/spiegel/dwarf/state.cxx \
		np/spiegel/dwarf/string_table.cxx \
		np/spiegel/dwarf/value.cxx \
		np/spiegel/dwarf/walker.cxx \
		np/spiegel/intercept.cxx \
		np/spiegel/mapping.cxx \
		$(addprefix np/spiegel/platform/,$(shell ./platform.sh --source)) \
		np/spiegel/spiegel.cxx \
		np/testmanager.cxx \
		np/testnode.cxx \
		np/text_listener.cxx \
		np/types.cxx \
		np/util/common.cxx \
		np/util/filename.cxx \
		np/util/tok.cxx \

libnovaprova_PRIVHEADERS= \
		np/spiegel/common.hxx \
		np/spiegel/dwarf/abbrev.hxx \
		np/spiegel/dwarf/compile_unit.hxx \
		np/spiegel/dwarf/entry.hxx \
		np/spiegel/dwarf/enumerations.hxx \
		np/spiegel/dwarf/reader.hxx \
		np/spiegel/dwarf/reference.hxx \
		np/spiegel/dwarf/section.hxx \
		np/spiegel/dwarf/state.hxx \
		np/spiegel/dwarf/string_table.hxx \
		np/spiegel/dwarf/value.hxx \
		np/spiegel/dwarf/walker.hxx \
		np/spiegel/intercept.hxx \
		np/spiegel/mapping.hxx \
		np/spiegel/platform/common.hxx \
		np/spiegel/spiegel.hxx \
		np/util/common.hxx \
		np/util/filename.hxx \
		np/util/tok.hxx \
		np_priv.h \

libnovaprova_HEADERS= \
		np.h \
		np/child.hxx \
		np/classifier.hxx \
		np/event.hxx \
		np/job.hxx \
		np/junit_listener.hxx \
		np/listener.hxx \
		np/plan.hxx \
		np/proxy_listener.hxx \
		np/runner.hxx \
		np/testmanager.hxx \
		np/testnode.hxx \
		np/text_listener.hxx \
		np/types.hxx \

libnovaprova_OBJS= \
	$(patsubst %.c,%.o,$(filter %.c,$(libnovaprova_SOURCE))) \
	$(patsubst %.cxx,%.o,$(filter %.cxx,$(libnovaprova_SOURCE)))
#
# Automatic dependency tracking
libnovaprova_DFILES= \
	$(patsubst %.c,$(depdir)/%.d,$(filter %.c,$(libnovaprova_SOURCE))) \
	$(patsubst %.cxx,$(depdir)/%.d,$(filter %.cxx,$(libnovaprova_SOURCE))) \

-include $(libnovaprova_DFILES)

%.o: %.cxx
	$(COMPILE.C) -o $@ $<

$(depdir)/%.d: %.cxx
	@[ -d $(@D) ] || mkdir -p $(@D)
	$(COMPILE.C) -MM -MF $@ -MT $(patsubst %.cxx,%.o,$<) $<

$(depdir)/%.d: %.c
	@[ -d $(@D) ] || mkdir -p $(@D)
	$(COMPILE.c) -MM -MF $@ -MT $(patsubst %.c,%.o,$<) $<

libnovaprova.a: $(libnovaprova_OBJS)
	$(AR) $(ARFLAGS) libnovaprova.a $(libnovaprova_OBJS)

DOC_DELIVERABLES= \
	    get-start/index.html \
	    get-start/pygmentize.css \
	    get-start/example1 \
	    get-start/example1/mytest.c \
	    get-start/example1/mycode.h \
	    get-start/example1/mycode.c \
	    get-start/example1/Makefile \
	    get-start/example1/testrunner.c \
	    api-ref \
	    man \

docs-local:
	$(RM) -r doc/api-ref doc/man
	doxygen
	ln -s doc doc-$(VERSION)
	tar -chjvf doc-$(VERSION).tar.bz2 $(addprefix doc-$(VERSION)/,$(DOC_DELIVERABLES))
	$(RM) doc-$(VERSION)

install: docs

install-local:
	$(INSTALL) -d $(DESTDIR)$(includedir)/novaprova/np
	for hdr in $(libnovaprova_HEADERS) ; do \
	    $(INSTALL) -m 644 $$hdr $(DESTDIR)$(includedir)/novaprova/$$hdr ;\
	done
	$(INSTALL) -d $(DESTDIR)$(libdir)
	$(INSTALL) -m 644 libnovaprova.a $(DESTDIR)$(libdir)/libnovaprova.a
	$(RANLIB) $(DESTDIR)$(libdir)/libnovaprova.a
	$(INSTALL) -d $(DESTDIR)$(mandir)/man3
	$(INSTALL) doc/man/man3/*.3 $(DESTDIR)$(mandir)/man3
	$(INSTALL) -d $(DESTDIR)$(pkgdocdir)/api-ref
	$(INSTALL) doc/api-ref/* $(DESTDIR)$(pkgdocdir)/api-ref
	$(INSTALL) novaprova.pc $(DESTDIR)$(configdir)

clean-local:
	$(RM) libnovaprova.a $(libnovaprova_OBJS)

distclean-local: clean-local
	$(RM) -r doc/api-ref doc/man
	$(RM) -r $(depdir)
	$(RM) novaproca.pc

check-local:

%: %.in
	sed \
	    -e 's|@PACKAGE@|$(PACKAGE)|' \
	    -e 's|@VERSION@|$(VERSION)|' \
	    -e 's|@prefix@|$(prefix)|' \
	    -e 's|@exec_prefix@|$(exec_prefix)|' \
	    -e 's|@includedir@|$(includedir)|' \
	    -e 's|@libdir@|$(libdir)|' \
	    -e 's|@mandir@|$(mandir)|' \
	    -e 's|@pkgdocdir@|$(pkgdocdir)|' \
	    -e 's|@configdir@|$(configdir)|' \
	    -e 's|@mandir@|$(mandir)|' \
	    < $< > $@

DIST_TARBALL=	$(PACKAGE)-$(VERSION).tar.bz2

DIST_FILES= $(shell git ls-files) \
	doc/get-start/index.html \
	doc/get-start/pygmentize.css \

DIST_DIRS= \
	doc/api-ref \
	doc/man \

dist: docs
	$(RM) -r $(PACKAGE)-$(VERSION)
	mkdir $(PACKAGE)-$(VERSION)
	for file in $(DIST_FILES) `find $(DIST_DIRS) -type f` ; do \
	    mkdir -p `dirname $(PACKAGE)-$(VERSION)/$$file` ;\
	    ln $$file $(PACKAGE)-$(VERSION)/$$file ;\
	done
	tar -chjvf $(DIST_TARBALL) $(PACKAGE)-$(VERSION)
	$(RM) -r $(PACKAGE)-$(VERSION)
