
CFLAGS=	-g

all:	hello osx-setcontext osx-siginfo call-abi.s \
	osx-file-descriptors osx-coredump

call-abi.s: call-abi.c
	$(CC) -S -o $@ $<

hello hello.map: hello.c
	$(CC) -o $@ -Wl,-map,hello.map hello.c

osx-setcontext: osx-setcontext.o fixed_setcontext.o
	$(CC) -o $@ $^
