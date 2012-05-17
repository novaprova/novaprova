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
LIBS=		-L.. -lnp -lstdc++ -lbfd -ldl -lrt \
		$(shell pkg-config --libs libxml++-2.6)
DEPS=		../np.h ../libnp.a

all install:

TESTS= \
    t000 \
    t001 \
    t002 \
    t003 \
    t005 \
    t006 \
    t007 \
    t008 \

tests: $(TESTS) spiegeltest

# Default to verbose for now
V=1

check: $(TESTS)
	@e=0 ; for t in $(TESTS) ; do \
	    a= ;\
	    [ "$V" ] && a="--verbose" ;\
	    s=runtest.sh ;\
	    [ -f $$t.sh ] && s=$$t.sh ;\
	    env bash $$s $$a $$t || e=1 ;\
	done ; exit $$e

%: %.c $(DEPS)
	$(LINK.c) -o $@ $< $(LIBS)

%: %.cxx $(DEPS)
	$(LINK.C) -o $@ $< $(LIBS)

%.c: %-genc.pl
	perl $< > $@

clean:
	$(RM) $(TESTS) spiegeltest
