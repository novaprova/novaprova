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
#include "np/util/common.hxx"
#include "np/util/trace.hxx"
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

namespace np {
namespace trace {

char *buf = 0;
#define BUFFER_SIZE (16*1024*1024)
const char hexdigits[] = "0123456789abcdef";

void init()
{
#if _NP_ENABLE_TRACE
    if (buf != 0)
	return;	    // already open

    static const char filename[] = "novaprova.trace.txt";
    int fd = open(filename, O_RDWR|O_CREAT|O_TRUNC, 0666);
    if (fd < 0)
    {
	perror(filename);
	exit(1);
    }

    int r = ftruncate(fd, BUFFER_SIZE);
    if (r < 0)
    {
	perror("ftruncate");
	exit(1);
    }

    buf = (char *)mmap(NULL, BUFFER_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (buf == (char *)MAP_FAILED)
    {
	perror("mmap");
	exit(1);
    }
    close(fd);

    memset(buf, '\n', BUFFER_SIZE);
    fprintf(stderr, "np: tracing to %s\n", filename);
#endif
}

// close the namespaces
}; };
