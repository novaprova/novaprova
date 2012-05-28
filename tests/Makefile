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

NP_TESTS= \
    t000 \
    t001 \
    t002 \
    t003 \
    t005 \
    t006 \
    t007 \
    t008 \

SPIEGEL_TESTS= \
    tinfo \

SPIEGEL_SUBTESTS= \
    namespace \

SPIEGEL_DATA= $(patsubst %,d-%.o,$(SPIEGEL_SUBTESTS))

tests: $(NP_TESTS) $(SPIEGEL_TESTS) $(SPIEGEL_DATA) spiegeltest

# Default to verbose for now
V=1

check: tests run_np run_spiegel

run_np:
	@e=0 ; for t in $(NP_TESTS) ; do \
	    a= ;\
	    [ "$V" ] && a="--verbose" ;\
	    s=runtest.sh ;\
	    [ -f $$t.sh ] && s=$$t.sh ;\
	    env bash $$s $$a $$t || e=1 ;\
	done ; exit $$e

run_spiegel:
	@e=0 ; for t in $(SPIEGEL_TESTS) ; do \
	    for st in $(SPIEGEL_SUBTESTS) ; do \
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
