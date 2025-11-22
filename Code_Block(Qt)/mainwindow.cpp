#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QPainterPath>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QFontMetrics>

// --- AutomatonVisualizer Implementation ---

AutomatonVisualizer::AutomatonVisualizer(QWidget *parent)
    : QWidget(parent) {
    setMouseTracking(true);
    setMinimumSize(400, 300);
}

void AutomatonVisualizer::setDFA(const DFA* dfa) {
    m_dfa = dfa;
    resetTrace(); // Clear any existing trace when setting a new DFA
    update();
}

void AutomatonVisualizer::setScale(double scale) {
    m_scale = std::max(0.5, std::min(3.0, scale));
    update();
}

double AutomatonVisualizer::getScale() const {
    return m_scale;
}

void AutomatonVisualizer::computeNodePositionsAuto(const DFA &d, const QRectF &rc, QVector<QPointF>& pts, double scale) {
    int n = (int)d.rev.size();
    pts.resize(n);
    if (n == 0) return;

    qreal cx = (rc.left() + rc.right()) / 2.0;
    qreal cy = (rc.top() + rc.bottom()) / 2.0;
    qreal baseRad = std::min(rc.width(), rc.height()) / 2.0 - 80;
    if (baseRad < 40) baseRad = 40;
    qreal radius = std::max(20.0, baseRad * scale);

    for (int i = 0; i < n; ++i) {
        double ang = (2.0 * M_PI * i) / n - M_PI / 2;
        pts[i] = QPointF(cx + radius * cos(ang), cy + radius * sin(ang));
    }
}

QString AutomatonVisualizer::charsToLabel(const QSet<char>& s) const {
    if (s.isEmpty()) return QString();

    QVector<int> v, non;
    for (char c : s) {
        if (isPrintable(c)) {
            v.append((unsigned char)c);
        } else {
            non.append((unsigned char)c);
        }
    }
    std::sort(v.begin(), v.end());
    std::sort(non.begin(), non.end());

    QString out;
    for (int i = 0; i < v.size();) {
        int a = v[i], b = a;
        int j = i + 1;
        while (j < v.size() && v[j] == b + 1) { b = v[j]; ++j; }
        if (!out.isEmpty()) out += ',';
        if (a == b) out += QChar(a);
        else if (b == a + 1) { out += QChar(a); out += ','; out += QChar(b); }
        else { out += QChar(a); out += '-'; out += QChar(b); }
        i = j;
    }
    for (int c : non) {
        if (!out.isEmpty()) out += ',';
        out += QString("0x%1").arg(c, 2, 16, QChar('0')).toUpper();
    }
    if (out.size() > 80) out = out.left(77) + "...";
    return out;
}

