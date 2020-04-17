VALGRIND_PREFIX=/usr/local/Cellar/valgrind/3.10.0

CFLAGS=	-g
CXXFLAGS=   -g

all:	hello osx-setcontext osx-siginfo call-abi.s \
	osx-file-descriptors osx-coredump memleak \
	sigchld sigusr1 except

call-abi.s: call-abi.c
	$(CC) -S -o $@ $<

hello hello.map: hello.c
	$(CC) -o $@ -Wl,-map,hello.map hello.c

osx-siginfo: osx-siginfo.c
	$(CC) -I$(VALGRIND_PREFIX)/include/valgrind -g -o $@ $^

osx-setcontext: osx-setcontext.o fixed_setcontext.o
	$(CC) -o $@ $^

memleak: memleak.c
	$(CC) -g -o $@ $<

run: run-memleak

run-memleak: memleak
	valgrind --leak-check=full ./memleak

run-sigchld: sigchld
	@echo "Running without valgrind"
	./sigchld
	@echo "Running with valgrind"
	valgrind ./sigchld

run-osx-siginfo:
	./osx-siginfo
	$(VALGRIND_PREFIX)/bin/valgrind ./osx-siginfo

except: except.o iexcept.o
	$(LINK.C) -o $@ $^

osx-writable: osx-writable.c
	$(LINK.c) -o $@ $^

%.o: %.cxx
	$(COMPILE.C) -o $@ $^

%.s:	%.cxx
	$(CXX) $(CXXFLAGS) -g -S -o $@ $^

clean:
	$(RM) hello osx-setcontext osx-siginfo call-abi.s
	$(RM) osx-file-descriptors osx-coredump memleak
	$(RM) sigchld sigusr1 except osx-writable
