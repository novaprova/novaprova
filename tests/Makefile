CC=		gcc
CDEBUGFLAGS=	-g
COPTFLAGS=	-O0
CDEFINES=	-D_GNU_SOURCE
CWARNFLAGS=	-Wall -Wextra -Wno-unused-function -Wno-unused-parameter
CFLAGS=		$(CDEBUGFLAGS) $(COPTFLAGS) $(CWARNFLAGS) \
		$(CDEFINES) $(INCLUDES)

CXX=		g++
CXXFLAGS=	$(CFLAGS)

INCLUDES=	-I..
LIBS=		../libnovaprova.a -lstdc++ -lbfd -ldl -lrt \
		$(shell pkg-config --libs libxml++-2.6)
DEPS=		../np.h ../libnovaprova.a

all install docs:

SIMPLE_TESTS= \
    tintercept \
    treader \
    tfilename \
    tstack \
    t000 \
    t001 \
    t002 \
    t003 \
    t005 \
    t006 \
    t007 \
    t008 \

DUMPERS= \
    tdumpacu \
    tdumpafn \
    tdumpatype \
    tdumpdabbr \
    tdumpdfn \
    tdumpdstruct \
    tdumpdvar \
    tdumpvar \

COMPOUND_TESTS= \
    taddr2line \
    tinfo \
    $(DUMPERS) \

COMPOUND_SUBTESTS= \
    globfunc \
    membfunc \
    namespace \

COMPOUND_DATA= $(patsubst %,d-%.o,$(COMPOUND_SUBTESTS))

tests: $(SIMPLE_TESTS) $(COMPOUND_TESTS) $(COMPOUND_DATA)

# Default to un-verbose
V=0

check: tests run

TESTS= \
    $(SIMPLE_TESTS) \
    $(foreach t,$(COMPOUND_TESTS),$(foreach s,$(COMPOUND_DATA),$t%$s))

run:
	@echo "=== Running tests"
	@e=0 ; [ "$V" -gt 0 ] && export VERBOSE=yes ; for tt in $(TESTS) ; do \
	    OIFS="$$IFS" ; IFS=% ; set -- $$tt ; IFS="$$OIFS" ;\
	    s=runtest.sh ;\
	    [ -f $$1.sh ] && s=$$1.sh ;\
	    env bash $$s $$* || e=1 ;\
	done ; exit $$e

%: %.c fw.a fw.h $(DEPS)
	$(LINK.c) -o $@ $< fw.a $(LIBS)

%: %.cxx fw.a fw.h $(DEPS)
	$(LINK.C) -o $@ $< fw.a $(LIBS)

d-%.o: d-%.cxx
	$(CXX) $(CDEBUGFLAGS) -o $@ -c $<

%.c: %-genc.pl
	perl $< > $@

fw.a: fw.o fw-stubs.o
	ar ruv $@ fw.o fw-stubs.o
	ranlib $@

$(DUMPERS): tdump
	ln -f $< $@

clean:
	$(RM) $(SIMPLE_TESTS) $(COMPOUND_TESTS) $(COMPOUND_DATA)
	$(RM) fw.a fw.o fw-stubs.o

distclean: clean
