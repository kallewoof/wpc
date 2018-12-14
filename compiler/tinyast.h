// Copyright (c) 2018 Karl-Johan Alm
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef included_tiny_ast_h_
#define included_tiny_ast_h_

#include <compiler/tinytokenizer.h>

#include <map>

namespace tiny {

typedef size_t ref;
static const ref nullref = 0;

// class program_t;
// 
struct st_callback_table {
//     bool ret = false;
//     virtual ref  load(const std::string& variable) = 0;
//     virtual void save(const std::string& variable, ref value) = 0;
//     virtual ref  bin(token_type op, ref lhs, ref rhs) = 0;
//     // virtual ref  mod(ref v, ref m) = 0;
//     virtual ref  unary(token_type op, ref val) = 0;
//     virtual ref  fcall(const std::string& fname, ref args) = 0;
//     virtual ref  pcall(ref program, ref args) = 0;
//     virtual ref  preg(program_t* program) = 0;
//     virtual ref  convert(const std::string& value, token_type type, token_type restriction) = 0;
//     virtual ref  to_array(size_t count, ref* refs) = 0;
//     virtual ref  at(ref arrayref, ref indexref) = 0;
//     virtual ref  range(ref arrayref, ref startref, ref endref) = 0;
//     virtual ref  compare(ref a, ref b, token_type op) = 0;
//     virtual bool truthy(ref v) = 0;
//     // modulo
//     virtual void push_mod(ref v) = 0;
//     virtual void pop_mod() = 0;
    virtual void emit(const std::string& output) = 0;
    virtual void branch(const std::string& desc, const std::string& expr, int priority, const std::string& pre = "") = 0;
    virtual void option_begin(const std::string& name) = 0;
    virtual void option_end(const std::string& name) = 0;
    virtual size_t cond_begin(const std::string& var, const std::string& expr, token_type cond) = 0;
    virtual void cond_end(size_t id) = 0;
};

struct st_t {
    virtual std::string to_string(bool terse = false) {
        return "????";
    }
    virtual void print() {
        printf("%s", to_string().c_str());
    }
    virtual void exec(st_callback_table* ct) {}
    virtual void cexe(st_callback_table* ct) {}
    virtual st_t* clone() {
        return new st_t();
    }
    virtual size_t ops() {
        return 1;
    }
};

struct st_c {
    st_t* r;
    size_t* refcnt;
    // void alive() { printf("made st_c with ptr %p ref %zu (%p)\n", r, refcnt ? *refcnt : 0, refcnt); }
    // void dead() { printf("deleting st_c with ptr %p ref %zu (%p)\n", r, refcnt ? *refcnt : 0, refcnt); }
    st_c(st_t* r_in) {
        r = r_in;
        refcnt = (size_t*)malloc(sizeof(size_t));
        *refcnt = 1;
        // alive();
    }
    st_c(const st_c& o) {
        r = o.r;
        refcnt = o.refcnt;
        (*refcnt)++;
        // alive();
    }
    st_c(st_c&& o) {
        r = o.r;
        refcnt = o.refcnt;
        o.r = nullptr;
        o.refcnt = nullptr;
        // alive();
    }
    st_c& operator=(const st_c& o) {
        if (refcnt) {
            if (!--(*refcnt)) {
                // dead();
                delete r;
                delete refcnt;
            }
        }
        r = o.r;
        refcnt = o.refcnt;
        (*refcnt)++;
        // alive();
        return *this;
    }
    ~st_c() {
        // dead();
        if (!refcnt) return;
        if (!--(*refcnt)) {
            delete r;
            delete refcnt;
        }
    }
    st_c clone() {
        return st_c(r->clone());
    }
};

struct pre_t: public st_t {
    std::string content;
    pre_t(const std::string& content_in) : content(content_in) {}
    virtual std::string to_string(bool terse) override {
        return strprintf("{{%s}}", content);
    }
    virtual void exec(st_callback_table* ct) override {
        ct->emit(content);
    }
    virtual st_t* clone() override {
        return new pre_t(content);
    }
};

struct cond_t: public st_t {
    std::string var;
    std::string val;
    token_type comparator;
    size_t last_id = 0;
    cond_t(const std::string& var_in, const std::string& val_in, token_type comparator_in)
    : var(var_in)
    , val(val_in)
    , comparator(comparator_in)
    {}
    virtual std::string to_string(bool terse) override {
        return strprintf("(%s %s %s)", var, token_type_str[comparator], val);
    }
    virtual void exec(st_callback_table* ct) override {
        if (last_id != 0) throw std::runtime_error("exec() called twice without corresponding cexe call (cond_t)");
        last_id = ct->cond_begin(var, val, comparator);
    }
    virtual void cexe(st_callback_table* ct) override {
        if (last_id == 0) throw std::runtime_error("cexe() called without corresponding exec call (cond_t)");
        ct->cond_end(last_id);
        last_id = 0;
    }
    virtual st_t* clone() override {
        return new cond_t(var, val, comparator);
    }
};

struct value_t: public st_t {
    std::string expr;
    std::string desc;
    std::string pre;
    st_t* condition;
    int priority; // -1=heavy, -1=uninteresting, 0=normal, 1=prioritized
    static bool prioritize(const std::string& pexpr, int& p) {
        if (pexpr == "heavy") p--;
        else if (pexpr == "uninteresting") p--;
        else if (pexpr == "light") p++;
        else if (pexpr == "interesting") p++;
        else if (pexpr == "prioritized") p++;
        else return pexpr == "normal";
        return true;
    }
    value_t(const std::string& expr_in, const std::string& desc_in, int priority_in, const std::string& pre_in, st_t* condition_in = nullptr)
    : expr(expr_in)
    , desc(desc_in)
    , priority(priority_in)
    , pre(pre_in)
    , condition(condition_in) {}
    ~value_t() {
        if (condition) delete condition;
    }
    virtual std::string to_string(bool terse) override {
        return strprintf("(%s[req: %s]; pri=%d; \"%s\" {{%s}})", expr, condition ? condition->to_string(terse) : "none", priority, desc, pre);
    }
    virtual void exec(st_callback_table* ct) override {
        if (condition) condition->exec(ct);
        ct->branch(desc, expr, priority, pre);
        if (condition) condition->cexe(ct);
    }
    virtual st_t* clone() override {
        return new value_t(expr, desc, priority, pre, condition ? condition->clone() : nullptr);
    }
};

struct option_t: public st_t {
    std::string name;
    std::vector<st_c> values;
    option_t(const std::string& name_in, const std::vector<st_c>& values_in)
    : name(name_in)
    , values(values_in) {}
    virtual std::string to_string(bool terse) override {
        std::string s = strprintf("<> %s {\n", name);
        for (const auto& v : values) s += "\t" + v.r->to_string() + "\n";
        return s + "}";
    }
    virtual void exec(st_callback_table* ct) override {
        ct->option_begin(name);
        for (const auto& v : values) {
            v.r->exec(ct);
        }
        ct->option_end(name);
    }
    virtual st_t* clone() override {
        return new option_t(name, values);
    }
};

} // namespace tiny

#endif // included_tiny_ast_h_
