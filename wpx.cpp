#include <wc.h>
#include <cliargs.h>

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
        printf(" #  | PRI      | CONFIG\n");
        size_t idx = wc.configurations->size();
        for (auto& c : *wc.configurations) { idx--; printf("%3zu | %8.5f | %s\n", idx, c->pri, c->to_string().c_str()); }
    } else if (ca.m.count('s')) {
        char fmtbuf[1024];
        char resbuf[1024];
        char occbuf[1024];
        char incbuf[1024];
        char* fbp = fmtbuf;
        char* rbp = resbuf;
        char* obp = occbuf;
        char* ibp = incbuf;
        (*fbp++) = ' ';
        (*rbp++) = ' ';
        (*obp++) = ' ';
        (*ibp++) = ' ';
        printf(" ");
        for (auto& o : wc.options) {
            size_t minwidth = 1 + o->name.length();
            size_t optlen = 0;
            for (auto& s : o->settings) {
                size_t slen = std::max<size_t>({3, s->value.length(), std::to_string(s->inclusions).length(), std::to_string(s->occurrences).length()});
                std::string fmt = "%-" + std::to_string(slen) + "s ";
                fbp += sprintf(fbp, fmt.c_str(), s->value.c_str());
                fmt = "%-" + std::to_string(slen) + "zu ";
                optlen += slen + 1;
                size_t r = (100 * s->inclusions) / s->occurrences;
                rbp += sprintf(rbp, fmt.c_str(), r);
                obp += sprintf(obp, fmt.c_str(), s->occurrences);
                ibp += sprintf(ibp, fmt.c_str(), s->inclusions);
            }
            if (minwidth > optlen) {
                std::string f = "%" + std::to_string(minwidth - optlen) + "s";
                fbp += sprintf(fbp, f.c_str(), "");
                rbp += sprintf(rbp, f.c_str(), "");
                obp += sprintf(obp, f.c_str(), "");
                ibp += sprintf(ibp, f.c_str(), "");
            }
            minwidth = std::max(minwidth, optlen);
            std::string hdr = "%-" + std::to_string(minwidth) + "s ";
            printf(hdr.c_str(), o->name.c_str());
            (*fbp++) = ' ';
            (*rbp++) = ' ';
            (*obp++) = ' ';
            (*ibp++) = ' ';
        }
        *rbp = *fbp = *obp = *ibp = 0;
        printf("\n%s\n%s (%%)\n%s (count)\n%s (total)\n", fmtbuf, resbuf, incbuf, occbuf);
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
