#ifndef included_wc_h
#define included_wc_h

#include <tinyformat.h>
#include <we.h>
#include <cstdlib>
#include <set>
#include <vector>

inline float frand() {
    static bool srand_called = false;
    if (!srand_called) { srand_called = true; srand(time(NULL)); }
    return (float)rand() / RAND_MAX;
}

/**
 * Configuration Generator
 * Takes as input a we:: environment.
 * Produces a vector of options with corresponding settings, and a vector
 * of all acceptable combinations of options.
 */
namespace wc {

#define D(T) \
    void serialize(FILE* fp, const T& t) { fwrite(&t, sizeof(t), 1, fp); /*printf("[ser %06ld] %d\n", ftell(fp), t);*/ } \
    void deserialize(FILE* fp, T& t) { fread(&t, sizeof(t), 1, fp); /*printf("[des %06ld] %d\n", ftell(fp), t);*/ }
D(size_t)
D(float)
D(tiny::token_type)
D(int)

struct req;
struct setting;
struct option;
struct configuration;

void serialize(FILE* fp, const req& r);
void deserialize(FILE* fp, req& r);
void serialize(FILE* fp, const setting& s);
void deserialize(FILE* fp, setting& s);
void serialize(FILE* fp, const option& o);
void deserialize(FILE* fp, option& o);
void serialize(FILE* fp, const configuration& c);
void deserialize(FILE* fp, configuration& c);

inline void serialize(FILE* fp, const std::string& s) { serialize(fp, s.size()); fwrite(s.c_str(), s.size(), 1, fp); /*printf("[ser %06ld] (string) %s\n", ftell(fp), s.c_str());*/ }
inline std::string deserialize_string(FILE* fp) { size_t sz; deserialize(fp, sz); char buf[sz + 1]; buf[sz] = 0; fread(buf, sz, 1, fp); /*printf("[des %06ld] (string) %s\n", ftell(fp), buf);*/ return buf; }

inline void deserialize(FILE* fp, std::string& s) { s = deserialize_string(fp); }

#define vser(fp, v) serialize(fp, v.size()); for (const auto& e : v) serialize(fp, e)
#define vdes(fp, v) do { size_t sz; deserialize(fp, sz); v.resize(sz); for (size_t i = 0; i < sz; ++i) deserialize(fp, v[i]); } while (0)
// template<typename T> inline void serialize(FILE* fp, const std::vector<T>& v) { serialize(fp, v.size()); for (const auto& e : v) serialize(fp, e); }
// template<typename T> inline void deserialize(FILE* fp, std::vector<T>& v) { size_t sz; deserialize(fp, sz); v.resize(sz); for (size_t i = 0; i < sz; ++i) deserialize(fp, v[i]); }

#define vpser(fp, v) serialize(fp, (v).size()); for (const auto& e : (v)) serialize(fp, *e)
#define vpdes(fp, v, T) do { size_t sz; deserialize(fp, sz); for (auto& e : v) delete e; (v).resize(sz); for (size_t i = 0; i < sz; ++i) { (v)[i] = new T(); deserialize(fp, *(v)[i]); } } while (0)
// template<typename T> inline void serialize(FILE* fp, const std::vector<T*>& v) { serialize(fp, v.size()); for (const auto& e : v) serialize(fp, *e); }
// template<typename T> inline void deserialize(FILE* fp, std::vector<T*>& v) { size_t sz; deserialize(fp, sz); for (auto& e : v) delete e; v.resize(sz); for (size_t i = 0; i < sz; ++i) { v[i] = new T(); deserialize(fp, v[i]); } }

struct req {
    std::string var, val;
    tiny::token_type cond;
    req() {}
    req(const std::string& var_in, const std::string& val_in, tiny::token_type cond_in)
    : var(var_in)
    , val(val_in)
    , cond(cond_in) {}
    bool met(const std::map<std::string,std::string>& state, const std::string& x_var = "", const std::string& x_val = "") const {
        bool equals = (state.count(var) && x_var != var && state.at(var) == val) || (x_var == var && x_val == val);
        return cond == tiny::tok_eq ? equals : !equals;
    }
    static void convert_we(std::vector<req*>& v, const std::vector<we::restricter*>& rest) {
        for (const auto& r : rest) {
            v.push_back(new req(r->var, r->val, r->condition));
        }
    }
    static bool met(const std::map<std::string,std::string>& state, const std::vector<req*>& requirements, const std::string& x_var = "", const std::string& x_val = "") {
        for (const auto& r : requirements) if (!r->met(state, x_var, x_val)) return false;
        return true;
    }
};

void serialize(FILE* fp, const req& r) {
    serialize(fp, r.var);
    serialize(fp, r.val);
    serialize(fp, r.cond);
}

void deserialize(FILE* fp, req& r) {
    r.var = deserialize_string(fp);
    r.val = deserialize_string(fp);
    deserialize(fp, r.cond);
}

struct setting {
    std::string value;
    std::string emits;
    std::vector<req*> requirements;
    int priority;
    setting() {}
    setting(const std::string& value_in, const std::string& emits_in, const std::vector<req*>& requirements_in, int priority_in)
    : value(value_in)
    , emits(emits_in)
    , requirements(requirements_in)
    , priority(priority_in) {}
};

void serialize(FILE* fp, const setting& s) {
    serialize(fp, s.value);
    serialize(fp, s.emits);
    vpser(fp, s.requirements);
    serialize(fp, s.priority);
}

void deserialize(FILE* fp, setting& s) {
    s.value = deserialize_string(fp);
    s.emits = deserialize_string(fp);
    vpdes(fp, s.requirements, req);
    deserialize(fp, s.priority);
}

struct option {
    std::string name;
    std::string emits;
    std::vector<setting*> settings;
    option() {}
    option(const std::string& name_in, const std::string& emits_in, const std::vector<setting*> settings_in = std::vector<setting*>())
    : name(name_in)
    , emits(emits_in)
    , settings(settings_in) {}
    void emit(size_t sidx, FILE* fp) const {
        fprintf(fp, "%s%s=%s%s\n", emits.c_str(), name.c_str(), settings.at(sidx)->value.c_str(), settings.at(sidx)->emits.c_str());
    }
};

void serialize(FILE* fp, const option& o) {
    serialize(fp, o.name);
    serialize(fp, o.emits);
    vpser(fp, o.settings);
}

void deserialize(FILE* fp, option& o) {
    o.name = deserialize_string(fp);
    o.emits = deserialize_string(fp);
    vpdes(fp, o.settings, setting);
}

struct configuration {
    std::vector<option*> options;
    std::map<std::string,std::string> state;
    std::vector<size_t> setting_index;
    std::vector<req*> requirements;
    size_t old_idx;
    float pri = 0;
    configuration() {}
    configuration(const std::vector<option*>& options_in, const std::map<std::string,std::string>& state_in, const std::vector<size_t>& setting_index_in, const std::vector<req*> requirements_in, option* opt, size_t sidx, std::vector<req*> reqs)
    : options(options_in)
    , state(state_in)
    , setting_index(setting_index_in)
    , requirements(requirements_in) {
        options.push_back(opt);
        setting_index.push_back(sidx);
        state[opt->name] = opt->settings[sidx]->value;
        assert(options.size() == setting_index.size());
        for (req* r : reqs) requirements.push_back(r);
    }
    void calc_pri() {
        pri = 0;
        for (size_t i = 0; i < options.size(); ++i) {
            pri += options[i]->settings[setting_index[i]]->priority;
        }
    }
    void branch(std::vector<configuration*>* config_pool, option* opt) {
        for (size_t i = 0; i < opt->settings.size(); ++i) {
            setting* s = opt->settings[i];
            if (req::met(state, s->requirements) && req::met(state, requirements, opt->name, s->value)) {
                config_pool->push_back(new configuration(options, state, setting_index, requirements, opt, i, s->requirements));
            }
        }
    }
    static void begin(std::vector<configuration*>* config_pool, option* opt) {
        std::vector<option*> options;
        std::map<std::string,std::string> state;
        std::vector<size_t> setting_index;
        std::vector<req*> requirements;
        for (size_t i = 0; i < opt->settings.size(); ++i) {
            config_pool->push_back(new configuration(options, state, setting_index, requirements, opt, i, opt->settings[i]->requirements));
        }
    }
    std::string to_string() const {
        std::string r = "";//"pri=" + std::to_string(pri);
        for (size_t i = 0; i < options.size(); ++i) {
            r += (r == "" ? "" : ", ") + options[i]->name + "=" + options[i]->settings[setting_index[i]]->value;
        }
        return r;
    }
    void penalize(const configuration& basis) {
        // for any matching setting indices, we penalize ourselves
        for (size_t i = 0; i < options.size(); ++i) {
            if (setting_index[i] == basis.setting_index[i]) {
                pri -= 0.01; // TODO: alter based on settings priority
            }
        }
    }
    void emit(FILE* fp) const {
        for (size_t i = 0; i < options.size(); ++i) {
            options.at(i)->emit(setting_index.at(i), fp);
        }
    }
};

void serialize(FILE* fp, const configuration& c) {
    std::vector<std::string> opt_strings;
    for (const auto& o : c.options) opt_strings.push_back(o->name);
    vser(fp, opt_strings);
    vser(fp, c.setting_index);
    serialize(fp, c.pri);
}

std::map<std::string,option*>* option_map_ptr = nullptr;

void deserialize(FILE* fp, configuration& c) {
    assert(option_map_ptr);
    c.options.clear();
    c.setting_index.clear();
    std::vector<std::string> opt_strings;
    vdes(fp, opt_strings);
    for (auto& opt_str : opt_strings) {
        c.options.push_back(option_map_ptr->at(opt_str));
    }
    vdes(fp, c.setting_index);
    deserialize(fp, c.pri);
}

struct wc: public we::configurator {
    std::string emits;
    std::vector<setting*> settings;
    std::vector<option*> options;
    std::map<std::string,option*> option_map;
    std::vector<configuration*>* configurations;