void AutomatonVisualizer::drawDFAVisual(QPainter &painter, const DFA &d, const QRectF &rc, QVector<QPointF> &posStore) {
    int n = (int)d.rev.size();
    if (n == 0) {
        painter.drawText(QRectF(8, 8, width(), 20), "(No states)");
        return;
    }

    QVector<QPointF> pts;
    if (posStore.size() == n) pts = posStore;
    else {
        computeNodePositionsAuto(d, rc, pts, m_scale);
        posStore = pts;
    }

    // Aggregate labels
    QMap<QPair<int, int>, QSet<char>> emap;
    for (int i = 0; i < n; ++i) {
        for (const auto &kv : d.trans[i]) {
            emap[{i, kv.second}].insert(kv.first);
        }
    }

    painter.setRenderHint(QPainter::Antialiasing);

    // Draw edges first
    for (const auto &pr : emap.toStdMap()) {
        int i = pr.first.first, j = pr.first.second;
        if (i < 0 || i >= n || j < 0 || j >= n) continue;
        const QSet<char>& labset = pr.second;
        QPointF p1 = pts[i], p2 = pts[j];

        if (i == j) {
            qreal rx = 22 * m_scale, ry = 12 * m_scale;
            qreal dx = p1.x() - (rc.left() + rc.right()) / 2.0;
            qreal dy = p1.y() - (rc.top() + rc.bottom()) / 2.0;
            qreal L = sqrt(dx*dx + dy*dy) + 1.0;
            qreal ox = (dx / L) * (rx + 8);
            qreal oy = (dy / L) * (ry + 8);
            QRectF ellipseRect(p1.x() + ox - rx, p1.y() + oy - ry - 16, 2*rx, 2*ry);
            painter.drawEllipse(ellipseRect);
            QString label = charsToLabel(labset);
            painter.drawText(QRectF(p1.x() + ox + rx + 6, p1.y() + oy - ry - 16 + 2, 200, 20), label);
        } else {
            qreal mx = (p1.x() + p2.x()) / 2.0, my = (p1.y() + p2.y()) / 2.0;
            qreal dx = p2.x() - p1.x(), dy = p2.y() - p1.y();
            qreal len = sqrt(dx*dx + dy*dy);
            if (len < 1) len = 1;
            qreal ux = dx / len, uy = dy / len;
            qreal offset = 12 + (len / 10.0);
            qreal sign = ((i - j) % 2 == 0) ? 1.0 : -1.0;
            qreal cx = mx + sign * (-uy) * offset;
            qreal cy = my + sign * (ux) * offset;

            QPainterPath path;
            path.moveTo(p1);
            path.cubicTo(QPointF((p1.x()*2 + cx)/3, (p1.y()*2 + cy)/3),
                         QPointF((p2.x()*2 + cx)/3, (p2.y()*2 + cy)/3),
                         p2);
            painter.drawPath(path);

            // Arrowhead at 0.85 along the path
            QPointF ap = path.pointAtPercent(0.85);
            QPointF apPrev = path.pointAtPercent(0.83);
            painter.drawLine(apPrev, ap);
            double angle = atan2(ap.y() - apPrev.y(), ap.x() - apPrev.x());
            QPointF p1_arrow = ap + QPointF(cos(angle - 0.5) * 8, sin(angle - 0.5) * 8);
            QPointF p2_arrow = ap + QPointF(cos(angle + 0.5) * 8, sin(angle + 0.5) * 8);
            painter.drawLine(ap, p1_arrow);
            painter.drawLine(ap, p2_arrow);

            QPointF lp = path.pointAtPercent(0.5);
            QString label = charsToLabel(labset);
            painter.drawText(QRectF(lp.x() + 6, lp.y() - 8, 200, 20), label);
        }
    }

    // Draw nodes
    for (int i = 0; i < n; ++i) {
        QPointF p = pts[i];
        qreal r = std::max(10.0, 18 * m_scale);
        if (i == m_selectedState) {
            painter.fillRect(QRectF(p.x() - r - 4, p.y() - r - 4, 2*(r+4), 2*(r+4)), QColor(220, 235, 255));
        }
        painter.drawEllipse(p, r, r);
        if (d.accepts.count(i)) {
            painter.save();
            QPen pen(QColor(34,139,34), 2);
            painter.setPen(pen);
            painter.drawEllipse(p, r + 6, r + 6);
            painter.restore();
        }
        QString label = QString::number(i);
        QFontMetrics fm(painter.font());
        QRect br = fm.boundingRect(label);
        painter.drawText(QRectF(p.x() - br.width()/2, p.y() - br.height()/2, br.width(), br.height()), label);
    }

    // Start arrow
    int s = d.start;
    if (s >= 0 && s < n) {
        QPointF p = pts[s];
        qreal sx = p.x() - 60 * m_scale, sy = p.y();
        painter.drawLine(QPointF(sx, sy), QPointF(p.x() - 18 * m_scale, p.y()));
        double angle = atan2(p.y() - sy, p.x() - 18 * m_scale - sx);
        QPointF p1_arrow = QPointF(p.x() - 18 * m_scale, p.y()) + QPointF(cos(angle - 0.5) * 8, sin(angle - 0.5) * 8);
        QPointF p2_arrow = QPointF(p.x() - 18 * m_scale, p.y()) + QPointF(cos(angle + 0.5) * 8, sin(angle + 0.5) * 8);
        QPointF arrowTip = QPointF(p.x() - 18 * m_scale, p.y());
        painter.drawLine(arrowTip, p1_arrow);
        painter.drawLine(arrowTip, p2_arrow);
        painter.drawText(QRectF(p.x() - 120 * m_scale, p.y() - 8, 100, 20), "start");
    }

    posStore = pts;
}

