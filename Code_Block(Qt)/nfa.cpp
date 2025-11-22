#include "nfa.h"

// create NFA that accepts a single character c
Fragment makeChar(NFA &n, char c) {
    int s = n.newState();
    int t = n.newState();
    if (c == 0) { // treat 0 as epsilon (not used here normally)
        n.addEps(s, t);
    } else {
        n.addTrans(s, c, t);
    }
    return {s,t};
}

// create NFA for a character class given a vector<char> allowed
Fragment makeCharClass(NFA &n, const std::vector<char>& allowed) {
    int s = n.newState();
    int t = n.newState();
    for (char c : allowed) n.addTrans(s, c, t);
    return {s,t};
}

// concatenation: a followed by b
Fragment concatFrag(NFA &n, const Fragment &a, const Fragment &b) {
    n.addEps(a.accept, b.start);
    return {a.start, b.accept};
}

// alternation (a|b)
Fragment altFrag(NFA &n, const Fragment &a, const Fragment &b) {
    int s = n.newState();
    int t = n.newState();
    n.addEps(s, a.start);
    n.addEps(s, b.start);
    n.addEps(a.accept, t);
    n.addEps(b.accept, t);
    return {s,t};
}

// Kleene star (a*)
Fragment starFrag(NFA &n, const Fragment &a) {
    int s = n.newState();
    int t = n.newState();
    n.addEps(s, a.start);
    n.addEps(s, t);
    n.addEps(a.accept, a.start);
    n.addEps(a.accept, t);
    return {s,t};
}

// plus (a+): equivalent to concat(a, a*)
Fragment plusFrag(NFA &n, const Fragment &a) {
    return concatFrag(n, a, starFrag(n, a));
}

// optional (a?)
Fragment optFrag(NFA &n, const Fragment &a) {
    int s = n.newState();
    int t = n.newState();
    n.addEps(s, a.start);
    n.addEps(s, t);
    n.addEps(a.accept, t);
    return {s,t};
}

// Helper: fill vector with range [a..b] inclusive
void pushRange(std::vector<char>& v, char a, char b) {
    for (unsigned char c = (unsigned char)a; c <= (unsigned char)b; ++c) v.push_back((char)c);
}

// identifier: [a-zA-Z_][a-zA-Z0-9_]*
NFA buildIdentifierNFA_thompson() {
    NFA n;
    // first class: letters + underscore
    std::vector<char> letters;
    pushRange(letters, 'a','z'); pushRange(letters,'A','Z');
    letters.push_back('_');
    Fragment f1 = makeCharClass(n, letters);
    // second class: letters + digits + underscore
    std::vector<char> idchars = letters;
    pushRange(idchars, '0','9');
    Fragment f2 = makeCharClass(n, idchars);
    // f2* (Kleene star)
    Fragment f2star = starFrag(n, f2);
    // concat f1 f2*
    Fragment full = concatFrag(n, f1, f2star);
    // setup NFA
    n.start = full.start;
    n.accepts.insert(full.accept);
    return n;
}

// number: [0-9]+(\.[0-9]+)?  -> digits+ optionally ('.' digits+)
NFA buildNumberNFA_thompson() {
    NFA n;
    // digit class
    std::vector<char> digits;
    pushRange(digits, '0','9');
    Fragment digit = makeCharClass(n, digits);
    // digits+  -> we implement plus as one digit then star(digit)
    Fragment digitsPlus = concatFrag(n, digit, starFrag(n, digit));
    // fractional part: '.' followed by digits+
    Fragment dot = makeChar(n, '.');
    Fragment frac = concatFrag(n, dot, digitsPlus);
    // optional fractional: (frac)?
    Fragment fracOpt = optFrag(n, frac);
    // final: digitsPlus concat fracOpt
    Fragment full = concatFrag(n, digitsPlus, fracOpt);
    n.start = full.start;
    n.accepts.insert(full.accept);
    return n;
}