    void replace_config(std::vector<configuration*>* new_config) {
        for (configuration* c : *configurations) delete c;
        delete configurations;
        configurations = new_config;
    }

    wc(we::we& env) {
        configurations = nullptr;
        for (we::node* n : env.nodes) {
            n->configure(this);
        }
        // printf("generated %zu options with %zu settings\n", options.size(), settings.size());
        for (option* opt : options) {
            std::vector<configuration*>* new_config = new std::vector<configuration*>();
            if (configurations == nullptr) {
                configurations = new_config;
                configuration::begin(configurations, opt);
            } else {
                for (configuration* cfg : *configurations) {
                    cfg->branch(new_config, opt);
                }
                replace_config(new_config);
            }
        }
        // printf("generated %zu configurations\n", configurations->size());
        normalize();
        blur();
        sort();
    }

    wc(FILE* fp) {
        configurations = new std::vector<configuration*>();
        load(fp);
        // size_t idx = configurations->size();
        // for (auto& c : *configurations) { idx--; c->old_idx = idx; }
        // for (auto& c : *configurations) { printf("- %s\n", c->to_string().c_str()); }
    }

    void save(FILE* fp) const {
        serialize(fp, emits);
        vpser(fp, options);
        vpser(fp, *configurations);
        // size_t idx = configurations->size();
        // for (auto& c : *configurations) { idx--; printf("- %zu->%zu %s\n", c->old_idx, idx, c->to_string().c_str()); }
    }

