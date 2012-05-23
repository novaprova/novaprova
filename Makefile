PACKAGE=	novaprova
VERSION=	0.1

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
CDEFINES=	-D_GNU_SOURCE -I. \
		$(shell pkg-config --cflags libxml++-2.6)
CWARNFLAGS=	-Wall -Wextra
CFLAGS=		$(CDEBUGFLAGS) $(COPTFLAGS) $(CWARNFLAGS) $(CDEFINES)
CXXFLAGS=	$(CFLAGS)
INSTALL=	/usr/bin/install -c
RANLIB=		ranlib
depdir=		.deps

SUBDIRS_PRE=
SUBDIRS_POST=	tests

all clean distclean check install:
	@for dir in $(SUBDIRS_PRE) ; do $(MAKE) -C $$dir $@ ; done
	@$(MAKE) $@-local
	@for dir in $(SUBDIRS_POST) ; do $(MAKE) -C $$dir $@ ; done

install check: all

all-local: libnp.a novaprova.pc

libnp_SOURCE=	\
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
		np/testmanager.cxx \
		np/testnode.cxx \
		np/text_listener.cxx \
		np/types.cxx \
		np/util/common.cxx \
		np/util/filename.cxx \
		np/util/tok.cxx \
		spiegel/dwarf/abbrev.cxx \
		spiegel/dwarf/compile_unit.cxx \
		spiegel/dwarf/entry.cxx \
		spiegel/dwarf/enumerations.cxx \
		spiegel/dwarf/reference.cxx \
		spiegel/dwarf/state.cxx \
		spiegel/dwarf/string_table.cxx \
		spiegel/dwarf/value.cxx \
		spiegel/dwarf/walker.cxx \
		spiegel/platform/linux.cxx \
		spiegel/intercept.cxx \
		spiegel/mapping.cxx \
		spiegel/spiegel.cxx \

libnp_PRIVHEADERS= \
		np_priv.h \
		np/util/common.hxx \
		np/util/filename.hxx \
		np/util/tok.hxx \
		spiegel/dwarf/abbrev.hxx \
		spiegel/dwarf/compile_unit.hxx \
		spiegel/dwarf/entry.hxx \
		spiegel/dwarf/enumerations.hxx \
		spiegel/dwarf/reader.hxx \
		spiegel/dwarf/reference.hxx \
		spiegel/dwarf/section.hxx \
		spiegel/dwarf/state.hxx \
		spiegel/dwarf/string_table.hxx \
		spiegel/dwarf/value.hxx \
		spiegel/dwarf/walker.hxx \
		spiegel/platform/common.hxx \
		spiegel/intercept.hxx \
		spiegel/common.hxx \
		spiegel/mapping.hxx \
		spiegel/spiegel.hxx \

libnp_HEADERS=	np.h \
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

libnp_OBJS=	\
	$(patsubst %.c,%.o,$(filter %.c,$(libnp_SOURCE))) \
	$(patsubst %.cxx,%.o,$(filter %.cxx,$(libnp_SOURCE)))
#
# Automatic dependency tracking
libnp_DFILES= \
	$(patsubst %.c,$(depdir)/%.d,$(filter %.c,$(libnp_SOURCE))) \
	$(patsubst %.cxx,$(depdir)/%.d,$(filter %.cxx,$(libnp_SOURCE))) \

-include $(libnp_DFILES)

%.o: %.cxx
	$(COMPILE.C) -o $@ $<

$(depdir)/%.d: %.cxx
	@[ -d $(@D) ] || mkdir -p $(@D)
	$(COMPILE.C) -MM -MF $@ -MT $(patsubst %.cxx,%.o,$<) $<

$(depdir)/%.d: %.c
	@[ -d $(@D) ] || mkdir -p $(@D)
	$(COMPILE.c) -MM -MF $@ -MT $(patsubst %.c,%.o,$<) $<

libnp.a: $(libnp_OBJS)
	$(AR) $(ARFLAGS) libnp.a $(libnp_OBJS)

documentation:
	$(RM) -r doc/api-ref doc/man
	doxygen
	cd doc ;\
	    ln -s api-ref api-ref-$(VERSION) ;\
	    tar -chjvf api-ref-$(VERSION).tar.bz2 api-ref-$(VERSION) ;\
	    rm -f api-ref-$(VERSION)

install-local: documentation
	$(INSTALL) -d $(DESTDIR)$(includedir)/novaprova/np
	for hdr in $(libnp_HEADERS) ; do \
	    $(INSTALL) -m 644 $$hdr $(DESTDIR)$(includedir)/novaprova/$$hdr ;\
	done
	$(INSTALL) -d $(DESTDIR)$(libdir)
	$(INSTALL) -m 644 libnp.a $(DESTDIR)$(libdir)/libnp.a
	$(RANLIB) $(DESTDIR)$(libdir)/libnp.a
	$(INSTALL) -d $(DESTDIR)$(mandir)/man3
	$(INSTALL) doc/man/man3/*.3 $(DESTDIR)$(mandir)/man3
	$(INSTALL) -d $(DESTDIR)$(pkgdocdir)/api-ref
	$(INSTALL) doc/api-ref/* $(DESTDIR)$(pkgdocdir)/api-ref
	$(INSTALL) novaprova.pc $(DESTDIR)$(configdir)

clean-local:
	$(RM) libnp.a $(libnp_OBJS)

distclean-local: clean-local
	$(RM) -r doc/api-ref doc/man
	$(RM) -r $(depdir)

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
