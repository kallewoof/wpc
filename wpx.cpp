#include <wc.h>
#include <cliargs.h>

int main(int argc, char* const* argv)
{
    cliargs ca;
    ca.add_option("help", 'h', no_arg);
    ca.add_option("list", 'l', no_arg);
    ca.parse(argc, argv);
    if (ca.m.count('h') || ca.l.size() == 0) {
        fprintf(stderr, "Syntax: %s [options] <configuration>\n", argv[0]);
        fprintf(stderr, "Available options:\n"
            "    --help | -h   Show this help text\n"
            "    --list | -l   List the contents of the given configuration\n"
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
