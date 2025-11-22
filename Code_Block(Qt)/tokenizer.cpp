#include "tokenizer.h"

static std::set<std::string> keywords = {
    "int","float","if","else","while","for","break","continue","return"
};
static std::set<char> operators = {
    '+','-','*','/','=','<','>','!','&','|','%'
};
static std::set<char> delimiters = {
    '(',')','{','}','[',']',',',';',':'
};

// Tokenize while tracking line and column (1-based), returns vector<TokenItem with line/col)
std::vector<TokenItem> tokenizeWithDFA(const std::string &input,
                                       const DFA &dfaId,
                                       const DFA &dfaNum)
{
    std::vector<TokenItem> out;
    int i = 0, n = (int)input.size();
    int line = 1, col = 1;
    while (i < n) {
        char c = input[i];
        if (isWhitespace(c)) {
            // update line/col for whitespace
            if (c == '\n') { ++line; col = 1; ++i; continue; }
            if (c == '\r') { ++i; /* ignore CR alone, or will be followed by LF */ col = 1; continue; }
            ++i; ++col; continue;
        }

        // start position
        int startLine = line, startCol = col;

        if (operators.count(c)) {
            out.push_back({ "Operator", std::string(1,c), startLine, startCol });
            ++i; ++col; continue;
        }
        if (delimiters.count(c)) {
            out.push_back({ "Delimiter", std::string(1,c), startLine, startCol });
            ++i; ++col; continue;
        }

        int lenId = dfaLongestMatch(dfaId, input, i);
        int lenNum = dfaLongestMatch(dfaNum, input, i);

        if (lenId == 0 && lenNum == 0) {
            out.push_back({ "Unknown", std::string(1,c), startLine, startCol });
            ++i; ++col;
        } else {
            if (lenId >= lenNum) {
                std::string tok = input.substr(i, lenId);
                std::string type = keywords.count(tok) ? "Keyword" : "Identifier";
                out.push_back({ type, tok, startLine, startCol });
                // update i, line/col (tokens here should not contain newlines, but guard anyway)
                for (int k = 0; k < lenId; ++k) {
                    if (input[i] == '\n') { ++line; col = 1; ++i; }
                    else { ++i; ++col; }
                }
            } else {
                std::string tok = input.substr(i, lenNum);
                out.push_back({ "Number", tok, startLine, startCol });
                for (int k = 0; k < lenNum; ++k) {
                    if (input[i] == '\n') { ++line; col = 1; ++i; }
                    else { ++i; ++col; }
                }
            }
        }
    }
    return out;
}
