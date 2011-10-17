CC=		gcc
CDEBUGFLAGS=	-g
COPTFLAGS=	-O0
CDEFINES=	-D_GNU_SOURCE
CWARNFLAGS=	-Wall -Wextra
CFLAGS=		$(CDEBUGFLAGS) $(COPTFLAGS) $(CWARNFLAGS) \
		$(CDEFINES) $(INCLUDES)

INCLUDES=	-I..
LIBS=		-L.. -lu4c -lbfd
DEPS=		../u4c.h ../libu4c.a

TESTS= \
    t000 \
    t001 \

tests: $(TESTS)

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

%.c: %-genc.pl
	perl $< > $@
