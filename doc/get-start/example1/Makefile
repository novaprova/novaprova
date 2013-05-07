#
#  Copyright 2011-2012 Gregory Banks
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#

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

testrunner:  $(TEST_OBJS) libmycode.a
	$(LINK.c) -o $@ $(TEST_OBJS) libmycode.a $(NOVAPROVA_LIBS)

clean:
	$(RM) testrunner libmycode.a $(TEST_OBJS) $(MYCODE_OBJS)
