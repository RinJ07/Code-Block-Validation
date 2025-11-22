#ifndef DFA_H
#define DFA_H

#include "nfa.h"
#include <set>
#include <map>
#include <vector>
#include <QVector> // For the trace path

// DFA representation
struct DFA {
    std::map<std::set<int>, int> mapping;
    std::vector<std::set<int>> rev;            // reverse mapping: id -> NFA set
    std::vector<std::map<char,int>> trans;     // per-state labeled transitions
    std::set<int> accepts;
    int start = 0;
};

// Epsilon closure
std::set<int> epsClosure(const NFA &n, const std::set<int> &states);

// Move on char
std::set<int> moveOnChar(const NFA &n, const std::set<int> &states, char c);

// Subset construction (NFA -> DFA) with reachable pruning
DFA subsetConstruction(const NFA &n);

// DFA longest match
int dfaLongestMatch(const DFA &d, const std::string &s, int pos);

// Modified version of dfaLongestMatch that returns the path taken
std::pair<int, QVector<int>> dfaLongestMatchWithTrace(const DFA &d, const std::string &s, int pos);

#endif // DFA_H
