
// main.cpp
// Automata Front-End Simulator with simple Automaton Visualizer
// - NFAs for Identifier and Number
// - subset construction -> DFAs used for lexical analysis (longest-match)
// - PDA (stack) used for balanced parentheses/braces/brackets
// - Win32 GUI, UTF-8/Unicode-safe
// - Simple visualizer draws DFA states and transitions in a child window

#include <windows.h>
#include <string>
#include <vector>
#include <stack>
#include <map>
#include <set>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cmath>

///// Utility: Unicode conversion /////
std::string ws2s(const std::wstring &wstr) {
    if (wstr.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string s(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &s[0], n, NULL, NULL);
    return s;
}
std::wstring s2ws(const std::string &s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), NULL, 0);
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &w[0], n);
    return w;
}

///// NFA representation (Thompson-style, simple) /////
struct NFAState {
    int id;
    std::map<char, std::set<int>> trans;
    std::set<int> eps;
};

struct NFA {
    std::vector<NFAState> states;
    int start = 0;
    std::set<int> accepts;
    int newState() {
        NFAState s; s.id = (int)states.size();
        states.push_back(s);
        return s.id;
    }
    void addTrans(int from, char c, int to) { states[from].trans[c].insert(to); }
    void addEps(int from, int to) { states[from].eps.insert(to); }
};

///// DFA representation /////
struct DFA {
    std::map<std::set<int>, int> mapping;
    std::vector<std::set<int>> rev;
    std::vector<std::map<char,int>> trans;
    std::set<int> accepts;
    int start = 0;
};

//// character helpers ////
inline bool isLetter(char c) { return std::isalpha((unsigned char)c) != 0; }
inline bool isDigit(char c) { return std::isdigit((unsigned char)c) != 0; }
inline bool isIdentifierChar(char c) { return isLetter(c) || isDigit(c) || c == '_'; }
inline bool isWhitespace(char c) { return c==' '||c=='\t'||c=='\n'||c=='\r'||c=='\f'||c=='\v'; }
inline bool isPrintable(char c) { return c >= 32 && c < 127; }

//// Build Identifier NFA ////
NFA buildIdentifierNFA() {
    NFA n;
    int s0 = n.newState();
    int s1 = n.newState();
    n.start = s0;
    n.accepts.insert(s1);
    for (int c = 0; c < 128; ++c) {
        char ch = (char)c;
        if (isLetter(ch) || ch == '_') n.addTrans(s0, ch, s1);
        if (isIdentifierChar(ch)) n.addTrans(s1, ch, s1);
    }
    return n;
}

//// Build Number NFA ////
NFA buildNumberNFA() {
    NFA n;
    int s0 = n.newState();
    int s1 = n.newState();
    int s2 = n.newState();
    int s3 = n.newState();
    n.start = s0;
    n.accepts.insert(s1);
    n.accepts.insert(s3);
    for (int c = 0; c < 128; ++c) {
        char ch = (char)c;
        if (isDigit(ch)) {
            n.addTrans(s0, ch, s1);
            n.addTrans(s1, ch, s1);
            n.addTrans(s2, ch, s3);
            n.addTrans(s3, ch, s3);
        }
    }
    n.addTrans(s1, '.', s2);
    return n;
}

//// Epsilon closure ////
std::set<int> epsClosure(const NFA &n, const std::set<int> &states) {
    std::set<int> res = states;
    std::vector<int> stack(states.begin(), states.end());
    while (!stack.empty()) {
        int st = stack.back(); stack.pop_back();
        for (int nxt : n.states[st].eps) {
            if (!res.count(nxt)) { res.insert(nxt); stack.push_back(nxt); }
        }
    }
    return res;
}

//// Move on char ////
std::set<int> moveOnChar(const NFA &n, const std::set<int> &states, char c) {
    std::set<int> res;
    for (int st : states) {
        auto it = n.states[st].trans.find(c);
        if (it != n.states[st].trans.end()) {
            for (int nxt : it->second) res.insert(nxt);
        }
    }
    return res;
}

