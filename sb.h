#ifndef included_sb_h_
#define included_sb_h_

struct sb {
    char* b;
    size_t cap;
    size_t ind;
    sb() {
        cap = 256;
        ind = 0;
        b = (char*)malloc(cap);
    }
    sb(const char* v) {
        ind = strlen(v);
        cap = ind < 128 ? 256 : ind << 1;
        b = new char[cap];
        memcpy(b, v, ind);
    }
    sb(const std::string& v) : sb(v.c_str()) {}
    ~sb() {
        free(b);
    }
    inline void grow(size_t need) {
        cap = need << 1;
        b = (char*)realloc(b, cap);
    }
    sb& operator+=(const char* v) {
        size_t len = strlen(v);
        if (len + ind > cap) grow(len + ind);
        memcpy(&b[ind], v, len);
        ind += len;
        b[ind] = 0;
        return *this;
    }
    sb& operator+=(const std::string& v) {
        return operator+=(v.c_str());
    }
    sb& operator+=(const char ch) {
        if (1 + ind > cap) grow(1 + ind);
        b[ind++] = ch;
        b[ind] = 0;
        return *this;
    }
    const char* c_str() const { return b; }
    std::string string() const { return b; }
};

#endif // included_sb_h_
