#include "pda.h"

// PDA for balanced parentheses
bool checkPDA(const std::string &s) {
    std::stack<char> stk;
    std::map<char,char> pairs = { {')','('}, {'}','{'}, {']','['} };
    for (char c : s) {
        if (c=='('||c=='{'||c=='[') stk.push(c);
        else if (c==')'||c=='}'||c==']') {
            if (stk.empty() || stk.top() != pairs[c]) return false;
            stk.pop();
        }
    }
    return stk.empty();
}
