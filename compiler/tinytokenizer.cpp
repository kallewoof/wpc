// Copyright (c) 2018 Karl-Johan Alm
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <compiler/tinytokenizer.h>

namespace tiny {

token_t* tokenize(const char* s) {
    token_t* head = nullptr;
    token_t* tail = nullptr;
    token_t* prev = nullptr;
    bool open = false;
    bool finalized = true;
    bool spaced = false;
    char finding = 0; char finding2 = 0;
    int prefsuflen = 0;
    token_type restrict_type = tok_undef;
    size_t token_start = 0;
    size_t i;
    int consumes;
    size_t line = 0, col = 0;
    for (i = 0; s[i]; ++i) {
        if (s[i] == '\n') {
            line++;
            col = 0;
        } else {
            col++;
        }
        // if we are finding a character, keep reading until we find it
        if (finding) {
            if (finding2 && (!s[i+1] || s[i+1] != finding2)) continue;
            if (s[i] != finding) continue;
            finding = finding2 = 0;
            open = false;
            if (s[i] == '\n') {
                // we actually want to parse this separately so we rewind i
                i--; col--;
            }
            continue; // we move one extra step, or "foo" will be read in as "foo
        }
        auto token = determine_token(s[i], i ? s[i-1] : 0, i - token_start, restrict_type, spaced ? tok_ws : tail ? tail->token : tok_undef, consumes);
        if (consumes) {
            // we only support 1 token consumption at this point
            tail->token = tok_consumable;
        }
        // printf("token = %s\n", token_type_str[token]);
        if (token == tok_consumable && tail->token == tok_consumable) {
            throw std::runtime_error(strprintf("tokenization failure 0 at character '%c'", s[i]));
            delete head;
            return nullptr;
        }
        if ((token == tok_hex || token == tok_bin) && tail->token == tok_number) tail->token = tok_consumable;
        // if whitespace, close
        if (token == tok_ws) {
            open = false;
            restrict_type = tok_undef;
            spaced = true;
        } else {
            spaced = false;
        }
        // if open, see if it stays open
        if (open) {
            open = token == tail->token;
            if (!open) restrict_type = tok_undef;
        }
        if (!open) {
            if (tail && tail->token == tok_consumable) {
                if (token == tok_hex || token == tok_bin) {
                    restrict_type = token;
                    delete tail;
                    if (head == tail) head = prev;
                    tail = prev;
                }
            } else if (!finalized) {
                size_t content_start = token_start - (prefsuflen > 1) + prefsuflen;
                size_t content_end = i - prefsuflen;
                if (content_end < content_start) content_end = content_start;
                tail->value = strndup(&s[content_start], content_end - content_start);
                prefsuflen = 0;
                finalized = true;
            }
            switch (token) {
            case tok_string:
            case tok_line_comment:
            case tok_lpre:
                if (token == tok_lpre) {
                    finding = finding2 = '}';
                    prefsuflen = 2;
                } else {
                    finding = token == tok_string ? s[i] : '\n';
                    prefsuflen = 1;
                }
            case tok_symbol:
            case tok_number:
            case tok_consumable:
                prev = tail;
                finalized = false;
                token_start = i;
                tail = new token_t(token, tail);
                tail->line = line;
                tail->col = col;
                if (!head) head = tail;
                open = true;
                break;
            case tok_set:
            case tok_lt:
            case tok_gt:
            case tok_not:
            case tok_lparen:
            case tok_rparen:
            case tok_rpre:
            case tok_mul:
            case tok_plus:
            case tok_minus:
            case tok_concat:
            case tok_comma:
            case tok_lcurly:
            case tok_rcurly:
            case tok_lbracket:
            case tok_rbracket:
            case tok_colon:
            case tok_semicolon:
            case tok_div:
            case tok_hex:
            case tok_bin:
            case tok_land:
            case tok_lor:
            case tok_pow:
            case tok_eq:
            case tok_le:
            case tok_ge:
            case tok_ne:
            case tok_arrow:
                if (tail && tail->token == tok_consumable) {
                    delete tail;
                    if (head == tail) head = prev;
                    tail = prev;
                }
                prev = tail;
                tail = new token_t(token, tail);
                tail->line = line;
                tail->col = col;
                tail->value = strndup(&s[i], 1 /* misses 1 char for concat/hex/bin, but irrelevant */);
                if (!head) head = tail;
                break;
            case tok_ws:
                break;
            case tok_undef:
                throw std::runtime_error(strprintf("tokenization failure 1 at character '%c'", s[i]));
                delete head;
                return nullptr;
            }
        }
        // for (auto x = head; x; x = x->next) printf(" %s", token_type_str[x->token]); printf("\n");
    }
    if (!finalized) {
        tail->value = strndup(&s[token_start], i-token_start);
        finalized = true;
    }
    while (head && (head->token == tok_consumable || head->token == tok_line_comment)) {
        token_t* tmp = head;
        head = head->next;
        tmp->next = nullptr;
        delete tmp;
    }
    for (token_t* t = head; t; t = t->next) {
        while (t->next && (t->next->token == tok_consumable || t->next->token == tok_line_comment)) {
            token_t* tmp = t->next;
            t->next = tmp->next;
            tmp->next = nullptr;
            delete tmp;
        }
    }
    return head;
}

} // namespace tiny
