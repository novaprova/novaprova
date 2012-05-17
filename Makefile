
prefix=		/usr/local
includedir=	$(prefix)/include
libdir=		$(prefix)/lib

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

SUBDIRS_PRE=	libspiegel
SUBDIRS_POST=	tests

all clean check install:
	@for dir in $(SUBDIRS_PRE) ; do $(MAKE) -C $$dir $@ ; done
	@$(MAKE) $@-local
	@for dir in $(SUBDIRS_POST) ; do $(MAKE) -C $$dir $@ ; done

install check: all

all-local: libnp.a

libnp_SOURCE=	\
		np.c \
		isyslog.c iassert.c icunit.c iexit.c uasserts.c \
		np/child.cxx \
		np/classifier.cxx \
		np/common.cxx \
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
		spiegel/common.cxx \
		spiegel/filename.cxx \
		spiegel/intercept.cxx \
		spiegel/mapping.cxx \
		spiegel/spiegel.cxx \
		spiegel/tok.cxx \

libnp_PRIVHEADERS= \
		np/common.hxx \
		np_priv.h \
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
		spiegel/filename.hxx \
		spiegel/intercept.hxx \
		spiegel/common.hxx \
		spiegel/mapping.hxx \
		spiegel/spiegel.hxx \
		spiegel/tok.hxx \

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
	$(RM) -r doc/html doc/man
	doxygen

install-local: documentation
	$(INSTALL) -d $(DESTDIR)$(includedir)
	for hdr in $(libnp_HEADERS) ; do \
	    $(INSTALL) -m 644 $$hdr $(DESTDIR)$(includedir)/$$hdr ;\
	done
	$(INSTALL) -d $(DESTDIR)$(libdir)
	$(INSTALL) -m 644 libnp.a $(DESTDIR)$(libdir)/libnp.a
	$(RANLIB) $(DESTDIR)$(libdir)/libnp.a

clean-local:
	$(RM) libnp.a $(libnp_OBJS)
	$(RM) -r doc/html doc/man
	$(RM) -r $(depdir)

check-local:

