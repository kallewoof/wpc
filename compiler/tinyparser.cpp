// Copyright (c) 2018 Karl-Johan Alm
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <compiler/tinyparser.h>

namespace tiny {

std::vector<std::string> pdt;
std::string pdts = "";
static size_t ctr = 0;
struct pdo {
    pdo(const std::string& v) {
        pdt.push_back(v);
        ctr++;
        printf("%s%s [%zu] {\n", pdts.c_str(), v.c_str(), ctr);
        pdts += "  ";
    }
    ~pdo() {
        pdts = pdts.substr(2);
        pdt.pop_back();
        if (pdt.size()) printf("%s} // %s\n", pdts.c_str(), pdt.back().c_str());
    }
};

token_t* head = nullptr;
inline size_t count(token_t* head, token_t* t) {
    size_t i = 0;
    for (token_t* q = head; q && q != t; q = q->next) i++;
    return i;
}

// std::string indent = "";
#define try(parser) \
    /*indent += " ";*/ \
    x = parser(s); \
    /*indent = indent.substr(1);*/\
    if (x) {\
        /*printf("#%zu [caching %s=%p(%s, %s)]\n", count(head, *s), x->to_string().c_str(), *s, *s ? token_type_str[(*s)->token] : "<null>", *s ? (*s)->value ?: "<nil>" : "<null>");\
        printf("GOT " #parser ": %s\n", x->to_string().c_str());*/\
        return x;\
    }
#define DEBUG_PARSER(s) //pdo __pdo(s) //printf("- %s\n", s)

st_t* parse_expr(token_t** s) {
    DEBUG_PARSER("expr");
    st_t* x;
    token_t* pcv = *s;
    try(parse_pre);
    try(parse_option);
    try(parse_ignored);
    return nullptr;
}

st_t* parse_ignored(token_t** s) {
    // # <blob> \n | <consumed>
    token_t* prev = *s;
    while ((*s)->token == tok_line_comment || (*s)->token == tok_consumable) {
        *s = (*s)->next;
    }
    if (prev == *s) return nullptr;
    return parse_expr(s);
}

st_t* parse_pre(token_t** s) {
    // '}}' token with content
    if ((*s)->token == tok_rpre) {
        pre_t* t = new pre_t((*s)->value);
        *s = (*s)->next;
        return t;
    }
    return nullptr;
}

st_t* parse_option(token_t** s) {
    // symbol lcurly [values] rcurly
    token_t* r = *s;
    if (r->token != tok_symbol || !r->next || r->next->token != tok_lcurly || !r->next->next) return nullptr;
    std::string option_name = r->value;
    r = r->next->next;
    std::vector<st_c> values;
    while (r && r->token != tok_rcurly) {
        st_t* v = parse_value(&r);
        if (!v) {
            // we also allow pre's
            v = parse_pre(&r);
        }
        if (!v) return nullptr;
        values.emplace_back(v);
    }
    if (r->token != tok_rcurly) return nullptr;
    *s = r->next;
    return new option_t(option_name, values);
}

// options
st_t* parse_value(token_t** s) {
    /*
    normal value 1(condition): "rprot";
    */
    token_t* r = *s;
    if (r->token != tok_symbol) return nullptr;
    int priority = 0;
    while (r->next && r->token == tok_symbol && value_t::prioritize(r->value, priority)) {
        r = r->next;
    }
    if (!r->next || r->token != tok_symbol || std::string("value") != r->value) return nullptr;
    r = r->next;
    if (!r->next || (r->token != tok_number && r->token != tok_symbol)) return nullptr;
    std::string value = r->value;
    std::string desc = "";
    std::string pre = "";
    st_t* condition = nullptr;
    r = r->next;
    if (r->token == tok_lparen) {
        // condition
        r = r->next;
        condition = parse_condition(&r);
        if (!r || r->token != tok_rparen) { if (condition) delete condition; return nullptr; }
        r = r->next;
    }
    if (r->token == tok_colon) {
        r = r->next;
        if (!r || !r->next || r->token != tok_string) { if (condition) delete condition; return nullptr; }
        desc = r->value;
        r = r->next;
    }
    if (r->token == tok_consumable) r = r->next;
    if (r->token == tok_rpre) {
        pre = r->value;
    } else if (!r || r->token != tok_semicolon) { if (condition) delete condition; return nullptr; }
    *s = r->next;
    return new value_t(value, desc, priority, pre, condition);
}

st_t* parse_condition(token_t** s) {
    // require var comparator expr
    token_t* r = *s;
    if (r->token != tok_symbol || std::string("require") != r->value || !r->next || !r->next->next || !r->next->next->next) return nullptr;
    r = r->next;
    if (r->token != tok_symbol) return nullptr;
    std::string var = r->value;
    r = r->next;
    switch (r->token) {
    case tok_eq:
    case tok_ne:
        break;
    default:
        return nullptr;
    }
    token_type comparator = r->token;
    r = r->next;
    if (r->token != tok_number && r->token != tok_symbol) return nullptr;
    std::string val = r->value;
    *s = r->next;
    return new cond_t(var, val, comparator);
}

st_t* treeify(token_t** tokens, bool multi) {
    head = *tokens;
    token_t* s = *tokens;
    st_t* value = parse_expr(&s);
    head = nullptr;
    if ((!s || multi) && value) {
        value = value->clone();
    }
    if (s && !multi) {
        throw std::runtime_error(strprintf("failed to treeify tokens around token %s", s->value ?: token_type_str[s->token]));
        return nullptr;
    }
    if (multi) *tokens = s;
    return value;
}

} // namespace tiny
