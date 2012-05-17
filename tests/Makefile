CC=		gcc
CDEBUGFLAGS=	-g
COPTFLAGS=	-O0
CDEFINES=	-D_GNU_SOURCE
CWARNFLAGS=	-Wall -Wextra -Wno-unused-function -Wno-unused-parameter
CFLAGS=		$(CDEBUGFLAGS) $(COPTFLAGS) $(CWARNFLAGS) \
		$(CDEFINES) $(INCLUDES)

INCLUDES=	-I..
LIBS=		-L.. -L../libspiegel -lu4c -lspiegel -lstdc++ -lbfd -ldl -lrt \
		$(shell pkg-config --libs libxml++-2.6)
DEPS=		../u4c.h ../libu4c.a ../libspiegel/libspiegel.a

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

clean:
	$(RM) $(TESTS)
