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

#include <np.h>
#include <getopt.h>
#include <sys/stat.h>
#include "np/testnode.hxx"
#include "np/testmanager.hxx"
#include "np/plan.hxx"
#include "np/util/filename.hxx"
#include "np/util/daemon.hxx"
#include "np/util/log.hxx"

static void
usage(const char *argv0)
{
    fprintf(stderr, "Usage: %s [--debug] output-format] testrunner\n", argv0);
    exit(1);
}

namespace np {

static int
get_file_mtime(const char *filename, struct timespec *mtimep)
{
#if defined(__apple__)
    struct stat64 sb;
    if (stat64(filename, &sb) < 0)
        return -1;
    *mtimep = sb.st_mtimespec;
#elif defined(__linux__)
    struct stat64 sb;
    if (stat64(filename, &sb) < 0)
        return -1;
    mtimep->tv_sec = sb.st_mtime;
    // mtimep->tv_nsec = sb.st_mtimensec;
    mtimep->tv_nsec = sb.st_mtim.tv_nsec;
    return 0;
#else
#error No implementation for get_file_mtime
    return -1;
#endif
}

class watch_daemon_t : public np::util::daemon_t
{
private:
    struct file_state_t
    {
        np::util::filename_t name;
        std::vector<testnode_t *> tests;
        std::vector<file_state_t *> down;   // files that I depend on
        std::vector<file_state_t *> up;     // files that depend on me
        struct timespec mtime;
        bool dirty;

        file_state_t(const np::util::filename_t &nm)
         :  name(nm),
            dirty(false)
        {
            mtime.tv_sec = 0;
            mtime.tv_nsec = 0;
        }
    };

    np::util::filename_t testrunner_;
    std::map<np::util::filename_t, file_state_t *> files_;

    void build_dependency_graph();
    file_state_t *find_file_state(const np::util::filename_t &nm)
    {
        std::map<np::util::filename_t, file_state_t *>::iterator itr = files_.find(nm);
        return (itr == files_.end() ? 0 : itr->second);
    }
    file_state_t *make_file_state(const np::util::filename_t &nm)
    {
        file_state_t *fstate = new file_state_t(nm);
        files_[fstate->name] = fstate;
        return fstate;
    }

public:
    watch_daemon_t() {}
    // TODO: dtor which cleans up files_

    watch_daemon_t &set_testrunner(const np::util::filename_t &testrunner)
    {
        testrunner_ = testrunner;
        np::util::filename_t d = testrunner.dirname();
        np::util::filename_t b = testrunner.basename();
        set_logfile(d.join(b + ".watch.log"));
        set_pidfile(d.join(b + ".watch.pid"));
        return *this;
    }

