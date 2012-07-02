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

all install docs:

BASIC_TESTS= \
    tnassert \
    tnexit \
    tnfail \
    tnmemleak \
    tnna \
    tnpass \
    tnsegv \
    tnsigill \
    tnsyslog \

OUTPUT_FORMATS= \
    junit

SIMPLE_TESTS= \
    tnaequalfail \
    tnaequalpass \
    tnafail \
    tnafalsefail \
    tnafalsepass \
    tnanequalfail \
    tnanequalpass \
    tnannullfail \
    tnannullpass \
    tnanullfail \
    tnanullpass \
    tnapass \
    tnapequalfail \
    tnapequalpass \
    tnapnequalfail \
    tnapnequalpass \
    tnasequalfail \
    tnasequalpass \
    tnasnequalfail \
    tnasnequalpass \
    tnatruefail \
    tnmocking \
    tnparameter \
    tnsyslogmatch \
    tntimeout \

PARALLEL_TESTS= \
    tnparallel \

MAINFUL_TESTS= \
    tfilename \
    tintercept \
    treader \
    tstack \

DUMPERS= \
    tdumpacu \
    tdumpafn \
    tdumpatype \
    tdumpdabbr \
    tdumpdfn \
    tdumpdstruct \
    tdumpdvar \
    tdumpvar \

COMPOUND_TESTS= \
    taddr2line \
    tinfo \
    $(DUMPERS) \

COMPOUND_SUBTESTS= \
    globfunc \
    membfunc \
    namespace \

COMPOUND_DATA= $(patsubst %,d-%.o,$(COMPOUND_SUBTESTS))

TESTS= \
    $(SIMPLE_TESTS) \
    $(foreach t,$(PARALLEL_TESTS),$t $(foreach j,1 2 4,$t%-j$j)) \
    $(foreach t,$(BASIC_TESTS),$t $(foreach s,$(OUTPUT_FORMATS),$t%-f$s)) \
    $(MAINFUL_TESTS) \
    $(foreach t,$(COMPOUND_TESTS),$(foreach s,$(COMPOUND_DATA),$t%$s))

# Extract only the test executables actually mentioned in $TESTS
# which allows us to build only those executables actually needed
# to run the tests named in $TESTS.
TEST_EXES= $(sort $(foreach t,$(TESTS),$(firstword $(subst %,$(nul) $(nul),$t))))

tests: $(TEST_EXES) $(COMPOUND_DATA)

# Default to un-verbose
V=0

check: tests run

list:
	@for t in $(TESTS) ; do \
	    echo "$$t" | tr '%' ' ' ;\
	done

run: .announce-run $(addprefix .run%,$(TESTS))

.PHONEY: .announce-run
.announce-run:
	@echo "=== Running tests"

.run%:
	@[ "$V" -gt 0 ] && export VERBOSE=yes ; env bash runtest.sh $(wordlist 2,10,$(subst %,$(nul) $(nul),$@))

%: %.c fw.a fw.h $(DEPS)
	$(LINK.c) -o $@ $< fw.a $(LIBS)

%: %.cxx fw.a fw.h $(DEPS)
	$(LINK.C) -o $@ $< fw.a $(LIBS)

d-%.o: d-%.cxx
	$(CXX) $(CDEBUGFLAGS) -o $@ -c $<

%.c: %-genc.pl
	perl $< > $@

fw.a: fw.o fw-stubs.o
	ar ruv $@ fw.o fw-stubs.o
	ranlib $@

$(DUMPERS): tdump
	ln -f $< $@

$(SIMPLE_TESTS) $(BASIC_TESTS) $(PARALLEL_TESTS): % : %.c main.o $(DEPS)
	$(LINK.c) -o $@ $< main.o $(LIBS)

clean:
	$(RM) $(TEST_EXES) $(COMPOUND_DATA)
	$(RM) fw.a fw.o fw-stubs.o

distclean: clean