void AutomatonVisualizer::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), palette().window());

    if (!m_dfa) {
        painter.drawText(QRect(12, 12, width(), 20), "(DFAs not yet built — press Analyze)");
    } else {
        QRectF rc(0, 0, width(), height());
        if (m_dfa) {
            drawDFAVisual(painter, *m_dfa, rc, m_nodePositions);
        } else {
            painter.drawText(QRect(12, 12, width(), 20), "(Select a DFA to visualize)");
        }

        // --- Draw the trace path if it exists ---
        if (!m_tracePath.isEmpty() && m_currentTraceStep >= 0) {
            painter.save();
            QPen tracePen(QColor(255, 165, 0), 3); // Orange pen for the trace
            painter.setPen(tracePen);

            // Highlight the current state in the trace
            if (m_currentTraceStep < m_tracePath.size()) {
                int currentState = m_tracePath[m_currentTraceStep];
                if (currentState >= 0 && currentState < m_nodePositions.size()) {
                    QPointF p = m_nodePositions[currentState];
                    qreal r = std::max(10.0, 18 * m_scale);
                    painter.setBrush(QColor(255, 255, 0, 100)); // Yellow translucent brush
                    painter.drawEllipse(p, r + 6, r + 6);
                    painter.setBrush(Qt::NoBrush);
                }
            }

            // Draw the path from start to current step
            for (int i = 0; i < m_currentTraceStep && i + 1 < m_tracePath.size(); ++i) {
                int from = m_tracePath[i];
                int to = m_tracePath[i + 1];
                if (from >= 0 && from < m_nodePositions.size() &&
                    to >= 0 && to < m_nodePositions.size()) {
                    QPointF p1 = m_nodePositions[from];
                    QPointF p2 = m_nodePositions[to];
                    painter.drawLine(p1, p2);
                }
            }

            // Draw an arrowhead for the last segment if there's a next step
            if (m_currentTraceStep > 0 && m_currentTraceStep < m_tracePath.size()) {
                int from = m_tracePath[m_currentTraceStep - 1];
                int to = m_tracePath[m_currentTraceStep];
                if (from >= 0 && from < m_nodePositions.size() &&
                    to >= 0 && to < m_nodePositions.size()) {
                    QPointF p1 = m_nodePositions[from];
                    QPointF p2 = m_nodePositions[to];
                    double angle = atan2(p2.y() - p1.y(), p2.x() - p1.x());
                    QPointF p1_arrow = p2 + QPointF(cos(angle - 0.5) * 8, sin(angle - 0.5) * 8);
                    QPointF p2_arrow = p2 + QPointF(cos(angle + 0.5) * 8, sin(angle + 0.5) * 8);
                    painter.drawLine(p2, p1_arrow);
                    painter.drawLine(p2, p2_arrow);
                }
            }

            // Display the current token text
            QString info = QString("Token: %1").arg(m_currentTokenText);
            QFontMetrics fm(painter.font());
            QRect br = fm.boundingRect(info);
            painter.drawText(QRectF(width() - br.width() - 10, 10, br.width(), br.height()), info);

            painter.restore();
        }
    }
}

void AutomatonVisualizer::mousePressEvent(QMouseEvent *event) {
    if (!m_dfa) return;

    QPointF mousePos = event->pos();
    int mx = mousePos.x(), my = mousePos.y();

    int found = -1;
    for (int i = 0; i < m_nodePositions.size(); ++i) {
        QPointF p = m_nodePositions[i];
        int dx = mx - p.x(), dy = my - p.y();
        qreal r = std::max(10.0, 18 * m_scale);
        if (dx*dx + dy*dy <= r*r) {
            found = i;
            m_dragging = true;
            m_dragIndex = i;
            m_dragOffset = QPointF(mx - p.x(), my - p.y());
            break;
        }
    }
    m_selectedState = found;

    // Emit signal for info panel (optional, if you add one back)
    QString info;
    if (found < 0) {
        info = "(no state selected)";
    } else {
        info = QString("State: %1\n").arg(found);
        if (found < m_dfa->rev.size()) {
            info += "NFA set: { ";
            bool first = true;
            for (int x : m_dfa->rev[found]) {
                if (!first) info += ", ";
                first = false;
                info += QString::number(x);
            }
            info += " }\n";
        }
        info += "Outgoing:\n";
        QMap<int, QSet<char>> out;
        if (found < m_dfa->trans.size()) {
            for (const auto &kv : m_dfa->trans[found]) {
                out[kv.second].insert(kv.first);
            }
        }
        for (const auto &kv : out.toStdMap()) {
            info += QString(" -> %1: %2\n").arg(kv.first).arg(charsToLabel(kv.second));
        }
    }
    emit stateSelected(found, info);

    update();
}

void AutomatonVisualizer::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragging && m_dragIndex >= 0 && m_dragIndex < m_nodePositions.size()) {
        QPointF mousePos = event->pos();
        QPointF &p = m_nodePositions[m_dragIndex];
        p.setX(mousePos.x() - m_dragOffset.x());
        p.setY(mousePos.y() - m_dragOffset.y());
        update();
    }
}

