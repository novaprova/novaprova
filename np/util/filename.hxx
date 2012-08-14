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
#ifndef __spiegel_filename_hxx__
#define __spiegel_filename_hxx__ 1

#include <string>

namespace np { namespace util {

class filename_t : public std::string
{
public:
    filename_t() {}
    filename_t(const filename_t &o) : std::string(o.c_str()) {}
    filename_t(const std::string &o) : std::string(o) {}
    filename_t(const char *s) : std::string(s) {}

    bool is_absolute() const
    {
	return (length() && at(0) == '/');
    }
    bool is_path_tail(filename_t file) const;
    filename_t make_absolute() const;
    filename_t make_absolute_to_file(filename_t absfile) const;
    filename_t make_absolute_to_dir(filename_t absdir) const;
    filename_t normalise() const;
    filename_t basename() const;
    static filename_t current_dir();

    void pop_back();

private:
    filename_t make_absolute_to(filename_t absfile, bool isdir) const;
};

// close the namespaces
}; };

#endif // __spiegel_filename_hxx__
