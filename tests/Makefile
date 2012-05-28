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
    t000 \
    t001 \
    t002 \
    t003 \
    t005 \
    t006 \
    t007 \
    t008 \
    tintercept \

COMPOUND_TESTS= \
    tinfo \

COMPOUND_SUBTESTS= \
    namespace \

COMPOUND_DATA= $(patsubst %,d-%.o,$(COMPOUND_SUBTESTS))

tests: $(SIMPLE_TESTS) $(COMPOUND_TESTS) $(COMPOUND_DATA) spiegeltest

# Default to verbose for now
V=1

check: tests run_np run_spiegel

run_simple:
	@e=0 ; for t in $(SIMPLE_TESTS) ; do \
	    a= ;\
	    [ "$V" ] && a="--verbose" ;\
	    s=runtest.sh ;\
	    [ -f $$t.sh ] && s=$$t.sh ;\
	    env bash $$s $$a $$t || e=1 ;\
	done ; exit $$e

run_spiegel:
	@e=0 ; for t in $(COMPOUND_TESTS) ; do \
	    for st in $(COMPOUND_SUBTESTS) ; do \
		a= ;\
		[ "$V" ] && a="--verbose" ;\
		s=spiegel.sh ;\
		[ -f $$t.sh ] && s=$$t.sh ;\
		env bash $$s $$a $$t $$st || e=1 ;\
	done ; done ; exit $$e

%: %.c $(DEPS)
	$(LINK.c) -o $@ $< $(LIBS)

%: %.cxx $(DEPS)
	$(LINK.C) -o $@ $< $(LIBS)

d-%.o: d-%.cxx
	$(CXX) $(CDEBUGFLAGS) -o $@ -c $<

%.c: %-genc.pl
	perl $< > $@

clean:
	$(RM) $(TESTS) spiegeltest

distclean: clean
