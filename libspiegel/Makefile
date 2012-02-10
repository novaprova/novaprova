
prefix=		/usr/local
includedir=	$(prefix)/include
libdir=		$(prefix)/lib

CC=		gcc
CXX=		g++
CDEBUGFLAGS=	-g
CXXDEBUGFLAGS=	-g
COPTFLAGS=	-O0
CXXOPTFLAGS=	-O0
CDEFINES=	-D_GNU_SOURCE -I.
CXXDEFINES=	-D_GNU_SOURCE -I.
CWARNFLAGS=	-Wall -Wextra
CXXWARNFLAGS=	-Wall -Wextra
CFLAGS=		$(CDEBUGFLAGS) $(COPTFLAGS) $(CWARNFLAGS) $(CDEFINES)
CXXFLAGS=	$(CXXDEBUGFLAGS) $(CXXOPTFLAGS) $(CXXWARNFLAGS) $(CXXDEFINES)
INSTALL=	/usr/bin/install -c
RANLIB=		ranlib

all: testrunner

libspiegel= libspiegel.a

libspiegel_SOURCES= \
	spiegel/dwarf/abbrev.cxx \
	spiegel/dwarf/compile_unit.cxx \
	spiegel/dwarf/entry.cxx \
	spiegel/dwarf/enumerations.cxx \
	spiegel/dwarf/state.cxx \
	spiegel/dwarf/string_table.cxx \
	spiegel/dwarf/value.cxx \
	spiegel/dwarf/walker.cxx \
	spiegel/common.c \
	spiegel/commonp.cxx \

libspiegel_PRIVHEADERS= \
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
	spiegel/common.h \
	spiegel/commonp.hxx \

libspiegel_HEADERS= \

libspiegel_OBJS= \
	$(patsubst %.c,%.o,$(filter %.c,$(libspiegel_SOURCES))) \
	$(patsubst %.cxx,%.o,$(filter %.cxx,$(libspiegel_SOURCES)))

$(libspiegel): $(libspiegel_OBJS)
	$(AR) $(ARFLAGS) $(libspiegel) $(libspiegel_OBJS)

spiegel/dwarf/%.o: spiegel/dwarf/%.c
	$(COMPILE.c) -o $@ $<
spiegel/dwarf/%.o: spiegel/dwarf/%.cxx
	$(COMPILE.C) -o $@ $<

spiegel/%.o: spiegel/%.c
	$(COMPILE.c) -o $@ $<
spiegel/%.o: spiegel/%.cxx
	$(COMPILE.C) -o $@ $<

%.o: %.c
	$(COMPILE.c) -o $@ $<
%.o: %.cxx
	$(COMPILE.C) -o $@ $<

testrunner_SOURCES= \
	testrunner.cxx

testrunner_OBJS= \
	$(patsubst %.c,%.o,$(filter %.c,$(testrunner_SOURCES))) \
	$(patsubst %.cxx,%.o,$(filter %.cxx,$(testrunner_SOURCES)))

testrunner_LDADD= \
	$(libspiegel) -lbfd

testrunner: $(testrunner_OBJS) $(libspiegel)
	$(LINK.C) -o testrunner $(testrunner_OBJS) $(testrunner_LDADD)

# install: all
# 	$(INSTALL) -d $(DESTDIR)$(includedir)
# 	for hdr in $(libu4c_HEADERS) ; do \
# 	    $(INSTALL) -m 644 $$hdr $(DESTDIR)$(includedir)/$$hdr ;\
# 	done
# 	$(INSTALL) -d $(DESTDIR)$(libdir)
# 	$(INSTALL) -m 644 libu4c.a $(DESTDIR)$(libdir)/libu4c.a
# 	$(RANLIB) $(DESTDIR)$(libdir)/libu4c.a

clean:
	$(RM) testrunner $(testrunner_OBJS)
	$(RM) $(libspiegel) $(libspiegel_OBJS)

