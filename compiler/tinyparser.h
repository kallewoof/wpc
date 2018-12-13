// Copyright (c) 2018 Karl-Johan Alm
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef included_tiny_parser_h_
#define included_tiny_parser_h_

#include <compiler/tinytokenizer.h>
#include <compiler/tinyast.h>

#include <map>

namespace tiny {

st_t* treeify(token_t** tokens, bool multi = false);

st_t* parse_ignored(token_t** s);

st_t* parse_pre(token_t** s);
st_t* parse_option(token_t** s);

// options
st_t* parse_value(token_t** s);
st_t* parse_condition(token_t** s);

} // namespace tiny

#endif // included_tiny_parser_h_
