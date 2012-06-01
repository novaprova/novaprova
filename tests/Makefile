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

all install documentation:

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

# Default to verbose for now
V=1

check: tests run_simple run_compound

run_simple:
	@e=0 ; for t in $(SIMPLE_TESTS) ; do \
	    a= ;\
	    [ "$V" ] && a="--verbose" ;\
	    s=runtest.sh ;\
	    [ -f $$t.sh ] && s=$$t.sh ;\
	    env bash $$s $$a $$t || e=1 ;\
	done ; exit $$e

run_compound:
	@e=0 ; for t in $(COMPOUND_TESTS) ; do \
	    for st in $(COMPOUND_SUBTESTS) ; do \
		a= ;\
		[ "$V" ] && a="--verbose" ;\
		s=runcompound.sh ;\
		[ -f $$t.sh ] && s=$$t.sh ;\
		env bash $$s $$a $$t $$st || e=1 ;\
	    done ;\
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
	$(RM) $@
	ln $< $@

clean:
	$(RM) $(SIMPLE_TESTS) $(COMPOUND_TESTS) $(COMPOUND_DATA)
	$(RM) fw.a fw.o fw-stubs.o

distclean: clean
