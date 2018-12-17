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
    inline void serialize(FILE* fp, const T& t) { fwrite(&t, sizeof(t), 1, fp); /*printf("[ser %06ld] %d\n", ftell(fp), t);*/ } \
    inline void deserialize(FILE* fp, T& t) { fread(&t, sizeof(t), 1, fp); /*printf("[des %06ld] %d\n", ftell(fp), t);*/ }
D(size_t)
D(float)
D(tiny::token_type)
D(int)
D(uint32_t)

struct req;
struct setting;
struct option;
struct configuration;

inline void serialize(FILE* fp, const std::string& s) { serialize(fp, s.size()); fwrite(s.c_str(), s.size(), 1, fp); /*printf("[ser %06ld] (string) %s\n", ftell(fp), s.c_str());*/ }
inline std::string deserialize_string(FILE* fp) { size_t sz; deserialize(fp, sz); char buf[sz + 1]; buf[sz] = 0; fread(buf, sz, 1, fp); /*printf("[des %06ld] (string) %s\n", ftell(fp), buf);*/ return buf; }

inline void deserialize(FILE* fp, std::string& s) { s = deserialize_string(fp); }

// #define vser(fp, v) serialize(fp, v.size()); for (const auto& e : v) serialize(fp, e)
// #define vdes(fp, v) do { size_t sz; deserialize(fp, sz); v.resize(sz); for (size_t i = 0; i < sz; ++i) deserialize(fp, v[i]); } while (0)
template<typename T> inline void serialize(FILE* fp, const std::vector<T>& v) { serialize(fp, v.size()); for (const auto& e : v) serialize(fp, e); }
template<typename T> inline void deserialize(FILE* fp, std::vector<T>& v) { size_t sz; deserialize(fp, sz); v.resize(sz); for (size_t i = 0; i < sz; ++i) deserialize(fp, v[i]); }

#define vpser(fp, v) serialize(fp, (v).size()); for (const auto& e : (v)) serialize(fp, *e)
#define vpdes(fp, v, T) do { size_t sz; deserialize(fp, sz); for (auto& e : v) delete e; (v).resize(sz); for (size_t i = 0; i < sz; ++i) { (v)[i] = new T(); deserialize(fp, *(v)[i]); } } while (0)
// template<typename T> inline void serialize(FILE* fp, const std::vector<T*>& v) { serialize(fp, v.size()); for (const auto& e : v) serialize(fp, *e); }
// template<typename T> inline void deserialize(FILE* fp, std::vector<T*>& v) { size_t sz; deserialize(fp, sz); for (auto& e : v) delete e; v.resize(sz); for (size_t i = 0; i < sz; ++i) { v[i] = new T(); deserialize(fp, v[i]); } }

struct req {
    std::string var, val;
    tiny::token_type cond;
    req();
    req(const std::string& var_in, const std::string& val_in, tiny::token_type cond_in);
    bool met(const std::map<std::string,std::string>& state, const std::string& x_var = "", const std::string& x_val = "") const;
    static void convert_we(std::vector<req*>& v, const std::vector<we::restricter*>& rest);
    static bool met(const std::map<std::string,std::string>& state, const std::vector<req*>& requirements, const std::string& x_var = "", const std::string& x_val = "");
};

inline void serialize(FILE* fp, const req& r) {
    serialize(fp, r.var);
    serialize(fp, r.val);
    serialize(fp, r.cond);
}

inline void deserialize(FILE* fp, req& r) {
    r.var = deserialize_string(fp);
    r.val = deserialize_string(fp);
    deserialize(fp, r.cond);
}

struct setting {
    size_t inclusions = 0;
    size_t occurrences = 0;
    std::string value;
    std::string emits;
    std::vector<req*> requirements;
    int priority;
    setting();
    setting(const std::string& value_in, const std::string& emits_in, const std::vector<req*>& requirements_in, int priority_in);
};

inline void serialize(FILE* fp, const setting& s) {
    serialize(fp, s.value);
    serialize(fp, s.emits);
    vpser(fp, s.requirements);
    serialize(fp, s.priority);
    serialize(fp, s.inclusions);
    serialize(fp, s.occurrences);
}

inline void deserialize(FILE* fp, setting& s) {
    s.value = deserialize_string(fp);
    s.emits = deserialize_string(fp);
    vpdes(fp, s.requirements, req);
    deserialize(fp, s.priority);
    deserialize(fp, s.inclusions);
    deserialize(fp, s.occurrences);
}

struct option {
    static uint32_t id_counter;
    uint32_t id = 0;
    std::string name;
    std::string emits;
    std::vector<setting*> settings;
    option();
    option(const std::string& name_in, const std::string& emits_in, const std::vector<setting*> settings_in = std::vector<setting*>());
    void emit(size_t sidx, FILE* fp) const;
};

inline void serialize(FILE* fp, const option& o) {
    serialize(fp, o.id);
    serialize(fp, o.name);
    serialize(fp, o.emits);
    vpser(fp, o.settings);
}

inline void deserialize(FILE* fp, option& o) {
    deserialize(fp, o.id);
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
    inline setting* si(size_t i) const { return options[i]->settings[setting_index[i]]; }
    inline setting* si(size_t i) { return options[i]->settings[setting_index[i]]; }
    configuration();
    configuration(const std::vector<option*>& options_in, const std::map<std::string,std::string>& state_in, const std::vector<size_t>& setting_index_in, const std::vector<req*> requirements_in, option* opt, size_t sidx, std::vector<req*> reqs);
    void will_emit();
    void calc_pri();
    void branch(std::vector<configuration*>* config_pool, option* opt);
    static void begin(std::vector<configuration*>* config_pool, option* opt);
    std::string to_string() const;
    void penalize(const configuration& basis);
    void emit(FILE* fp) const;
};

inline void serialize(FILE* fp, const configuration& c) {
    serialize(fp, c.setting_index);
    serialize(fp, c.pri);
}

inline void deserialize(FILE* fp, configuration& c, const std::vector<option*>& options) {
    c.options = options;
    deserialize(fp, c.setting_index);
    deserialize(fp, c.pri);
}

struct wc: public we::configurator {
    size_t total_combinations;
    std::string emits;
    std::vector<setting*> settings;
    std::vector<option*> options;
    std::map<std::string,option*> option_map;
    std::vector<configuration*>* configurations;

    void replace_config(std::vector<configuration*>* new_config);

    wc(we::we& env);

    wc(FILE* fp);

    void save(FILE* fp) const;

    void load(FILE* fp);

    bool emit_and_penalize(FILE* stream);

    void sort();

    void blur();

    void normalize();

    virtual void emit(const std::string& output) override;

    virtual void branch(const std::string& desc, const std::string& var, const std::string& val, const std::string& emits, int priority, std::vector<we::restricter*> conditions) override;
};

} // namespace wc

#endif // included_wc_h
