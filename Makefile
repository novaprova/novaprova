
prefix=		/usr/local
includedir=	$(prefix)/include
libdir=		$(prefix)/lib

CC=		g++
CXX=		g++
CDEBUGFLAGS=	-g
COPTFLAGS=	-O0
CDEFINES=	-D_GNU_SOURCE -Ilibspiegel -I. \
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

libnp_PRIVHEADERS= \
		np/common.hxx \
		np_priv.h \

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

