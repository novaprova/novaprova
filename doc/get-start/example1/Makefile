
all: libmycode.a

MYCODE_SOURCE=	mycode.c
MYCODE_OBJS=	$(MYCODE_SOURCE:.c=.o)

libmycode.a: $(MYCODE_OBJS)
	ar ruv $@ $(MYCODE_OBJS)
	ranlib $@

NOVAPROVA_CFLAGS= $(shell pkg-config --cflags novaprova)
NOVAPROVA_LIBS= $(shell pkg-config --libs novaprova)

CFLAGS= -g $(NOVAPROVA_CFLAGS)

check: testrunner
	./testrunner

TEST_SOURCE= mytest.c
TEST_OBJS=  $(TEST_SOURCE:.c=.o)

testrunner:  testrunner.o $(TEST_OBJS) libmycode.a
	$(LINK.c) -o $@ testrunner.o $(TEST_OBJS) libmycode.a $(NOVAPROVA_LIBS)

