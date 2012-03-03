#ifndef __U4C_CLASSIFIER_H__
#define __U4C_CLASSIFIER_H__ 1

#include "common.h"
#include <regex.h>

namespace u4c {

class classifier_t
{
public:
    static void *operator new(size_t sz) { return xmalloc(sz); }
    static void operator delete(void *x) { free(x); }

    classifier_t()
    {
	results_[0] = false;
	results_[1] = true;
    }
    ~classifier_t()
    {
	xfree(re_);
	regfree(&compiled_re_);
    }

    bool set_regexp(const char *, bool);
    void set_results(int failed, int matched)
    {
	results_[0] = failed;
	results_[1] = matched;
    }
    int classify(const char *, char *, size_t) const;

private:
    char *re_;
    regex_t compiled_re_;
    int results_[2];
};

// close the namespace
};

#endif /* __U4C_CLASSIFIER_H__ */

