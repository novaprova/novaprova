#ifndef __U4C_PRIV_H__
#define __U4C_PRIV_H__ 1

#include "common.h"
#include "u4c.h"
#include "spiegel/spiegel.hxx"
#include "spiegel/dwarf/state.hxx"
#include "spiegel/filename.hxx"
#include <bfd.h>
#include <sys/poll.h>
#include <vector>
#include <list>

struct u4c_event_t;
struct u4c_function_t;

#include "u4c/types.hxx"
#include "u4c/classifier.hxx"
#include "u4c/child.hxx"
#include "u4c/testnode.hxx"
#include "u4c/listener.hxx"
#include "u4c/text_listener.hxx"
#include "u4c/proxy_listener.hxx"
#include "u4c/plan.hxx"
#include "u4c/testmanager.hxx"
#include "u4c/runner.hxx"

/* run.c */
#define __u4c_merge(r1, r2) \
    do { \
	u4c::result_t _r1 = (r1), _r2 = (u4c::result_t)(r2); \
	(r1) = (_r1 > _r2 ? _r1 : _r2); \
    } while(0)


#endif /* __U4C_PRIV_H__ */