//// Subset construction ////
DFA subsetConstruction(const NFA &n) {
    DFA d;
    std::set<int> startSet = epsClosure(n, { n.start });
    d.mapping[startSet] = 0;
    d.rev.push_back(startSet);
    d.trans.emplace_back();
    for (size_t i = 0; i < d.rev.size(); ++i) {
        auto curSet = d.rev[i];
        for (int a : n.accepts)
            if (curSet.count(a)) { d.accepts.insert((int)i); break; }
        for (int c = 0; c < 128; ++c) {
            char ch = (char)c;
            auto moved = moveOnChar(n, curSet, ch);
            if (!moved.empty()) {
                auto epsed = epsClosure(n, moved);
                if (!epsed.empty()) {
                    if (!d.mapping.count(epsed)) {
                        int id = (int)d.rev.size();
                        d.mapping[epsed] = id;
                        d.rev.push_back(epsed);
                        d.trans.emplace_back();
                    }
                    int to = d.mapping[epsed];
                    d.trans[i][ch] = to;
                }
            }
        }
    }
    d.start = 0;
    return d;
}

//// DFA longest match ////
int dfaLongestMatch(const DFA &d, const std::string &s, int pos) {
    int cur = d.start;
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

//// TOKENIZER ////
struct TokenItem { std::string type, text; };

std::set<std::string> keywords = {
    "int","float","if","else","while","for","break","continue","return"
};
std::set<char> operators = {
    '+','-','*','/','=','<','>','!','&','|','%'
};
std::set<char> delimiters = {
    '(',')','{','}','[',']',',',';',':'
};

std::vector<TokenItem> tokenizeWithDFA(const std::string &input,
                                      const DFA &dfaId,
                                      const DFA &dfaNum)
{
    std::vector<TokenItem> out;
    int i = 0, n = (int)input.size();
    while (i < n) {
        char c = input[i];
        if (isWhitespace(c)) { ++i; continue; }

        if (operators.count(c)) {
            out.push_back({ "Operator", std::string(1,c) });
            ++i; continue;
        }
        if (delimiters.count(c)) {
            out.push_back({ "Delimiter", std::string(1,c) });
            ++i; continue;
        }

        int lenId = dfaLongestMatch(dfaId, input, i);
        int lenNum = dfaLongestMatch(dfaNum, input, i);

        if (lenId == 0 && lenNum == 0) {
            out.push_back({ "Unknown", std::string(1,c) });
            ++i;
        } else {
            if (lenId >= lenNum) {
                std::string tok = input.substr(i, lenId);
                std::string type = keywords.count(tok) ? "Keyword" : "Identifier";
                out.push_back({ type, tok });
                i += lenId;
            } else {
                std::string tok = input.substr(i, lenNum);
                out.push_back({ "Number", tok });
                i += lenNum;
            }
        }
    }
    return out;
}

//// PDA for balanced parentheses ////
bool checkPDA(const std::string &s) {
    std::stack<char> stk;
    std::map<char,char> pairs = {
        {')','('}, {'}','{'}, {']','['}
    };
    for (char c : s) {
        if (c=='('||c=='{'||c=='[') stk.push(c);
        else if (c==')'||c=='}'||c==']') {
            if (stk.empty() || stk.top() != pairs[c]) return false;
            stk.pop();
        }
    }
    return stk.empty();
}

//// GUI GLOBALS ////
HWND hInput, hTokens, hSyntax, hVisual;
DFA g_dfaId, g_dfaNum;
bool g_haveDfas = false;
int g_visualChoice = 0; // 0 none, 1 id, 2 num

// control IDs
constexpr int ID_BTN_ANALYZE = 101;
constexpr int ID_BTN_SHOW_ID = 102;
constexpr int ID_BTN_SHOW_NUM = 103;

void AnalyzeCode(HWND hwndInput, HWND hwndTokens, HWND hwndSyntax,
                 const DFA &dfaId, const DFA &dfaNum)
{
    int len = GetWindowTextLengthW(hwndInput) + 1;
    std::wstring wbuf(len, L'\0');
    GetWindowTextW(hwndInput, &wbuf[0], len);
    if (!wbuf.empty() && wbuf.back() == L'\0') wbuf.pop_back();
    std::string code = ws2s(wbuf);

    auto tokens = tokenizeWithDFA(code, dfaId, dfaNum);

    std::ostringstream oss;
    for (auto &t : tokens)
        oss << t.type << ": " << t.text << "\r\n";

    SetWindowTextW(hwndTokens, s2ws(oss.str()).c_str());

    bool balanced = checkPDA(code);
    SetWindowTextW(hwndSyntax,
                   balanced ?
                   L"Syntax: PDA accepts — balanced delimiters ✅"
                   : L"Syntax: PDA rejects — unbalanced delimiters ❌");
}

//// Visualizer helpers ////
void DrawArrow(HDC hdc, int x1, int y1, int x2, int y2) {
    // simple arrow line with small arrowhead
    MoveToEx(hdc, x1, y1, NULL);
    LineTo(hdc, x2, y2);
    // arrowhead
    double angle = atan2((double)(y2 - y1), (double)(x2 - x1));
    const int ah = 8;
    POINT p1 = { x2 - (int)(ah * cos(angle - 0.5)), y2 - (int)(ah * sin(angle - 0.5)) };
    POINT p2 = { x2 - (int)(ah * cos(angle + 0.5)), y2 - (int)(ah * sin(angle + 0.5)) };
    MoveToEx(hdc, x2, y2, NULL);
    LineTo(hdc, p1.x, p1.y);
    MoveToEx(hdc, x2, y2, NULL);
    LineTo(hdc, p2.x, p2.y);
}

void drawDFAVisual(HDC hdc, const DFA &d, RECT rc) {
    int n = (int)d.rev.size();
    if (n == 0) {
        TextOutW(hdc, rc.left + 10, rc.top + 10, L"(No states)", 9);
        return;
    }
    // layout in circle
    int cx = (rc.left + rc.right) / 2;
    int cy = (rc.top + rc.bottom) / 2;
    int radius = std::min((rc.right-rc.left),(rc.bottom-rc.top)) / 2 - 40;
    if (radius < 20) radius = 20;
    std::vector<POINT> pts(n);
    for (int i = 0; i < n; ++i) {
        double ang = (2.0 * M_PI * i) / n - M_PI/2;
        pts[i].x = cx + (int)(radius * cos(ang));
        pts[i].y = cy + (int)(radius * sin(ang));
    }
    // draw transitions (only draw one label per printable char or aggregated)
    SetBkMode(hdc, TRANSPARENT);
    HPEN hPenOld = (HPEN)SelectObject(hdc, GetStockObject(BLACK_PEN));
    for (int i = 0; i < n; ++i) {
        for (auto &kv : d.trans[i]) {
            char c = kv.first;
            int j = kv.second;
            // draw line from pts[i] to pts[j]
            int x1 = pts[i].x;
            int y1 = pts[i].y;
            int x2 = pts[j].x;
            int y2 = pts[j].y;
            if (i == j) {
                // self-loop: draw small arc / circle above the node
                int rx = 16, ry = 12;
                Ellipse(hdc, x1 + rx - 8, y1 - ry - 8, x1 + rx + 8, y1 - ry + 8);
                // label
                if (isPrintable(c)) {
                    wchar_t buf[8]; swprintf_s(buf, L"%c", c);
                    TextOutW(hdc, x1 + rx + 10, y1 - ry - 6, buf, (int)wcslen(buf));
                } else {
                    TextOutW(hdc, x1 + rx + 10, y1 - ry - 6, L"\\x", 2);
                }
            } else {
                // offset endpoints a bit so circles don't overlap lines
                double dx = x2 - x1, dy = y2 - y1;
                double len = sqrt(dx*dx + dy*dy);
                if (len < 1) len = 1;
                double ux = dx / len, uy = dy / len;
                int r = 18; // radius of node circle
                int sx = (int)(x1 + ux * r);
                int sy = (int)(y1 + uy * r);
                int ex = (int)(x2 - ux * r);
                int ey = (int)(y2 - uy * r);
                DrawArrow(hdc, sx, sy, ex, ey);
                // label near midpoint
                int mx = (sx + ex) / 2 + (int)(-uy * 12);
                int my = (sy + ey) / 2 + (int)(ux * 12);
                if (isPrintable(c)) {
                    wchar_t buf[8]; swprintf_s(buf, L"%c", c);
                    TextOutW(hdc, mx, my, buf, (int)wcslen(buf));
                } else {
                    TextOutW(hdc, mx, my, L"\\x", 2);
                }
            }
        }
    }
    // draw nodes
    for (int i = 0; i < n; ++i) {
        int x = pts[i].x, y = pts[i].y;
        int r = 18;
        Ellipse(hdc, x - r, y - r, x + r, y + r);
        // fill with light color? skip to keep simple GDI
        // if accept state draw double circle
        if (d.accepts.count(i)) {
            int r2 = 22;
            Ellipse(hdc, x - r2, y - r2, x + r2, y + r2);
        }
        // draw state index
        wchar_t lbl[32];
        swprintf_s(lbl, L"%d", i);
        // center text
        SIZE sz; GetTextExtentPoint32W(hdc, lbl, (int)wcslen(lbl), &sz);
        TextOutW(hdc, x - sz.cx/2, y - sz.cy/2, lbl, (int)wcslen(lbl));
    }
    // start arrow
    int s = d.start;
    if (s >= 0 && s < n) {
        POINT p = pts[s];
        MoveToEx(hdc, p.x - 60, p.y, NULL);
        LineTo(hdc, p.x - 18, p.y);
        // small arrow
        DrawArrow(hdc, p.x - 60, p.y, p.x - 18, p.y);
        TextOutW(hdc, p.x - 120, p.y - 8, L"start", 5);
    }
    SelectObject(hdc, hPenOld);
}

LRESULT CALLBACK VisualProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, (HBRUSH)(COLOR_WINDOW+1));
        if (!g_haveDfas) {
            TextOutW(hdc, rc.left + 10, rc.top + 10, L"(DFAs not yet built — press Analyze)", 34);
        } else {
            if (g_visualChoice == 1) {
                drawDFAVisual(hdc, g_dfaId, rc);
            } else if (g_visualChoice == 2) {
                drawDFAVisual(hdc, g_dfaNum, rc);
            } else {
                TextOutW(hdc, rc.left + 10, rc.top + 10, L"(Select 'Show Identifier DFA' or 'Show Number DFA')", 50);
            }
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    default:
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
}

