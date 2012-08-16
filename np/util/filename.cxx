/*
 * Copyright 2011-2012 Gregory Banks
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
#include "np/util/common.hxx"
#include "np/util/filename.hxx"
#include "np/util/tok.hxx"

namespace np { namespace util {
using namespace std;

bool
filename_t::is_path_tail(filename_t file) const
{
    ssize_t difflen = (ssize_t)length() - (ssize_t)file.length();
    if (difflen < 0)
	return false;
    const char *path = c_str();
    const char *tail = path + difflen;
    return (!strcmp(tail, file.c_str()) && (tail == path || tail[-1] == '/'));
}

filename_t
filename_t::make_absolute_to(filename_t absfile, bool isdir) const
{
    filename_t abs;
    tok_t tok(c_str(), "/");
    const char *part;

    if (is_absolute())
    {
	abs.append("/");
    }
    else if (absfile.length())
    {
	if (isdir)
	{
	    abs.append(absfile);
	}
	else
	{
	    if (absfile.is_path_tail(*this))
		return absfile;
	    size_t t = absfile.find_last_of('/');
	    assert(t != string::npos);
	    abs.append(absfile.c_str(), t);
	}
    }
    else
    {
	abs.append(current_dir());
    }

    while ((part = tok.next()) != 0)
    {
	if (!strcmp(part, "."))
	{
	    continue;
	}
	if (!strcmp(part, ".."))
	{
	    size_t p = abs.find_last_of('/');
	    if (p)
		abs.resize(p);
	    continue;
	}
	if (abs.length() > 1)
	    abs.append("/");
	abs.append(part);
    }

    return abs;
}

filename_t
filename_t::make_absolute() const
{
    return make_absolute_to(filename_t(), false);
}

filename_t
filename_t::make_absolute_to_file(filename_t absfile) const
{
    return make_absolute_to(absfile, false);
}

filename_t
filename_t::make_absolute_to_dir(filename_t absdir) const
{
    return make_absolute_to(absdir, true);
}

//
// Normalise an absolute or relative path.  Remove "."
// path components, collapse ".." path components where
// possible.  Returns a new string.
//
filename_t
filename_t::normalise() const
{
    string up;
    string down;
    tok_t tok(c_str(), "/");
    const char *part;
    bool is_abs = is_absolute();

    if (is_abs)
	up.append("/");

    while ((part = tok.next()) != 0)
    {
	if (!strcmp(part, "."))
	{
	    continue;
	}
	if (!strcmp(part, ".."))
	{
	    size_t i = down.find_last_of('/');

	    if (i != string::npos)
		down.resize(i);
	    else if (down.length())
		down.resize(0);
	    else if (!is_abs)
		up.append("../");
	    continue;
	}
	if (down.length() > 1)
	    down.append("/");
	down.append(part);
    }

    if (!down.length() && !up.length())
	return filename_t(".");

    up.append(down);
    return up;
}

filename_t
filename_t::basename() const
{
    size_t tail = find_last_of('/');
    return (tail == string::npos ? filename_t(*this) : substr(tail+1));
}

filename_t
filename_t::current_dir()
{
    char path[PATH_MAX];
    getcwd(path, sizeof(path));
    return path;
}

void
filename_t::pop_back()
{
    size_t p = find_last_of('/');
    if (p != string::npos && p)
	resize(p-1);
}

// close the namespaces
}; };
