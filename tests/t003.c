#include <syslog.h>
#include <unistd.h>
#include <stdlib.h>
#include <u4c.h>

#define m1	1
#define m2	2

static void test_invalid_regex(void)
{
    u4c_syslog_match("[foo", m1);	    /* fail */
    syslog(LOG_ERR, "fnarp");
    U4C_ASSERT_EQUAL(u4c_syslog_count(m1), 0);
}

static void test_unmatched_tag(void)
{
    int x = u4c_syslog_count(m1);	/* fail */
    U4C_ASSERT_EQUAL(x, 0);
}

static void test_no_messages(void)
{
    /* no syslog messages => count is 0 */
    u4c_syslog_match("foo.*baz", m1);
    U4C_ASSERT_EQUAL(u4c_syslog_count(m1), 0);
    /* pass */
}

static void test_one_message_no_matches(void)
{
    /* one syslog message which doesn't match => unmatched
     * messages FAIL the test */
    syslog(LOG_ERR, "fnarp");	    /* fail */
}

static void test_one_message_unmatched(void)
{
    /* one syslog message which doesn't match => unmatched
     * messages FAIL the test */
    u4c_syslog_match("foo.*baz", m1);
    syslog(LOG_ERR, "fnarp");	    /* fail */
    U4C_ASSERT_EQUAL(u4c_syslog_count(m1), 0);
}

static void test_one_message_ignored(void)
{
    /* one syslog message which matches an SL_IGNORE */
    u4c_syslog_ignore("fna*");
    syslog(LOG_ERR, "fnarp");
    /* pass */
}

static void test_one_message_one_match(void)
{
    /* one syslog message which does match => count is 1,
     * both macros succeed */
    u4c_syslog_match("foo.*baz", m1);
    syslog(LOG_ERR, "foo bar baz");
    U4C_ASSERT_EQUAL(u4c_syslog_count(m1), 1);
    /* pass */
}

static void test_one_message_one_match_want_five(void)
{
    /* one syslog message which does match => count is 1,
     * we check for 5 */
    u4c_syslog_match("foo.*baz", m1);
    syslog(LOG_ERR, "foo bar baz");
    U4C_ASSERT_EQUAL(u4c_syslog_count(m1), 5);	/* fail */
}

static void test_one_message_multiple_matches_same_tag(void)
{
    /* one syslog message with multiple matches => count is 1 */
    u4c_syslog_match("fuu.*bas", m1);
    u4c_syslog_match("bleah", m1);
    syslog(LOG_ERR, "fuu bleah bas");
    U4C_ASSERT_EQUAL(u4c_syslog_count(m1), 1);
    /* pass */
}

static void test_one_message_multiple_matches_different_tags(void)
{
    /* one syslog message with multiple matches which are tracked
     * separately => count is 1 */
    u4c_syslog_match("fuu.*bas", m1);
    u4c_syslog_match("bleah", m2);
    syslog(LOG_ERR, "fuu bleah bas");
    U4C_ASSERT_EQUAL(u4c_syslog_count(m1), 1);
    U4C_ASSERT_EQUAL(u4c_syslog_count(m2), 0);
    /* pass */
}

int
main(int argc, char **argv)
{
    int ec = 0;
    u4c_runner_t *runner = u4c_init();
    ec = u4c_run_tests(runner, 0);
    u4c_done(runner);
    exit(ec);
}