    void load(FILE* fp) {
        emits = deserialize_string(fp);
        vpdes(fp, options, option);
        for (auto& o : options) {
            option_map[o->name] = o;
        }
        option_map_ptr = &option_map;
        vpdes(fp, *configurations, configuration);
    }

    bool emit_and_penalize(FILE* stream) {
        if (!configurations || configurations->size() == 0) return false;
        auto cfg = configurations->back();
        configurations->pop_back();
        for (auto& c : *configurations) c->penalize(*cfg);
        fprintf(stream, "%s", emits.c_str());
        cfg->emit(stream);
        sort();
        return true;
    }

    void sort() {
        std::sort((*configurations).begin(), (*configurations).end(), [](const configuration* a, const configuration* b) { return a->pri < b->pri; });
    }

    void blur() {
        for (auto c : *configurations) {
            c->pri += (frand() - 0.5) / 10;
        }
    }

    void normalize() {
        float min = 1e99, max = -1e99;
        for (auto c : *configurations) {
            c->calc_pri();
            if (min > c->pri) min = c->pri;
            if (max < c->pri) max = c->pri;
        }
        float len = max - min;
        // adjust to 0..1
        float add = -min;
        for (auto c : *configurations) {
            c->pri = (c->pri + add) / len;
        }
    }

    virtual void emit(const std::string& output) override {
        emits += output;
    }

    virtual void branch(const std::string& desc, const std::string& var, const std::string& val, const std::string& emits, int priority, std::vector<we::restricter*> conditions) override {
        std::vector<req*> conds;
        req::convert_we(conds, conditions);
        setting* s = new setting(val, emits, conds, priority);
        settings.push_back(s);
        if (option_map.count(var)) {
            option* opt = option_map.at(var);
            opt->settings.push_back(s);
        } else {
            std::vector<setting*> sv{s};
            option* opt = new option(var, "", sv);
            options.push_back(opt);
            option_map[var] = opt;
        }
    }
};

} // namespace wc

#endif // included_wc_h