LRESULT CALLBACK WindowProcMain(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        NFA nid = buildIdentifierNFA();
        NFA nnum = buildNumberNFA();
        g_dfaId = subsetConstruction(nid);
        g_dfaNum = subsetConstruction(nnum);
        g_haveDfas = true;

        CreateWindowW(L"STATIC",
            L"Enter code (any text). DFAs handle lexing; PDA checks nesting.",
            WS_CHILD | WS_VISIBLE,
            20, 0, 700, 20, hwnd, NULL, NULL, NULL);

        hInput = CreateWindowW(L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
            20, 20, 700, 120, hwnd, NULL, NULL, NULL);

        CreateWindowW(L"BUTTON", L"Analyze",
            WS_CHILD | WS_VISIBLE,
            20, 150, 100, 30,
            hwnd, (HMENU)ID_BTN_ANALYZE, NULL, NULL);

        CreateWindowW(L"BUTTON", L"Show Identifier DFA",
            WS_CHILD | WS_VISIBLE,
            140, 150, 160, 30,
            hwnd, (HMENU)ID_BTN_SHOW_ID, NULL, NULL);

        CreateWindowW(L"BUTTON", L"Show Number DFA",
            WS_CHILD | WS_VISIBLE,
            320, 150, 140, 30,
            hwnd, (HMENU)ID_BTN_SHOW_NUM, NULL, NULL);

        hTokens = CreateWindowW(L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
            20, 190, 700, 180, hwnd, NULL, NULL, NULL);

        hSyntax = CreateWindowW(L"STATIC", L"",
            WS_CHILD | WS_VISIBLE,
            20, 380, 700, 30, hwnd, NULL, NULL, NULL);

        // Register visualizer window class
        WNDCLASSW wcv = {};
        wcv.lpfnWndProc = VisualProc;
        wcv.hInstance = (HINSTANCE)GetModuleHandle(NULL);
        wcv.lpszClassName = L"AutomatonVisualizerClass";
        wcv.hCursor = LoadCursor(NULL, IDC_ARROW);
        RegisterClassW(&wcv);

        // create visualizer child
        hVisual = CreateWindowExW(0, L"AutomatonVisualizerClass", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            20, 420, 700, 120,
            hwnd, NULL, NULL, NULL);

        // initial syntax text
        SetWindowTextW(hSyntax, L"Syntax: (press Analyze)");
        return 0;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == ID_BTN_ANALYZE) {
            // ensure DFAs up to date (rebuild so it uses latest logic if code changed)
            NFA nid = buildIdentifierNFA();
            NFA nnum = buildNumberNFA();
            g_dfaId = subsetConstruction(nid);
            g_dfaNum = subsetConstruction(nnum);
            g_haveDfas = true;

            AnalyzeCode(hInput, hTokens, hSyntax, g_dfaId, g_dfaNum);
            // after analyzing, trigger redraw of visualizer
            InvalidateRect(hVisual, NULL, TRUE);
        } else if (id == ID_BTN_SHOW_ID) {
            g_visualChoice = 1;
            InvalidateRect(hVisual, NULL, TRUE);
        } else if (id == ID_BTN_SHOW_NUM) {
            g_visualChoice = 2;
            InvalidateRect(hVisual, NULL, TRUE);
        }
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"AutomataSimulatorWindow";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProcMain;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME, L"Compiler Front-End Automata Simulator (Visualizer)",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT,
        760, 600,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}
