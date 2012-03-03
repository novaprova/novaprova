
#include "classifier.hxx"

namespace u4c {

bool
classifier_t::set_regexp(const char *re, bool case_sensitive)
{
    re_ = xstrdup(re);
    int r = regcomp(&compiled_re_, re,
		    REG_EXTENDED|(case_sensitive ? 0 : REG_ICASE));
    if (r)
    {
	char err[1024];
	regerror(r, &compiled_re_, err, sizeof(err));
	fprintf(stderr, "u4c: bad classifier %s: %s\n",
		re, err);
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
		fprintf(stderr, "u4c: match for classifier %s too long\n",
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
	char err[1024];
	regerror(r, &compiled_re_, err, sizeof(err));
	fprintf(stderr, "u4c: failed matching \"%s\": %s\n",
		func, err);
    }

    return results_[0];
}

// close the namespace
};
