/*
 * Copyright 2011-2021 Gregory Banks
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
#include <unistd.h>
#include <stdlib.h>
#include <np.h>

#define m1	1
#define m2	2

/* silly words courtesy hipsum.co */

static NP_USED void test_invalid_regex(void)
{
    np_syslog_match("[edison", m1);	    /* fail */
    syslog(LOG_ERR, "hoodie");
    NP_ASSERT_EQUAL(np_syslog_count(m1), 0);
}

static NP_USED void test_unmatched_tag(void)
{
    int x = np_syslog_count(m1);	/* fail */
    NP_ASSERT_EQUAL(x, 0);
}

static NP_USED void test_no_messages(void)
{
    /* no syslog messages => count is 0 */
    np_syslog_match("chia.*bushwick", m1);
    NP_ASSERT_EQUAL(np_syslog_count(m1), 0);
    /* pass */
}

static NP_USED void test_one_message_no_matches(void)
{
    /* one syslog message which doesn't match => unmatched
     * messages FAIL the test */
    syslog(LOG_ERR, "stumptown");	    /* fail */
}

static NP_USED void test_one_message_unmatched(void)
{
    /* one syslog message which doesn't match => unmatched
     * messages FAIL the test */
    np_syslog_match("tacos.*listicle", m1);
    syslog(LOG_ERR, "retro");	    /* fail */
    NP_ASSERT_EQUAL(np_syslog_count(m1), 0);
}

static NP_USED void test_one_message_ignored(void)
{
    /* one syslog message which matches an SL_IGNORE */
    np_syslog_ignore("wha*");
    syslog(LOG_ERR, "whatever");
    /* pass */
}

static NP_USED void test_one_message_one_match(void)
{
    /* one syslog message which does match => count is 1,
     * both macros succeed */
    np_syslog_match("dream.*lyn", m1);
    syslog(LOG_ERR, "dreamcatcher prism brooklyn");
    NP_ASSERT_EQUAL(np_syslog_count(m1), 1);
    /* pass */
}

static NP_USED void test_one_message_one_match_want_five(void)
{
    /* one syslog message which does match => count is 1,
     * we check for 5 */
    np_syslog_match("kog.*ock", m1);
    syslog(LOG_ERR, "kogi humblebrag hammock");
    NP_ASSERT_EQUAL(np_syslog_count(m1), 5);	/* fail */
}

static NP_USED void test_one_message_multiple_matches_same_tag(void)
{
    /* one syslog message with multiple matches => count is 1 */
    np_syslog_match("cra.*ape", m1);
    np_syslog_match("flannel", m1);
    syslog(LOG_ERR, "cray flannel vape");
    NP_ASSERT_EQUAL(np_syslog_count(m1), 1);
    /* pass */
}

static NP_USED void test_one_message_multiple_matches_different_tags(void)
{
    /* one syslog message with multiple matches which are tracked
     * separately => count is 1 */
    np_syslog_match("corn.*gon", m1);
    np_syslog_match("hashtag", m2);
    syslog(LOG_ERR, "cornhole hashtag hexagon");
    NP_ASSERT_EQUAL(np_syslog_count(m1), 1);
    NP_ASSERT_EQUAL(np_syslog_count(m2), 0);
    /* pass */
}

