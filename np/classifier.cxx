
#include "classifier.hxx"

namespace np {

const char *
classifier_t::error_string() const
{
    static char buf[2048];

    if (!error_)
	return 0;

    int n = snprintf(buf, sizeof(buf)-100, "/%s/: ", re_);
    regerror(error_, &compiled_re_, buf+n, sizeof(buf)-n-1);

    return buf;
}

bool
classifier_t::set_regexp(const char *re, bool case_sensitive)
{
    re_ = np::util::xstrdup(re);
    error_ = regcomp(&compiled_re_, re,
		    REG_EXTENDED|(case_sensitive ? 0 : REG_ICASE));
    if (error_)
    {
	fprintf(stderr, "np: bad classifier %s\n", error_string());
	return false;
    }
    return true;
}

int
classifier_t::classify(const char *func,
		       char *match_return,
		       size_t maxmatch) const
{
    regmatch_t match[2];
    int r = regexec(&compiled_re_, func, 2, match, 0);

    if (r == 0)
    {
// fprintf(stderr, "MATCHED \"%s\" to \"%s\"\n", func, cl->re);
// fprintf(stderr, "    submatch [ {%d %d} {%d %d}]\n",
// 	    match[0].rm_so, match[0].rm_eo,
// 	    match[1].rm_so, match[1].rm_eo);

	/* successful match */
	if (match_return && match[1].rm_so >= 0)
	{
	    size_t len = match[1].rm_eo - match[1].rm_so;
	    if (len >= maxmatch)
	    {
		fprintf(stderr, "np: match for classifier %s too long\n",
				re_);
		return results_[0];
	    }
	    memcpy(match_return, func+match[1].rm_so, len);
	    match_return[len] = '\0';
// fprintf(stderr, "    match_return \"%s\"\n", match_return);
	}
	return results_[1];
    }

    if (r != REG_NOMATCH)
    {
	/* some runtime error */
	fprintf(stderr, "np: failed matching \"%s\": %s\n",
		func, error_string());
    }

    return results_[0];
}

// close the namespace
};
