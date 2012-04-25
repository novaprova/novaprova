/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2005 Greg Banks <gnb@users.sourceforge.net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "spiegel/common.hxx"

using namespace std;
namespace spiegel {

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define safestr(s)  ((s) == 0 ? "" : (s))
int safe_strcmp(const char *a, const char *b)
{
    return strcmp(safestr(a), safestr(b));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int u32cmp(uint32_t ul1, uint32_t ul2);
int u64cmp(uint64_t ull1, uint64_t ull2);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const char *argv0;

void
fatal(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    fprintf(stderr, "%s: ", argv0);
    vfprintf(stderr, fmt, args);
    fputc('\n', stderr);
    fflush(stderr);
    va_end(args);

    exit(1);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void oom(void) __attribute__((noreturn));

/* Do write() but handle short writes */
static int
do_write(int fd, const char *buf, int len)
{
    int remain = len;

    while (remain > 0)
    {
	int n = write(fd, buf, remain);
	if (n < 0)
	    return -1;
	buf += n;
	remain -= n;
    }
    return len;
}

static void
oom(void)
{
    static const char message[] = ": no memory, exiting\n";

    if (do_write(2, argv0, strlen(argv0)) < 0)
	exit(1);
    if (do_write(2, message, sizeof(message)-1) < 0)
	exit(1);
    exit(1);
}

void *
xmalloc(size_t sz)
{
    void *x;

    if ((x = malloc(sz)) == 0)
	oom();
    memset(x, 0, sz);
    return x;
}

char *
xstrdup(const char *s)
{
    size_t len;
    char *x;

    if (!s)
	return 0;
    len = strlen(s);
    if ((x = (char *)malloc(len+1)) == 0)
	oom();
    strcpy(x, s);
    return x;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
u32cmp(uint32_t ul1, uint32_t ul2)
{
    if (ul1 > ul2)
	return 1;
    if (ul1 < ul2)
	return -1;
    return 0;
}

int
u64cmp(uint64_t ull1, uint64_t ull2)
{
    if (ull1 > ull2)
	return 1;
    if (ull1 < ull2)
	return -1;
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

unsigned long
page_size(void)
{
    static unsigned long ps;
    if (!ps)
	ps = sysconf(_SC_PAGESIZE);
    return ps;
}

unsigned long
page_round_up(unsigned long x)
{
    unsigned long ps = page_size();
    return ((x + ps - 1) / ps) * ps;
}

unsigned long
page_round_down(unsigned long x)
{
    unsigned long ps = page_size();
    return (x / ps) * ps;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

string hex(unsigned long x)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "0x%lx", x);
    return string(buf);
}

string dec(unsigned int x)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%u", x);
    return string(buf);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

// close the namespace 
};

/*END*/
