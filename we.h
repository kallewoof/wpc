#ifndef included_we_h
#define included_we_h

#include <compiler/tinyast.h>
#include <tinyformat.h>

/**
 * Environment Generator
 * Takes as input as parsed tree of symbols.
 * Produces a vector of nodes.
 */
namespace we {

struct restricter;

struct configurator {
    virtual void emit(const std::string& output) = 0;
    virtual void branch(const std::string& desc, const std::string& var, const std::string& val, const std::string& emits, int priority, std::vector<restricter*> conditions) = 0;
};

struct node {
    virtual std::string to_string() const {
        return "<node>";
    }
    virtual void configure(configurator* cfg) {}
};

struct emitter: public node {
    std::string output;
    emitter(const std::string& output_in) : node() {
        output = output_in;
    }
    virtual std::string to_string() const override {
        return strprintf("<emitter: %s>", output);
    }
    virtual void configure(configurator* cfg) override {
        cfg->emit(output);
    }
};

struct restricter: public node {
    std::string var, val;
    tiny::token_type condition;
    restricter(const std::string& var_in, const std::string& val_in, tiny::token_type condition_in)
    : var(var_in)
    , val(val_in)
    , condition(condition_in) {}
    virtual std::string to_string() const override {
        return strprintf("<restricter: %s%s%s>", var, condition == tiny::tok_eq ? "==" : "!=", val);
    }
    virtual void configure(configurator* cfg) override {
        throw std::runtime_error("restricters do not configure() directly");
    }
};

struct brancher: public node {
    std::string desc, var, val, pre;
    int priority;
    std::vector<restricter*> conditions;
    brancher(const std::string& desc_in, const std::string& var_in, const std::string& val_in, const std::string& pre_in, int priority_in, const std::vector<restricter*>& conditions_in)
    : desc(desc_in)
    , var(var_in)
    , val(val_in)
    , pre(pre_in)
    , priority(priority_in)
    , conditions(conditions_in) {}
    virtual std::string conditions_string() const {
        if (conditions.size() == 0) return "none";
        std::string res = "";
        for (const auto& c : conditions) res += (res == "" ? "" : ", ") + c->to_string();
        return "[" + res + "]";
    }
    virtual std::string to_string() const override {
        return strprintf("<brancher pri=%d \"%s\": %s=%s, <emit %s>, conditions=%s>", priority, desc, var, val, pre, conditions_string());
    }
    virtual void configure(configurator* cfg) override {
        cfg->branch(desc, var, val, pre, priority, conditions);
    }
};

struct env {
    env* parent = nullptr;
    std::string option;
    std::vector<std::string>& option_stack;
    std::vector<restricter*>& cond_stack;
    std::vector<restricter*> local_conditions;
    env(std::vector<std::string>& option_stack_in, std::vector<restricter*>& cond_stack_in, const std::string& option_in="root")
    : option(option_in)
    , option_stack(option_stack_in)
    , cond_stack(cond_stack_in) {
        option_stack.emplace_back(option_in);
    }
    env* push(const std::string& option_name) {
        env* child = new env(option_stack, cond_stack, option_name);
        child->parent = this;
        return child;
    }
    env* pop() {
        if (option_stack.back() != option) throw std::runtime_error(strprintf("invalid stack operation (popping option %s, when back() = %s)", option, option_stack.back()));
        option_stack.pop_back();
        while (local_conditions.size()) {
            if (cond_stack.back() != local_conditions.back()) throw std::runtime_error(strprintf("invalid stack operation (popping condition %s, when back() = %s)", cond_stack.back()->to_string(), local_conditions.back()->to_string()));
            cond_stack.pop_back();
            local_conditions.pop_back();
        }
        env* lparent = parent;
        delete this;
        return lparent;
    }
    void push_condition(restricter* cond) {
        cond_stack.push_back(cond);
        local_conditions.push_back(cond);
    }
    void pop_condition(size_t id) {
        if (id != cond_stack.size()) throw std::runtime_error(strprintf("invalid stack operation (pop id %zu instead of %zu)", id, cond_stack.size()));
        if (local_conditions.size() == 0) throw std::runtime_error("env violation (local conditions 0 @ pop_condition call)");
        local_conditions.pop_back();
        cond_stack.pop_back();
    }
};

struct we: public tiny::st_callback_table {
    std::map<std::string,std::string> state;
    std::vector<restricter*> restricters;

    std::vector<node*> nodes;
    std::vector<std::string> option_stack;
    std::vector<restricter*> cond_stack;
    env* e;
    we() {
        e = new env(option_stack, cond_stack);
    }
    ~we() {
        for (auto& e : restricters) delete e;
        for (auto& n : nodes) delete n;
    }

    virtual void emit(const std::string& output) override {
        nodes.push_back(new emitter(output));
    }

    virtual void option_begin(const std::string& name) override {
        e = e->push(name);
    }

    virtual void option_end(const std::string& name) override {
        // TODO: remove redundant checks
        if (option_stack.size() == 0) throw std::runtime_error("invalid stack operation (option stack is empty @ option_end call)");
        if (option_stack.back() != name) throw std::runtime_error(strprintf("invalid stack operation (in popping %s, %s was encountered)", name, option_stack.back()));
        e = e->pop();
    }

    virtual size_t cond_begin(const std::string& var, const std::string& expr, tiny::token_type cond) override {
        restricter* r = new restricter(var, expr, cond);
        restricters.push_back(r);
        e->push_condition(r);
        return cond_stack.size();
    }

    virtual void cond_end(size_t id) override {
        e->pop_condition(id);
    }

    virtual void branch(const std::string& desc, const std::string& expr, int priority, const std::string& pre = "") override {
        if (option_stack.size() == 0) throw std::runtime_error("invalid branch call (option stack is empty)");
        nodes.push_back(new brancher(desc, option_stack.back(), expr, pre, priority, cond_stack));
    }

    void print() const {
        for (const auto& v : nodes) {
            printf("＊＊ %s\n", v->to_string().c_str());
        }
    }
};

} // namespace we

#endif // included_we_h
