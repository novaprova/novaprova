/*
 * Copyright 2020 Chris Packham, Gregory Banks
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
 * Test auto-mocking of libc fopen().  This currently fails because:
 *
 * - glibc's fopen() is not always called that at the symbol level, and
 * - the debug_info for libc is usually stripped, and
 * - NovaProva doesn't know how to read separate debuginfo
 * 
 * based on test code contributed by cpackham https://github.com/novaprova/novaprova/issues/34
 */
#include <np.h>
#include <stdio.h>


static int mock_fopen_called = 0;
static int mock_fclose_called = 0;

FILE *mock_fopen(const char *path, const char *mode)
{
    mock_fopen_called++;
    return (FILE *) 1;
}

int mock_fclose(FILE *fp)
{
    mock_fclose_called++;
    return 0;
}

int myfopen(char *path)
{
    FILE *fp = fopen(path, "r+");

    if (fp)
    {
        printf("fp = %p\n", fp);
        fclose(fp);
        return 0;
    }

    return -1;
}

void test_fopen(void)
{
    int r;

    NP_ASSERT_EQUAL(mock_fopen_called, 0);
    NP_ASSERT_EQUAL(mock_fclose_called, 0);
    r = myfopen("foo");
    NP_ASSERT_EQUAL(r, 0);
    NP_ASSERT_EQUAL(mock_fopen_called, 1);
    NP_ASSERT_EQUAL(mock_fclose_called, 1);
}
