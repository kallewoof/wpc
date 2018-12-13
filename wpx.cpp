#include <wc.h>

int main(int argc, const char* argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Syntax: %s <configuration>\n", argv[0]);
        exit(1);
    }
    FILE* fp = fopen(argv[1], "rb");
    if (!fp) {
        fprintf(stderr, "File not found or not readable: %s\n", argv[1]);
        exit(1);
    }
    wc::wc wc(fp);
    fclose(fp);
    if (!wc.emit_and_penalize(stdout)) {
        exit(1);
    }
    fp = fopen(argv[1], "wb");
    wc.save(fp);
    fclose(fp);
    exit(0);
}
