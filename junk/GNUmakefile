
CFLAGS=	-g

all:	hello osx-setcontext osx-siginfo call-abi.s \
	osx-file-descriptors osx-coredump memleak

call-abi.s: call-abi.c
	$(CC) -S -o $@ $<

hello hello.map: hello.c
	$(CC) -o $@ -Wl,-map,hello.map hello.c

osx-setcontext: osx-setcontext.o fixed_setcontext.o
	$(CC) -o $@ $^

memleak: memleak.c
	$(CC) -g -o $@ $<

run: run-memleak

run-memleak: memleak
	valgrind --leak-check=full ./memleak

clean:
	$(RM) hello osx-setcontext osx-siginfo call-abi.s
	$(RM) osx-file-descriptors osx-coredump memleak
