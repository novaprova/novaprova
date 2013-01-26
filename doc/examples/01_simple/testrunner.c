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

#include <stdlib.h>
#include <unistd.h>
#include <np.h>	    /* NovaProva library */

int
main(int argc, char **argv)
{
    int ec = 0;
    np_runner_t *runner;

    /* Initialise the NovaProva library */
    runner = np_init();

    /* Run all the discovered tests */
    ec = np_run_tests(runner, NULL);

    /* Shut down the NovaProva library */
    np_done(runner);

    exit(ec);
}

