#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "dfa.h"
#include <string>
#include <vector>
#include <set>

// TokenItem now includes line+column
struct TokenItem { std::string type, text; int line, col; };

// Tokenize while tracking line and column (1-based), returns vector<TokenItem with line/col)
std::vector<TokenItem> tokenizeWithDFA(const std::string &input,
                                       const DFA &dfaId,
                                       const DFA &dfaNum);

#endif // TOKENIZER_H
