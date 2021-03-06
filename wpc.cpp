#include <compiler/tinytokenizer.h>
#include <compiler/tinyparser.h>
#include <we.h>
#include <wc.h>

std::string derive_output(const std::string& str) {
    auto i = str.rfind('.', str.length());
    if (i != std::string::npos && str.length() - i < 5) {
        std::string s = str;
        s.replace(i + 1, 3, "scd");
        return s;
    }
    return str + ".scd";
}

int main(int argc, const char* argv[])
{
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Syntax: %s <specification> [<output>]\n", argv[0]);
        fprintf(stderr, "Output is derived from <specification> if left out.\n");
        exit(1);
    }
    const char* spec = argv[1];
    std::string output_string = argc == 3 ? argv[2] : derive_output(spec);
    const char* output = output_string.c_str();
    FILE* fp = fopen(spec, "r");
    if (!fp) {
        fprintf(stderr, "File not found or not readable: %s\n", spec);
        exit(1);
    }
    char buf[1024];
    std::string s;
    while (fgets(buf, 1024, fp)) {
        s += buf;
    }
    fclose(fp);
    auto t = tiny::tokenize(s.c_str());
    we::we we;
    while (t) {
        auto p = tiny::treeify(&t, true);
        if (!p) {
            fprintf(stderr, "parse failed around line %zu, col %zu\n", t->line, t->col);
            exit(1);
        }
        // printf("We got sumfin:\n");
        // p->print();
        // printf("\n");
        p->exec(&we);
        // printf("Result:\n");
        // we.print();
    }

    wc::wc wc(we);

    FILE* fres = fopen(output, "wb");
    if (!fres) {
        fprintf(stderr, "Unable to open file: %s\n", output);
        exit(1);
    }
    wc.save(fres);
    fclose(fres);
    printf("%zu\n", wc.configurations->size());
}
