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

#include <syslog.h>
#include <np.h>	    /* NovaProva library */

static void test_unexpected(void)
{
    syslog(LOG_ERR, "This message shouldn't happen");
}

static void test_expected(void)
{
    np_syslog_ignore("entirely expected");
    syslog(LOG_ERR, "This message was entirely expected");
}