    bool mainloop();
};

void
watch_daemon_t::build_dependency_graph()
{
    dprintf("Building dependency graph");

    // build a default plan with all the tests
    testmanager_t *testmanager = testmanager_t::instance_for_executable(testrunner_.c_str());
    plan_t *plan = new plan_t();
    plan->add_node(testmanager->get_root());

    std::map<np::util::filename_t, np::spiegel::compile_unit_t *> compile_units_by_filename;
    for (auto &cup : testmanager->get_spiegel_state()->get_compile_units())
        compile_units_by_filename[cup->get_absolute_path()] = cup;

    dprintf("Seeding queue of pending files from testnodes\n");
    std::vector<file_state_t *> pending;
    for (plan_t::iterator pitr = plan->begin() ; pitr != plan->end() ; ++pitr)
    {
        testnode_t *tn = pitr.get_node();
        dprintf("Testnode %s\n", tn->get_fullname().c_str());
        const np::spiegel::compile_unit_t *cu = tn->get_function(FT_TEST)->get_compile_unit();
        np::util::filename_t source_path = cu->get_absolute_path();
        file_state_t *fstate = find_file_state(source_path);
        if (!fstate)
        {
            fstate = make_file_state(source_path);
            dprintf("Enqueueing file %s\n", fstate->name.c_str());
            pending.push_back(fstate);
        }
        fstate->tests.push_back(tn);
    }

    dprintf("Working the queue of pending files\n");
    while (!pending.empty())
    {
        file_state_t *fstate = pending.back();
        pending.pop_back();
        dprintf("Dequeued file %s\n", fstate->name.c_str());

        // get the file's baseline mtime
        if (get_file_mtime(fstate->name.c_str(), &fstate->mtime) < 0)
        {
            eprintf("Cannot get mtime of file %s: %s\n", fstate->name.c_str(), strerror(errno));
            continue;
        }

        auto cuitr = compile_units_by_filename.find(fstate->name);
        if (cuitr == compile_units_by_filename.end())
        {
            dprintf("No compile unit for this file\n");
            continue;
        }

        for (auto &cfilename : cuitr->second->get_all_absolute_paths())
        {
            np::util::filename_t filename(cfilename);
            file_state_t *down_fstate = find_file_state(filename);
            if (!down_fstate)
            {
                down_fstate = make_file_state(filename);
                dprintf("Enqueueing file %s\n", down_fstate->name.c_str());
                pending.push_back(down_fstate);
            }
            if (down_fstate != fstate)
            {
                down_fstate->up.push_back(fstate);
                fstate->down.push_back(down_fstate);
            }
        }
    }
    dprintf("Finished working the queue of pending files\n");

    delete plan;
}

bool
watch_daemon_t::mainloop()
{
    build_dependency_graph();

    fprintf(stderr, "XXX dumping dependency graph\n");
    for (auto &pair : files_)
    {
        file_state_t *fstate = pair.second;
        fprintf(stderr, "fstate {\n");
        fprintf(stderr, "    name \"%s\"\n", fstate->name.c_str());
        fprintf(stderr, "    tests {\n");
        for (auto &tn : fstate->tests)
        {
            std::string fullname = tn->get_fullname();
            fprintf(stderr, "        %s\n", fullname.c_str());
        }
        fprintf(stderr, "    }\n");
        fprintf(stderr, "    down {\n");
        for (auto &down_fstate : fstate->down)
            fprintf(stderr, "        %s\n", down_fstate->name.c_str());
        fprintf(stderr, "    }\n");
        fprintf(stderr, "    up {\n");
        for (auto &up_fstate : fstate->up)
            fprintf(stderr, "        %s\n", up_fstate->name.c_str());
        fprintf(stderr, "    }\n");
        fprintf(stderr, "    mtime %llu.%lu\n",
                (unsigned long long)fstate->mtime.tv_sec, (unsigned long)fstate->mtime.tv_nsec);
        fprintf(stderr, "    dirty %s\n", fstate->dirty ? "true" : "false");
        fprintf(stderr, "}\n");
    }
    fprintf(stderr, "XXX done\n");

    /*
    while (running())
    {
    }
    */

    return true;
}

// close the namespace
};

// Codes returned from getopt_long() for options which have *only* long
// forms.  These are integers outside of the 7bit ASCII range so they do
// not clash with the characters used for short options.
enum opt_codes_t
{
    OPT_HELP=256,
    OPT_DEBUG,
    OPT_FOREGROUND
};

int
main(int argc, char **argv)
{
    bool foreground = true; // XXX
    bool debug = false;
    int c;
    static const struct option opts[] =
    {
	{ "foreground", no_argument, NULL, OPT_FOREGROUND },
	{ "debug", no_argument, NULL, OPT_DEBUG },
	{ "help", no_argument, NULL, OPT_HELP },
	{ NULL, 0, NULL, 0 },
    };

    /* Parse arguments */
    while ((c = getopt_long(argc, argv, "", opts, NULL)) >= 0)
    {
	switch (c)
	{
        case OPT_DEBUG:
            debug = true;
            break;
        case OPT_FOREGROUND:
            foreground = true;
            break;
        case OPT_HELP:
        default:
            // note, for unknown options getopt_long() has already
            // printed a message about the option being unknown..
	    usage(argv[0]);
	}
    }
    np::log::basic_config(debug ? np::log::DEBUG : np::log::INFO, 0);
    if (argc-optind != 1)
        usage(argv[0]);
    np::util::filename_t testrunner = ((const char **)argv)[optind];
    dprintf("testrunner = \"%s\"\n", testrunner.c_str());

    np::watch_daemon_t daemon;
    daemon.set_testrunner(testrunner).set_foreground(foreground);

    if (!daemon.start())
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