void AutomatonVisualizer::mouseReleaseEvent(QMouseEvent *event) {
    Q_UNUSED(event);
    if (m_dragging) {
        m_dragging = false;
        m_dragIndex = -1;
    }
}

void AutomatonVisualizer::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        double factor = (event->angleDelta().y() > 0) ? 1.1 : 0.9;
        setScale(m_scale * factor);
    } else {
        QWidget::wheelEvent(event); // Pass to parent for scrolling if needed
    }
}

void AutomatonVisualizer::zoomIn() {
    setScale(m_scale * 1.1);
}

void AutomatonVisualizer::zoomOut() {
    setScale(m_scale * 0.9);
}

// --- NEW SLOTS FOR TRACE VISUALIZATION ---

void AutomatonVisualizer::setTracePath(const QVector<int>& path, const QString& tokenText) {
    m_tracePath = path;
    m_currentTokenText = tokenText;
    m_currentTraceStep = 0; // Start at the beginning
    update();
}

void AutomatonVisualizer::resetTrace() {
    m_tracePath.clear();
    m_currentTokenText.clear();
    m_currentTraceStep = -1;
    update();
}

void AutomatonVisualizer::nextTraceStep() {
    if (m_tracePath.isEmpty() || m_currentTraceStep < 0) return;
    ++m_currentTraceStep;
    if (m_currentTraceStep >= m_tracePath.size()) {
        m_currentTraceStep = m_tracePath.size() - 1;
    }
    update();
}

void AutomatonVisualizer::prevTraceStep() {
    if (m_tracePath.isEmpty() || m_currentTraceStep < 0) return;
    --m_currentTraceStep;
    if (m_currentTraceStep < 0) {
        m_currentTraceStep = 0;
    }
    update();
}

// --- MainWindow Implementation ---

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setupUI();

    // Build initial DFAs
    NFA nid = buildIdentifierNFA_thompson();
    NFA nnum = buildNumberNFA_thompson();
    m_dfaId = subsetConstruction(nid);
    m_dfaNum = subsetConstruction(nnum);
    m_haveDfas = true;

    // Set the DFA for the visualizer to show the Identifier DFA by default
    m_visualizer->setDFA(&m_dfaId);
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI() {
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Main layout: Split into left and right panels
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    // --- LEFT PANEL ---
    QVBoxLayout *leftPanel = new QVBoxLayout();

    // User Input Label and Text Edit
    QLabel *userInputLabel = new QLabel("User Input", this);
    leftPanel->addWidget(userInputLabel);

    m_inputEdit = new QTextEdit(this);
    m_inputEdit->setPlaceholderText("Enter your code here...");
    m_inputEdit->setMinimumHeight(150);
    leftPanel->addWidget(m_inputEdit);

    // Run Button (aligned to the right of the User Input area)
    QHBoxLayout *runButtonLayout = new QHBoxLayout();
    runButtonLayout->addStretch(); // Pushes the button to the right
    QPushButton *runButton = new QPushButton("Run", this);
    runButtonLayout->addWidget(runButton);
    leftPanel->addLayout(runButtonLayout);

    // DFA Path Diagram Container Label and Widget
    QLabel *dfaLabel = new QLabel("DFA Path Diagram Container", this);
    leftPanel->addWidget(dfaLabel);

    m_visualizer = new AutomatonVisualizer(this);
    m_visualizer->setMinimumHeight(300); // Set a minimum height for the diagram
    leftPanel->addWidget(m_visualizer);

    mainLayout->addLayout(leftPanel);

    // --- RIGHT PANEL ---
    QVBoxLayout *rightPanel = new QVBoxLayout();

    // Tokenization Table Label and Widget
    QLabel *tokenTableLabel = new QLabel("Tokenization Table", this);
    rightPanel->addWidget(tokenTableLabel);

    m_tokensTable = new QTableWidget(this);
    m_tokensTable->setColumnCount(4);
    m_tokensTable->setHorizontalHeaderLabels({"Type", "Item", "Line", "Column"});
    m_tokensTable->horizontalHeader()->setStretchLastSection(true);
    m_tokensTable->verticalHeader()->setVisible(false); // Hide row numbers
    m_tokensTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tokensTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tokensTable->setAlternatingRowColors(true);
    m_tokensTable->setMinimumWidth(400);
    rightPanel->addWidget(m_tokensTable);

    // Trace Control Buttons
    QHBoxLayout *traceControlLayout = new QHBoxLayout();
    QPushButton *prevButton = new QPushButton("<< Prev", this);
    QPushButton *nextButton = new QPushButton("Next >>", this);
    traceControlLayout->addWidget(prevButton);
    traceControlLayout->addWidget(nextButton);
    rightPanel->addLayout(traceControlLayout);

    // Proceed to Parser Button (aligned to the bottom-right)
    QHBoxLayout *parserButtonLayout = new QHBoxLayout();
    parserButtonLayout->addStretch(); // Pushes the button to the right
    QPushButton *proceedButton = new QPushButton("Proceed to Parser", this);
    parserButtonLayout->addWidget(proceedButton);
    rightPanel->addLayout(parserButtonLayout);

    mainLayout->addLayout(rightPanel);

    // Connect signals
    connect(runButton, &QPushButton::clicked, this, &MainWindow::onAnalyzeClicked);
    connect(proceedButton, &QPushButton::clicked, this, &MainWindow::onProceedToParserClicked);
    connect(prevButton, &QPushButton::clicked, m_visualizer, &AutomatonVisualizer::prevTraceStep);
    connect(nextButton, &QPushButton::clicked, m_visualizer, &AutomatonVisualizer::nextTraceStep);

    // Set window title and size
    setWindowTitle("Compiler Front-End Automata Simulator (Qt)");
    resize(1000, 700); // Adjusted size to accommodate the new layout
}

