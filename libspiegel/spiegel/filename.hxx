#ifndef __spiegel_filename_hxx__
#define __spiegel_filename_hxx__ 1

#include <string>

namespace spiegel {

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
    static filename_t current_dir();

    void pop_back();

private:
    filename_t make_absolute_to(filename_t absfile, bool isdir) const;
};

}; // namespace spiegel

#endif // __spiegel_filename_hxx__
