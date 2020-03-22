/*
 * Copyright 2011-2013 Gregory Banks
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
#include <sys/fcntl.h>

static int the_fd = -1;

/*
 * For portability we ensure the file descriptors reported in the NP
 * output are completely predictable rather than relying on the fd
 * number returned naturally from open().
 */
static int opento(const char *filename, int mode, int flags, int tofd)
{
    int tmpfd, retfd;
    tmpfd = open(filename, flags, mode);
    if (tmpfd < 0)
	return -1;
    retfd = dup2(tmpfd, tofd);	// could be -1
    close(tmpfd);
    return retfd;
}

static void test_leaky_test(void)
{
    fprintf(stderr, "MSG leaking fd for .leaky_test.dat\n");
    opento(".leaky_test.dat", O_WRONLY|O_CREAT, 0666, 20);
}

static int set_up(void)
{
    the_fd = opento(".leaky_fixture.dat", O_WRONLY|O_CREAT, 0666, 21);
    return 0;
}

static int tear_down(void)
{
    if (the_fd >= 0)
    {
	close(the_fd);
	the_fd = -1;
    }
    return 0;
}

static void test_leaky_fixture(void)
{
    fprintf(stderr, "MSG leaking fd for .leaky_fixture.dat\n");
    the_fd = -1;
}

