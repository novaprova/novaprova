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

tests: $(TESTS)

check: $(TESTS)
	for t in $(TESTS) ; do \
	    s=runtest.sh ;\
	    [ -f $$t.sh ] && s=$$t.sh ;\
	    env bash $$s $$t ;\
	done

%: %.c $(DEPS)
	$(LINK.c) -o $@ $< $(LIBS)

