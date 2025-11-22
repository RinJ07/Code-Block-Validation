#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPoint>
#include <QPointF>
#include <QMap>
#include <QSet>
#include <QVector>
#include <QPair>
#include <QMessageBox>
#include <QDebug>
#include <sstream>
#include <algorithm>
#include <cmath>
#include "dfa.h"
#include "tokenizer.h"
#include "pda.h"

class AutomatonVisualizer : public QWidget {
    Q_OBJECT

public:
    explicit AutomatonVisualizer(QWidget *parent = nullptr);

    void setDFA(const DFA* dfa);
    void setScale(double scale);
    double getScale() const;

signals:
    void stateSelected(int state, const QString& info);

public slots:
    void zoomIn();
    void zoomOut();
    void setTracePath(const QVector<int>& path, const QString& tokenText); // NEW
    void resetTrace(); // NEW
    void nextTraceStep(); // NEW
    void prevTraceStep(); // NEW

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    const DFA* m_dfa = nullptr;
    QVector<QPointF> m_nodePositions;
    int m_selectedState = -1;
    bool m_dragging = false;
    int m_dragIndex = -1;
    QPointF m_dragOffset;
    double m_scale = 1.0;

    // NEW MEMBERS FOR TRACE VISUALIZATION
    QVector<int> m_tracePath;
    int m_currentTraceStep = -1;
    QString m_currentTokenText;

    void computeNodePositionsAuto(const DFA &d, const QRectF &rc, QVector<QPointF>& pts, double scale);
    void drawDFAVisual(QPainter &painter, const DFA &d, const QRectF &rc, QVector<QPointF> &posStore);
    QString charsToLabel(const QSet<char>& s) const;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAnalyzeClicked();
    void onShowIdClicked();
    void onShowNumClicked();
    void onTokenTableClicked();
    void onStateSelected(int state, const QString& info);
    void onProceedToParserClicked();

private:
    void setupUI();
    void analyzeCode();
    void showTokenTable();

    QTextEdit *m_inputEdit;
    QTableWidget *m_tokensTable;
    QLabel *m_syntaxLabel;
    AutomatonVisualizer *m_visualizer;
    QTextEdit *m_infoEdit;

    DFA m_dfaId;
    DFA m_dfaNum;
    bool m_haveDfas = false;
    int m_visualChoice = 0; // 0 none, 1 id, 2 num
};

#endif // MAINWINDOW_H
