
prefix=		/usr/local
includedir=	$(prefix)/include
libdir=		$(prefix)/lib

CC=		g++
CXX=		g++
CDEBUGFLAGS=	-g
COPTFLAGS=	-O0
CDEFINES=	-D_GNU_SOURCE -Ilibspiegel -I.
CWARNFLAGS=	-Wall -Wextra
CFLAGS=		$(CDEBUGFLAGS) $(COPTFLAGS) $(CWARNFLAGS) $(CDEFINES)
CXXFLAGS=	$(CFLAGS)
INSTALL=	/usr/bin/install -c
RANLIB=		ranlib
depdir=		.deps

all: libu4c.a

libu4c_SOURCE=	\
		u4c.c \
		isyslog.c iassert.c icunit.c iexit.c uasserts.c \
		u4c/child.cxx \
		u4c/classifier.cxx \
		u4c/common.cxx \
		u4c/plan.cxx \
		u4c/proxy_listener.cxx \
		u4c/runner.cxx \
		u4c/testmanager.cxx \
		u4c/testnode.cxx \
		u4c/text_listener.cxx \
		u4c/types.cxx \

libu4c_PRIVHEADERS= \
		u4c/common.hxx \
		u4c_priv.h \

libu4c_HEADERS=	u4c.h \
		u4c/child.hxx \
		u4c/classifier.hxx \
		u4c/listener.hxx \
		u4c/plan.hxx \
		u4c/proxy_listener.hxx \
		u4c/runner.hxx \
		u4c/testmanager.hxx \
		u4c/testnode.hxx \
		u4c/text_listener.hxx \
		u4c/types.hxx \

libu4c_OBJS=	\
	$(patsubst %.c,%.o,$(filter %.c,$(libu4c_SOURCE))) \
	$(patsubst %.cxx,%.o,$(filter %.cxx,$(libu4c_SOURCE)))
#
# Automatic dependency tracking
libu4c_DFILES= \
	$(patsubst %.c,$(depdir)/%.d,$(filter %.c,$(libu4c_SOURCE))) \
	$(patsubst %.cxx,$(depdir)/%.d,$(filter %.cxx,$(libu4c_SOURCE))) \

-include $(libu4c_DFILES)

%.o: %.cxx
	$(COMPILE.C) -o $@ $<

$(depdir)/%.d: %.cxx
	@[ -d $(@D) ] || mkdir -p $(@D)
	$(COMPILE.C) -MM -MF $@ -MT $(patsubst %.cxx,%.o,$<) $<

$(depdir)/%.d: %.c
	@[ -d $(@D) ] || mkdir -p $(@D)
	$(COMPILE.c) -MM -MF $@ -MT $(patsubst %.c,%.o,$<) $<

libu4c.a: $(libu4c_OBJS)
	$(AR) $(ARFLAGS) libu4c.a $(libu4c_OBJS)

install: all
	$(INSTALL) -d $(DESTDIR)$(includedir)
	for hdr in $(libu4c_HEADERS) ; do \
	    $(INSTALL) -m 644 $$hdr $(DESTDIR)$(includedir)/$$hdr ;\
	done
	$(INSTALL) -d $(DESTDIR)$(libdir)
	$(INSTALL) -m 644 libu4c.a $(DESTDIR)$(libdir)/libu4c.a
	$(RANLIB) $(DESTDIR)$(libdir)/libu4c.a

clean:
	$(RM) libu4c.a $(libu4c_OBJS)
	$(RM) -r $(depdir)

check: all
	cd tests ; $(MAKE) $@

