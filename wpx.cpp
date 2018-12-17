#include <wc.h>
#include <cliargs.h>
#include <sb.h>

inline std::string c(const std::string& v, size_t w) {
    char buf[256];
    size_t vlen = v.length();
    if (vlen >= w) return v;
    size_t l = (w - vlen) / 2;
    size_t r = w - l - vlen;
    std::string fmt = "%" + (l ? std::to_string(l + vlen) : "") + "s" + (r ? "%" + std::to_string(r) + "s" : "");
    sprintf(buf, fmt.c_str(), v.c_str(), "");
    return buf;
    return r ? strprintf(fmt, v, "") : strprintf(fmt, v);
}

int main(int argc, char* const* argv)
{
    cliargs ca;
    ca.add_option("help", 'h', no_arg);
    ca.add_option("list", 'l', no_arg);
    ca.add_option("stats", 's', no_arg);
    ca.parse(argc, argv);
    if (ca.m.count('h') || ca.l.size() == 0) {
        fprintf(stderr, "Syntax: %s [options] <configuration>\n", argv[0]);
        fprintf(stderr, "Available options:\n"
            "    --help  | -h   Show this help text\n"
            "    --list  | -l   List the contents of the given configuration\n"
            "    --stats | -s   Show statistics about coverage\n"
        );
        exit(1);
    }
    FILE* fp = fopen(ca.l[0], "rb");
    if (!fp) {
        fprintf(stderr, "File not found or not readable: %s\n", ca.l[0]);
        exit(1);
    }
    wc::wc wc(fp);
    fclose(fp);
    if (ca.m.count('l')) {
        printf(" #  |    PRI   |    PEN   | CONFIG\n");
        size_t idx = wc.configurations->size();
        for (auto& c : *wc.configurations) { idx--; printf("%3zu | %8.5f | %8.5f | %s\n", idx, c->pri, c->last_penalty, c->to_string().c_str()); }
    } else if (ca.m.count('s')) {
        sb fmt, res, occ, inc, pri;
        fmt += ' '; res += ' '; occ += ' '; inc += ' '; pri += ' ';
        printf(" ");
        for (auto& o : wc.options) {
            size_t minwidth = 1 + o->name.length();
            size_t optlen = 0;
            for (auto& s : o->settings) {
                size_t slen = std::max<size_t>({3, s->value.length(), std::to_string(s->inclusions).length(), std::to_string(s->occurrences).length()});
                std::string f = "%-" + std::to_string(slen) + "s ";
                fmt += strprintf(f, s->value);
                f = "%-" + std::to_string(slen) + "zu ";
                optlen += slen + 1;
                size_t r = s->occurrences ? (100 * s->inclusions) / s->occurrences : 100;
                res += strprintf(f, r);
                occ += strprintf(f, s->occurrences);
                inc += strprintf(f, s->inclusions);
                f = "%-" + std::to_string(slen) + "d ";
                pri += strprintf(f, s->priority);
            }
            if (minwidth > optlen) {
                std::string f = "%" + std::to_string(minwidth - optlen) + "s";
                fmt += strprintf(f, "");
                res += strprintf(f, "");
                occ += strprintf(f, "");
                inc += strprintf(f, "");
                pri += strprintf(f, "");
            }
            minwidth = std::max(minwidth, optlen);
            fputs((c(o->name, minwidth) + " ").c_str(), stdout);
            fmt += ' '; res += ' '; occ += ' '; inc += ' '; pri += ' ';
        }
        printf("\n%s\n%s (%%)\n%s (priority)\n%s (count)\n%s (total)\n", fmt.c_str(), res.c_str(), pri.c_str(), inc.c_str(), occ.c_str());
    } else {
        if (!wc.emit_and_penalize(stdout)) {
            exit(1);
        }
        fp = fopen(argv[1], "wb");
        wc.save(fp);
        fclose(fp);
        exit(0);
    }
}
