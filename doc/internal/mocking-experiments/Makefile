
PROGRAMS= \
    foo \
    bar \
    baz \
    quux \

all: $(PROGRAMS)

CC=	gcc
CFLAGS=	-Wall -g

%: %.c
	$(LINK.c) -o $@ $<

clean:
	$(RM) $(PROGRAMS)