void MainWindow::onAnalyzeClicked() {
    // Rebuild DFAs (keeps logic in sync)
    NFA nid = buildIdentifierNFA_thompson();
    NFA nnum = buildNumberNFA_thompson();
    m_dfaId = subsetConstruction(nid);
    m_dfaNum = subsetConstruction(nnum);
    m_haveDfas = true;

    analyzeCode();
    m_visualizer->update();
}

void MainWindow::onShowIdClicked() {
    m_visualChoice = 1;
    m_visualizer->setDFA(&m_dfaId);
    m_visualizer->update();
}

void MainWindow::onShowNumClicked() {
    m_visualChoice = 2;
    m_visualizer->setDFA(&m_dfaNum);
    m_visualizer->update();
}

void MainWindow::onTokenTableClicked() {
    showTokenTable();
}

void MainWindow::onStateSelected(int state, const QString& info) {
    Q_UNUSED(state);
    Q_UNUSED(info);
    // If you add an info panel back, you can use this slot.
}

void MainWindow::onProceedToParserClicked() {
    // For now, just show a message box.
    // In a real application, this would trigger the parsing phase.
    QMessageBox::information(this, "Parser", "Proceeding to Parser... (This is a placeholder.)");
}

void MainWindow::analyzeCode() {
    if (!m_haveDfas) return;

    std::string code = m_inputEdit->toPlainText().toStdString();
    if (code.empty()) {
        m_tokensTable->setRowCount(0);
        m_visualizer->resetTrace();
        return;
    }

    auto tokens = tokenizeWithDFA(code, m_dfaId, m_dfaNum);

    // Clear the existing table
    m_tokensTable->setRowCount(0);

    // Populate the table
    for (const auto &t : tokens) {
        int row = m_tokensTable->rowCount();
        m_tokensTable->insertRow(row);

        m_tokensTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(t.type)));
        m_tokensTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(t.text)));
        m_tokensTable->setItem(row, 2, new QTableWidgetItem(QString::number(t.line)));
        m_tokensTable->setItem(row, 3, new QTableWidgetItem(QString::number(t.col)));
    }

    // For simplicity, trace the path for the entire input string using the Identifier DFA.
    // In a real app, you would trace individual tokens.
    if (!tokens.empty()) {
        const auto& firstToken = tokens[0];
        QString qTokenText = QString::fromStdString(firstToken.text);

        // Trace the path for the entire input string (starting at position 0)
        auto [len, path] = dfaLongestMatchWithTrace(m_dfaId, code, 0);
        if (!path.isEmpty()) {
            m_visualizer->setTracePath(path, qTokenText);
        } else {
            // If the Identifier DFA doesn't match, try the Number DFA
            auto [lenNum, pathNum] = dfaLongestMatchWithTrace(m_dfaNum, code, 0);
            if (!pathNum.isEmpty()) {
                m_visualizer->setTracePath(pathNum, qTokenText);
            } else {
                m_visualizer->resetTrace();
            }
        }
    } else {
        m_visualizer->resetTrace();
    }
}

void MainWindow::showTokenTable() {
    // This function is now redundant as the table is populated in analyzeCode.
    // It's kept for potential future use or if you want to trigger the table update differently.
    analyzeCode();
}
