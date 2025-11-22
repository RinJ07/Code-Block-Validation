#include "dfa.h"

// Epsilon closure
std::set<int> epsClosure(const NFA &n, const std::set<int> &states) {
    std::set<int> res = states;
    std::vector<int> stvec(states.begin(), states.end());
    while (!stvec.empty()) {
        int s = stvec.back(); stvec.pop_back();
        if (s < 0 || s >= (int)n.states.size()) continue;
        for (int nxt : n.states[s].eps) {
            if (!res.count(nxt)) { res.insert(nxt); stvec.push_back(nxt); }
        }
    }
    return res;
}

// Move on char
std::set<int> moveOnChar(const NFA &n, const std::set<int> &states, char c) {
    std::set<int> res;
    for (int s : states) {
        if (s < 0 || s >= (int)n.states.size()) continue;
        auto it = n.states[s].trans.find(c);
        if (it != n.states[s].trans.end()) {
            for (int nxt : it->second) res.insert(nxt);
        }
    }
    return res;
}

// Subset construction (NFA -> DFA) with reachable pruning
DFA subsetConstruction(const NFA &n) {
    DFA d;
    std::set<int> startSet = epsClosure(n, { n.start });
    d.mapping[startSet] = 0;
    d.rev.push_back(startSet);
    d.trans.emplace_back();

    for (size_t i = 0; i < d.rev.size(); ++i) {
        auto curSet = d.rev[i];
        // mark accept
        for (int a : n.accepts) if (curSet.count(a)) { d.accepts.insert((int)i); break; }
        // consider all possible input chars (only ASCII subset 0..127)
        for (int c = 0; c < 128; ++c) {
            char ch = (char)c;
            auto moved = moveOnChar(n, curSet, ch);
            if (moved.empty()) continue;
            auto epsed = epsClosure(n, moved);
            if (!epsed.empty()) {
                if (!d.mapping.count(epsed)) {
                    int id = (int)d.rev.size();
                    d.mapping[epsed] = id;
                    d.rev.push_back(epsed);
                    d.trans.emplace_back();
                }
                d.trans[i][ch] = d.mapping[epsed];
            }
        }
    }

    // reachable-state pruning from d.start (0)
    std::vector<char> seen(d.rev.size(), 0);
    std::vector<int> q; q.push_back(d.start); seen[d.start] = 1;
    for (size_t idx = 0; idx < q.size(); ++idx) {
        int u = q[idx];
        for (auto &kv : d.trans[u]) {
            int v = kv.second;
            if (!seen[v]) { seen[v] = 1; q.push_back(v); }
        }
    }
    // remap states to compact indices
    std::map<int,int> remap;
    int newId = 0;
    for (int i = 0; i < (int)seen.size(); ++i) if (seen[i]) remap[i] = newId++;
    DFA d2;
    d2.start = remap[d.start];
    d2.rev.resize(newId);
    d2.trans.resize(newId);
    for (auto &p : remap) {
        d2.rev[p.second] = d.rev[p.first]; // copy NFA sets
        if (d.accepts.count(p.first)) d2.accepts.insert(p.second);
        // copy transitions
        for (auto &kv : d.trans[p.first]) {
            int to = kv.second;
            if (!seen[to]) continue;
            char c = kv.first;
            d2.trans[p.second][c] = remap[to];
        }
    }
    return d2;
}

// DFA longest match
int dfaLongestMatch(const DFA &d, const std::string &s, int pos) {
    if (d.rev.empty()) return 0;
    int cur = d.start;
    if (cur < 0 || cur >= (int)d.rev.size()) return 0;
    int lastAcceptPos = -1;
    for (int i = pos; i < (int)s.size(); ++i) {
        char c = s[i];
        auto it = d.trans[cur].find(c);
        if (it == d.trans[cur].end()) break;
        cur = it->second;
        if (d.accepts.count(cur)) lastAcceptPos = i;
    }
    return lastAcceptPos >= 0 ? (lastAcceptPos - pos + 1) : 0;
}

// Modified version of dfaLongestMatch that returns the path taken
std::pair<int, QVector<int>> dfaLongestMatchWithTrace(const DFA &d, const std::string &s, int pos) {
    if (d.rev.empty()) return {0, {}};
    int cur = d.start;
    if (cur < 0 || cur >= (int)d.rev.size()) return {0, {}};

    int lastAcceptPos = -1;
    QVector<int> path;
    path.append(cur); // Start with the initial state

    for (int i = pos; i < (int)s.size(); ++i) {
        char c = s[i];
        auto it = d.trans[cur].find(c);
        if (it == d.trans[cur].end()) break;
        cur = it->second;
        path.append(cur); // Record the state we moved to
        if (d.accepts.count(cur)) lastAcceptPos = i;
    }

    return {lastAcceptPos >= 0 ? (lastAcceptPos - pos + 1) : 0, path};
}
