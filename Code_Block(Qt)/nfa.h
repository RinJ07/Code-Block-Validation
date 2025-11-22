#ifndef NFA_H
#define NFA_H

#include <vector>
#include <set>
#include <map>
#include <string>
#include <cctype>
#include <algorithm>

// Character helpers
inline bool isLetter(char c) { return std::isalpha((unsigned char)c) != 0; }
inline bool isDigit(char c) { return std::isdigit((unsigned char)c) != 0; }
inline bool isIdentifierChar(char c) { return isLetter(c) || isDigit(c) || c == '_'; }
inline bool isWhitespace(char c) { return c==' '||c=='\t'||c=='\n'||c=='\r'||c=='\f'||c=='\v'; }
inline bool isPrintable(char c) { return (unsigned char)c >= 32 && (unsigned char)c < 127; }

// NFA representation (Thompson-style)
struct NFAState {
    int id;
    std::map<char, std::set<int>> trans; // labeled transitions
    std::set<int> eps;                   // epsilon transitions
};

struct NFA {
    std::vector<NFAState> states;
    int start = -1;
    std::set<int> accepts;

    int newState() {
        NFAState s;
        s.id = (int)states.size();
        states.push_back(s);
        return s.id;
    }

    void addTrans(int from, char c, int to) { states[from].trans[c].insert(to); }
    void addEps(int from, int to) { states[from].eps.insert(to); }
};

struct Fragment { int start, accept; };

// Thompson helpers: build small NFAs and combine (concatenation, union, star, plus, optional)
Fragment makeChar(NFA &n, char c);
Fragment makeCharClass(NFA &n, const std::vector<char>& allowed);
Fragment concatFrag(NFA &n, const Fragment &a, const Fragment &b);
Fragment altFrag(NFA &n, const Fragment &a, const Fragment &b);
Fragment starFrag(NFA &n, const Fragment &a);
Fragment plusFrag(NFA &n, const Fragment &a);
Fragment optFrag(NFA &n, const Fragment &a);

// Build NFAs for the project regexes using Thompson construction
void pushRange(std::vector<char>& v, char a, char b);
NFA buildIdentifierNFA_thompson();
NFA buildNumberNFA_thompson();

#endif // NFA_H
