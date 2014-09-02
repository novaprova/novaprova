/*
 * Copyright 2014 Gregory Banks
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
 *
 * Verify that the mprotect() system call works the way we expect it to.
 * In particular, that it succeeds when given page aligned arguments,
 * and PROT_WRITE can be turned off and on again for heap memory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <setjmp.h>
#include <sys/mman.h>
#include <assert.h>

static volatile unsigned int nsegvs = 0;
static jmp_buf jbuf;
static int trying;

static void handle_segv(int sig)
{
    nsegvs++;
    if (trying)
	longjmp(jbuf, 1);
    fprintf(stderr, "unexpectedly caught signal %d, exiting\n", sig);
    exit(1);
}

#define TRY \
    if ((trying = 1) && !setjmp(jbuf))
#define CATCH \
    else if (!(trying = 0))

// we use a struct to avoid having the functions use any
// global variables, which might use instructions relative
// to %rip, which would cause them to break when moved.
struct foo
{
    volatile int called_a;
    volatile int called_b;
};

void function_a(struct foo *f)
{
    f->called_a++;
}

void post_a_sentinal(void)
{
    printf("I'm never called\n");
}

void function_b(struct foo *f)
{
    f->called_b++;
}

void post_b_sentinal(void)
{
    printf("I'm never called either\n");
}


int main(int argc, char **argv)
{
    int r;
    unsigned long pagesize = sysconf(_SC_PAGESIZE);

    void (*old)(int);
    old = signal(SIGSEGV, handle_segv);
    if (old == SIG_ERR)
    {
	fprintf(stderr, "signal(SIGSEGV) failed: %s\n", strerror(errno));
	return 1;
    }
    old = signal(SIGBUS, handle_segv);
    if (old == SIG_ERR)
    {
	fprintf(stderr, "signal(SIGSEGV) failed: %s\n", strerror(errno));
	return 1;
    }

    printf("Page size is %lu bytes\n", pagesize);

    char *pages = NULL;
    r = posix_memalign((void **)&pages, pagesize, 3*pagesize);
    if (r != 0)
    {
	fprintf(stderr, "posix_memalign() failed: %s\n", strerror(r));
	return 1;
    }
    printf("memalign allocated memory at %p\n", pages);
    if ((unsigned long)pages & (pagesize-1))
    {
	fprintf(stderr, "posix_memalign() returned a pointer not aligned to page size\n");
	return 1;
    }

    printf("testing protection of freshly allocated pages\n");
    nsegvs = 0;
    TRY {
	pages[0] = 'A';
	pages[pagesize-1] = 'B';
	pages[pagesize] = 'C';
	pages[2*pagesize-1] = 'D';
	pages[2*pagesize] = 'E';
	pages[3*pagesize-1] = 'F';
	assert(nsegvs == 0);
	assert(pages[0] == 'A');
	assert(pages[pagesize-1] == 'B');
	assert(pages[pagesize] == 'C');
	assert(pages[2*pagesize-1] == 'D');
	assert(pages[2*pagesize] == 'E');
	assert(pages[3*pagesize-1] == 'F');
	assert(nsegvs == 0);
    } CATCH ;
    printf("ok\n");

    printf("testing turning write permission off for middle page\n");
    r = mprotect(pages+pagesize, pagesize, PROT_READ);
    if (r != 0)
    {
	fprintf(stderr, "mprotect(PROT_READ) failed: %s\n", strerror(errno));
	return 1;
    }
    nsegvs = 0;
    TRY {
	pages[0] = 'G';
	pages[pagesize-1] = 'H';
    } CATCH ;
    assert(nsegvs == 0);
    TRY { pages[pagesize] = 'I'; } CATCH ;
    TRY { pages[2*pagesize-1] = 'J'; } CATCH ;
    assert(nsegvs == 2);
    TRY {
	pages[2*pagesize] = 'K';
	pages[3*pagesize-1] = 'L';
    } CATCH ;
    assert(nsegvs == 2);
    TRY {
	assert(pages[0] == 'G');
	assert(pages[pagesize-1] == 'H');
	assert(pages[pagesize] == 'C');		// write failed
	assert(pages[2*pagesize-1] == 'D');	// write failed
	assert(pages[2*pagesize] == 'K');
	assert(pages[3*pagesize-1] == 'L');
    } CATCH ;
    assert(nsegvs == 2);
    printf("ok\n");

    printf("testing turning write permission back on for middle page\n");
    r = mprotect(pages+pagesize, pagesize, PROT_READ|PROT_WRITE);
    if (r != 0)
    {
	fprintf(stderr, "mprotect(PROT_READ|PROT_WRITE) failed: %s\n", strerror(errno));
	return 1;
    }
    nsegvs = 0;
    TRY {
	pages[0] = 'M';
	pages[pagesize-1] = 'N';
	pages[pagesize] = 'O';
	pages[2*pagesize-1] = 'P';
	pages[2*pagesize] = 'Q';
	pages[3*pagesize-1] = 'R';
    } CATCH ;
    assert(nsegvs == 0);
    TRY {
	assert(pages[0] == 'M');
	assert(pages[pagesize-1] == 'N');
	assert(pages[pagesize] == 'O');		// write failed
	assert(pages[2*pagesize-1] == 'P');	// write failed
	assert(pages[2*pagesize] == 'Q');
	assert(pages[3*pagesize-1] == 'R');
    } CATCH ;
    assert(nsegvs == 0);
    printf("ok\n");

    printf("testing that our functions were compiled where we thought they would be\n");
    assert((unsigned long)function_a < (unsigned long)post_a_sentinal);
    assert((unsigned long)post_a_sentinal < (unsigned long)function_b);
    assert((unsigned long)function_b < (unsigned long)post_b_sentinal);
    unsigned long len_a = (unsigned long)post_a_sentinal - (unsigned long)function_a;
    unsigned long len_b = (unsigned long)post_b_sentinal - (unsigned long)function_b;
    assert(len_a > 0);
    assert(len_b > 0);
    assert(len_b <= len_a);
    assert(len_a <= pagesize);
    printf("ok\n");

    printf("testing that our functions do what we think\n");
    struct foo foo = { 0, 0 };
    function_a(&foo);
    assert(foo.called_a == 1);
    assert(foo.called_b == 0);
    function_b(&foo);
    assert(foo.called_a == 1);
    assert(foo.called_b == 1);
    printf("ok\n");

    printf("testing that the text segment is write-protected by default\n");
    nsegvs = 0;
    TRY { *(char *)function_a = 'S'; } CATCH;
    TRY { *(char *)function_b = 'T'; } CATCH;
    assert(nsegvs == 2);
    printf("ok\n");

    // round down the 1st byte of the function to a page boundary
    unsigned long map_start = (unsigned long)function_a & ~(pagesize-1);
    // round up the byte beyond the end of the function to a page boundary
    unsigned long map_end = (((unsigned long)function_a + len_a) + (pagesize-1)) & ~(pagesize-1);
    printf("rounded function_a boundaries are 0x%lx - 0x%lx\n", map_start, map_end);

    printf("testing turning on write permission for text pages\n");
    r = mprotect((void*)map_start, map_end-map_start, PROT_READ|PROT_WRITE|PROT_EXEC);
    if (r != 0)
    {
	fprintf(stderr, "mprotect(PROT_READ|PROT_WRITE|PROT_EXEC) failed: %s\n", strerror(errno));
	return 1;
    }
    printf("ok\n");

    printf("testing replacing the function_a code with the function_b code\n");
    nsegvs = 0;
    char *saved_a = malloc(len_a);
    assert(saved_a != NULL);
    TRY {
	memcpy(saved_a, function_a, len_a);
	memcpy(function_a, function_b, len_b);
    } CATCH;
    assert(nsegvs == 0);
    printf("ok\n");

    printf("testing that the replaced function code has changed\n");
    TRY {
	foo.called_a = 0;
	foo.called_b = 0;
	function_a(&foo);
	assert(foo.called_a == 0);
	assert(foo.called_b == 1);
	function_b(&foo);
	assert(foo.called_a == 0);
	assert(foo.called_b == 2);
    } CATCH;
    printf("ok\n");

    printf("testing restoring the function_a code\n");
    // Restore the function_a code
    nsegvs = 0;
    TRY { memcpy(function_a, saved_a, len_a); } CATCH;
    assert(nsegvs == 0);
    printf("ok\n");

    printf("testing turning write permission off for text pages\n");
    r = mprotect((void *)map_start, map_end-map_start, PROT_READ|PROT_EXEC);
    if (r != 0)
    {
	fprintf(stderr, "mprotect(PROT_READ|PROT_EXEC) failed: %s\n", strerror(errno));
	return 1;
    }
    printf("ok\n");

    printf("testing that functions are the original code again\n");
    nsegvs = 0;
    TRY {
	foo.called_a = 0;
	foo.called_b = 0;
	function_a(&foo);
	assert(foo.called_a == 1);
	assert(foo.called_b == 0);
	function_b(&foo);
	assert(foo.called_a == 1);
	assert(foo.called_b == 1);
    } CATCH;
    assert(nsegvs == 0);
    printf("ok\n");

    printf("testing that functions are not writable, once again\n");
    nsegvs = 0;
    TRY { *(char *)function_a = 'U'; } CATCH;
    TRY { *(char *)function_b = 'V'; } CATCH;
    assert(nsegvs == 2);
    printf("ok\n");

    return 0;
}
