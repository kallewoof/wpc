#include <map>
#include <vector>

#include <getopt.h>

#include <tinyformat.h>

enum cliarg_type {
    no_arg = no_argument,
    req_arg = required_argument,
    opt_arg = optional_argument,
};

struct cliopt {
    char* longname;
    char shortname;
    cliarg_type type;
    cliopt(const char* longname_in, const char shortname_in, cliarg_type type_in)
    : longname(strdup(longname_in))
    , shortname(shortname_in)
    , type(type_in)
    {}
    ~cliopt() { free(longname); }
    struct option get_option(std::string& opt) {
        opt += strprintf("%c%s", shortname, type == no_arg ? "" : ":");
        return {longname, type, nullptr, shortname};
    }
};

struct cliargs {
    std::map<char, std::string> m;
    std::vector<const char*> l;
    std::vector<cliopt*> long_options;

    ~cliargs() {
        while (!long_options.empty()) {
            delete long_options.back();
            long_options.pop_back();
        }
    }
    void add_option(const char* longname, const char shortname, cliarg_type t) {
        long_options.push_back(new cliopt(longname, shortname, t));
    }
    void parse(int argc, char* const* argv) {
        struct option long_opts[long_options.size() + 1];
        std::string opt = "";
        for (size_t i = 0; i < long_options.size(); i++) {
            long_opts[i] = long_options[i]->get_option(opt);
        }
        long_opts[long_options.size()] = {0,0,0,0};
        int c;
        int option_index = 0;
        for (;;) {
            c = getopt_long(argc, argv, opt.c_str(), long_opts, &option_index);
            if (c == -1) {
                break;
            }
            if (optarg) {
                m[c] = optarg;
            } else {
                m[c] = "1";
            }
        }
        while (optind < argc) {
            l.push_back(argv[optind++]);
        }
    }
};
