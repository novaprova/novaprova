/*
 * Copyright 2011-2014 Gregory Banks
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <np.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <exception>

namespace foo
{

class exception : public std::exception
{
private:
    char *msg_;

public:
    exception() throw() : msg_(0) {}
    exception(const char *m) throw() : msg_(m ? strdup(m) : 0) {}
    ~exception() throw() { free(msg_); }
    const char* what() const throw() { return msg_; }
};

void bar() throw(exception)
{
    throw exception("Oh that went badly");
}

};

static void test_uncaught_exception(void)
{
    fprintf(stderr, "MSG before call to bar\n");
    foo::bar();
    fprintf(stderr, "MSG after call to bar\n");
}

static void test_caught_exception(void)
{
    try
    {
	fprintf(stderr, "MSG before call to bar, in try block\n");
	foo::bar();
	fprintf(stderr, "MSG after call to bar, in try block\n");
    }
    catch (foo::exception &e)
    {
	fprintf(stderr, "MSG caught exception: %s\n", e.what());
    }
}
