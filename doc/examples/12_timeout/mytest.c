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
#include <stdlib.h>

void test_slow(void)
{
    int i;
    fprintf(stderr, "Test runs for 100 seconds\n");
    for (i = 0 ; i < 100 ; i += 10) {
	fprintf(stderr, "Have been running for %d sec\n", i);
	sleep(10);
    }
    fprintf(stderr, "Test ends after %d sec\n", i);
}

