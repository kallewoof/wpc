#include "wc.h"

namespace wc {

uint32_t option::id_counter = 0;

req::req() {}
req::req(const std::string& var_in, const std::string& val_in, tiny::token_type cond_in)
    : var(var_in)
    , val(val_in)
    , cond(cond_in) {}
bool req::met(const std::map<std::string,std::string>& state, const std::string& x_var, const std::string& x_val) const {
    if (x_var != var && !state.count(var)) return true; // unknown, i.e. an as yet traversed option introduces this constriction
    bool equals = x_var == var ? x_val == val : (!state.count(var) || state.at(var) == val);
    return cond == tiny::tok_eq ? equals : !equals;
}
void req::convert_we(std::vector<req*>& v, const std::vector<we::restricter*>& rest) {
    for (const auto& r : rest) {
        v.push_back(new req(r->var, r->val, r->condition));
    }
}
bool req::met(const std::map<std::string,std::string>& state, const std::vector<req*>& requirements, const std::string& x_var, const std::string& x_val) {
    for (const auto& r : requirements) if (!r->met(state, x_var, x_val)) return false;
    return true;
}

setting::setting() {}
setting::setting(const std::string& value_in, const std::string& emits_in, const std::vector<req*>& requirements_in, int priority_in)
    : value(value_in)
    , emits(emits_in)
    , requirements(requirements_in)
    , priority(priority_in) {}

option::option() {
    id = id_counter++;
}
option::option(const std::string& name_in, const std::string& emits_in, const std::vector<setting*> settings_in)
    : name(name_in)
    , emits(emits_in)
    , settings(settings_in) {
    id = id_counter++;
}
void option::emit(size_t sidx, FILE* fp) const {
    fprintf(fp, "%s%s=%s%s\n", emits.c_str(), name.c_str(), settings.at(sidx)->value.c_str(), settings.at(sidx)->emits.c_str());
}

configuration::configuration() {}
configuration::configuration(const std::vector<option*>& options_in, const std::map<std::string,std::string>& state_in, const std::vector<size_t>& setting_index_in, const std::vector<req*> requirements_in, option* opt, size_t sidx, std::vector<req*> reqs)
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
void configuration::will_emit() {
    for (size_t i = 0; i < options.size(); ++i) ++si(i)->inclusions;
}
void configuration::calc_pri() {
    pri = 0;
    for (size_t i = 0; i < options.size(); ++i) {
        pri += si(i)->priority;
    }
}
void configuration::branch(std::vector<configuration*>* config_pool, option* opt) {
    for (size_t i = 0; i < opt->settings.size(); ++i) {
        setting* s = opt->settings[i];
        if (req::met(state, s->requirements) && req::met(state, requirements, opt->name, s->value)) {
            config_pool->push_back(new configuration(options, state, setting_index, requirements, opt, i, s->requirements));
        }
    }
}
void configuration::begin(std::vector<configuration*>* config_pool, option* opt) {
    std::vector<option*> options;
    std::map<std::string,std::string> state;
    std::vector<size_t> setting_index;
    std::vector<req*> requirements;
    for (size_t i = 0; i < opt->settings.size(); ++i) {
        config_pool->push_back(new configuration(options, state, setting_index, requirements, opt, i, opt->settings[i]->requirements));
    }
}
std::string configuration::to_string() const {
    std::string r = "";//"pri=" + std::to_string(pri);
    for (size_t i = 0; i < options.size(); ++i) {
        r += (r == "" ? "" : ", ") + options[i]->name + "=" + si(i)->value;
    }
    return r;
}
void configuration::penalize(const configuration& basis) {
    // for any matching setting indices, we penalize ourselves
    last_penalty = 0;
    for (size_t i = 0; i < options.size(); ++i) {
        if (setting_index[i] == basis.setting_index[i]) {
            auto s = si(i);
            // base penalty
            float penalty = 0.01;
            // up to 0.02 extra based on how many occurrences remain
            penalty += 0.02 * s->inclusions / s->occurrences;
            // for pri>0, reduce penalty by (5*pri)%; for pri<0, increase it
            penalty *= 1.0 - 0.05 * s->priority;
            last_penalty += penalty;
            pri -= penalty;
        }
    }
}
void configuration::emit(FILE* fp) const {
    for (size_t i = 0; i < options.size(); ++i) {
        options.at(i)->emit(setting_index.at(i), fp);
    }
}

void wc::replace_config(std::vector<configuration*>* new_config) {
    for (configuration* c : *configurations) delete c;
    delete configurations;
    configurations = new_config;
}

wc::wc(we::we& env) {
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
    // calculate occurrences
    for (auto& c : *configurations) {
        for (size_t i = 0; i < c->options.size(); ++i) ++c->si(i)->occurrences;
    }
    // printf("generated %zu configurations\n", configurations->size());
    total_combinations = configurations->size();
    normalize();
    blur();
    sort();
}

wc::wc(FILE* fp) {
    configurations = new std::vector<configuration*>();
    load(fp);
    // size_t idx = configurations->size();
    // for (auto& c : *configurations) { idx--; c->old_idx = idx; }
    // for (auto& c : *configurations) { printf("- %s\n", c->to_string().c_str()); }
}

void wc::save(FILE* fp) const {
    serialize(fp, total_combinations);
    serialize(fp, emits);
    vpser(fp, options);
    vpser(fp, *configurations);
    // size_t idx = configurations->size();
    // for (auto& c : *configurations) { idx--; printf("- %zu->%zu %s\n", c->old_idx, idx, c->to_string().c_str()); }
}

void wc::load(FILE* fp) {
    deserialize(fp, total_combinations);
    emits = deserialize_string(fp);
    vpdes(fp, options, option);
    size_t sz;
    deserialize(fp, sz);
    for (auto& e : *configurations) delete e;
    (*configurations).resize(sz);
    for (size_t i = 0; i < sz; ++i) {
        (*configurations)[i] = new configuration();
        deserialize(fp, *(*configurations)[i], options);
    }
}

bool wc::emit_and_penalize(FILE* stream) {
    if (!configurations || configurations->size() == 0) return false;
    auto cfg = configurations->back();
    cfg->will_emit();
    configurations->pop_back();
    for (auto& c : *configurations) c->penalize(*cfg);
    fprintf(stream, "%s", emits.c_str());
    cfg->emit(stream);
    sort();
    return true;
}

void wc::sort() {
    std::sort((*configurations).begin(), (*configurations).end(), [](const configuration* a, const configuration* b) { return a->pri < b->pri; });
}

void wc::blur() {
    for (auto c : *configurations) {
        c->pri += (frand() - 0.5) / 10;
    }
}

void wc::normalize() {
    float min = 1e99, max = -1e99;
    for (auto c : *configurations) {
        c->calc_pri();
        if (min > c->pri) min = c->pri;
        if (max < c->pri) max = c->pri;
    }
    float len = max - min;
    if (len == 0) {
        for (auto c : *configurations) {
            c->pri = 0;
        }
    } else {
        // adjust to 0..1
        float add = -min;
        for (auto c : *configurations) {
            c->pri = (c->pri + add) / len;
        }
    }
}

void wc::emit(const std::string& output) {
    emits += output;
}

void wc::branch(const std::string& desc, const std::string& var, const std::string& val, const std::string& emits, int priority, std::vector<we::restricter*> conditions) {
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

} // namespace wc
