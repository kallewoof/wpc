#include <compiler/tinytokenizer.h>
#include <compiler/tinyparser.h>
#include <we.h>
#include <wc.h>

int main(int argc, const char* argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Syntax: %s <specification> <output>\n", argv[0]);
        exit(1);
    }
    FILE* fp = fopen(argv[1], "r");
    if (!fp) {
        fprintf(stderr, "File not found or not readable: %s\n", argv[1]);
        exit(1);
    }
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp) + 1;
    char* buf = (char*)malloc(sz);
    fseek(fp, 0, SEEK_SET);
    fread(buf, 1, sz, fp);
    fclose(fp);
    auto t = tiny::tokenize(buf);
    we::we we;
    while (t) {
        auto p = tiny::treeify(&t, true);
        if (!p) {
            fprintf(stderr, "parse failed\n");
            break;
        }
        // printf("We got sumfin:\n");
        // p->print();
        // printf("\n");
        p->exec(&we);
        // printf("Result:\n");
        // we.print();
    }

    wc::wc wc(we);

    FILE* fres = fopen(argv[2], "wb");
    if (!fres) {
        fprintf(stderr, "Unable to open file: %s\n", argv[2]);
        exit(1);
    }
    wc.save(fres);
    fclose(fres);
}
