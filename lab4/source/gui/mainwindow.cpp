#include "mainwindow.h"
#include "dsa.h"

#include <QTabWidget>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QFile>
#include <QFileInfo>
#include <QByteArray>
#include <QIntValidator>
#include <QFont>
#include <QStatusBar>
#include <QScrollArea>

#include <vector>

// ─── Helpers ─────────────────────────────────────────────────────────────────

void MainWindow::setStatus(QLabel *lbl, const QString &msg, bool ok) {
    lbl->setText(msg);
    lbl->setStyleSheet(ok
        ? "color:#1b5e20; font-weight:bold; padding:2px 0;"
        : "color:#b71c1c; font-weight:bold; padding:2px 0;");
}

static std::vector<uint8_t> toVec(const QByteArray &ba) {
    return std::vector<uint8_t>(
        reinterpret_cast<const uint8_t*>(ba.constData()),
        reinterpret_cast<const uint8_t*>(ba.constData()) + ba.size());
}

static QString formatHashSteps(const std::vector<dsa::HashStep>& steps, int64_t q, int64_t h0) {
    QString out;
    out += QString("  H₀ = %1\n").arg(h0);
    if (steps.empty()) {
        out += QString("  Файл пустой — шагов нет.\n");
        out += QString("  H = H₀ mod q = %1 mod %2 = %3\n").arg(h0).arg(q).arg(h0 % q);
        return out;
    }
    const int MAX_SHOW = 50;
    int show = (int)std::min((int)steps.size(), MAX_SHOW);
    for (int i = 0; i < show; i++) {
        const auto& st = steps[i];
        out += QString("  H%1 = (H%2 + M%1)² mod q = (%3 + %4)² mod %5 = %6² mod %5 = %7\n")
                   .arg(st.i).arg(st.i - 1)
                   .arg(st.h_prev).arg(st.mi).arg(q)
                   .arg(st.sum).arg(st.h_next);
    }
    if ((int)steps.size() > MAX_SHOW)
        out += QString("  ... (%1 шагов всего, показаны первые %2)\n")
                   .arg(steps.size()).arg(MAX_SHOW);
    return out;
}

static QLineEdit *makeReadonly(const QString &ph = {}) {
    auto *e = new QLineEdit;
    e->setReadOnly(true);
    e->setPlaceholderText(ph);
    e->setStyleSheet("background:#f5f5f5; color:#0d47a1; font-weight:bold;");
    e->setFont(QFont("Courier", 10));
    return e;
}

static QPushButton *makeBtn(const QString &text, const QString &bg, const QString &hover) {
    auto *b = new QPushButton(text);
    b->setFixedHeight(32);
    b->setStyleSheet(QString(
        "QPushButton{background:%1;color:white;border-radius:4px;font-size:12px;padding:0 10px;}"
        "QPushButton:hover{background:%2;}"
        "QPushButton:pressed{background:#333;}").arg(bg, hover));
    return b;
}

// ─── Вкладка: Подпись ────────────────────────────────────────────────────────

QWidget *MainWindow::buildSignTab() {
    auto *scroll = new QScrollArea;
    auto *tab    = new QWidget;
    auto *outer  = new QVBoxLayout(tab);
    outer->setContentsMargins(10, 10, 10, 10);
    outer->setSpacing(8);

    // ── 1. Параметры q, p, h → g ────────────────────────────────────
    auto *gBox = new QGroupBox("Шаг 1: вычисление g  (нужны q, p, h)");
    auto *gg   = new QGridLayout(gBox);
    gg->setSpacing(6);

    auto inp = [&](const QString &lbl, QLineEdit *&e, const QString &ph, int row) {
        gg->addWidget(new QLabel(lbl), row, 0);
        e = new QLineEdit;
        e->setPlaceholderText(ph);
        e->setValidator(new QIntValidator(0, 2000000000, tab));
        gg->addWidget(e, row, 1);
    };

    inp("q  (простое):",              m_qEdit, "простое число",        0);
    inp("p  (простое, (p−1) % q=0):", m_pEdit, "p−1 кратно q",         1);
    inp("h  (1 < h < p−1):",          m_hEdit, "основание генератора", 2);

    m_computeGBtn = makeBtn("Вычислить g", "#1565c0", "#1976d2");
    gg->addWidget(m_computeGBtn, 3, 0, 1, 2);
    connect(m_computeGBtn, &QPushButton::clicked, this, &MainWindow::onComputeG);

    gg->addWidget(new QLabel("g = h^((p−1)/q) mod p:"), 4, 0);
    m_gEdit = makeReadonly("вычисляется автоматически");
    gg->addWidget(m_gEdit, 4, 1);

    // ── 2. x → y ────────────────────────────────────────────────────
    auto *yBox = new QGroupBox("Шаг 2: вычисление y  (нужны q, p, h, x)");
    auto *yg   = new QGridLayout(yBox);
    yg->setSpacing(6);

    yg->addWidget(new QLabel("x  (закрытый ключ, 0 < x < q):"), 0, 0);
    m_xEdit = new QLineEdit;
    m_xEdit->setPlaceholderText("закрытый ключ");
    m_xEdit->setValidator(new QIntValidator(1, 2000000000, tab));
    yg->addWidget(m_xEdit, 0, 1);

    m_computeYBtn = makeBtn("Вычислить y", "#6a1b9a", "#7b1fa2");
    yg->addWidget(m_computeYBtn, 1, 0, 1, 2);
    connect(m_computeYBtn, &QPushButton::clicked, this, &MainWindow::onComputeY);

    yg->addWidget(new QLabel("y = g^x mod p  (открытый ключ):"), 2, 0);
    m_yEdit = makeReadonly("вычисляется автоматически");
    yg->addWidget(m_yEdit, 2, 1);

    m_signParamStatus = new QLabel;
    m_signParamStatus->setWordWrap(true);

    // ── 3. k + файл → подпись ───────────────────────────────────────
    auto *signBox = new QGroupBox("Шаг 3: подпись файла  (нужны все параметры + k)");
    auto *sg      = new QGridLayout(signBox);
    sg->setSpacing(6);

    sg->addWidget(new QLabel("H₀ (начальное значение хеша):"), 0, 0);
    m_h0Edit = new QLineEdit;
    m_h0Edit->setPlaceholderText("по умолчанию 100");
    m_h0Edit->setText("100");
    m_h0Edit->setValidator(new QIntValidator(0, 2000000000, tab));
    sg->addWidget(m_h0Edit, 0, 1);

    sg->addWidget(new QLabel("k  (0 < k < q, случайный):"), 1, 0);
    m_kEdit = new QLineEdit;
    m_kEdit->setPlaceholderText("случайное число");
    m_kEdit->setValidator(new QIntValidator(1, 2000000000, tab));
    sg->addWidget(m_kEdit, 1, 1);

    sg->addWidget(new QLabel("Файл:"), 2, 0);
    auto *fileRow = new QHBoxLayout;
    m_signFileEdit = new QLineEdit;
    m_signFileEdit->setPlaceholderText("Путь к файлу...");
    m_signBrowseBtn = new QPushButton("Обзор...");
    m_signBrowseBtn->setFixedWidth(80);
    connect(m_signBrowseBtn, &QPushButton::clicked, this, &MainWindow::onSignBrowse);
    fileRow->addWidget(m_signFileEdit);
    fileRow->addWidget(m_signBrowseBtn);
    sg->addLayout(fileRow, 2, 1);

    m_signBtn = makeBtn("Подписать файл", "#0d47a1", "#1565c0");
    m_signBtn->setFixedHeight(36);
    sg->addWidget(m_signBtn, 3, 0, 1, 2);
    connect(m_signBtn, &QPushButton::clicked, this, &MainWindow::onSign);

    // ── Хеш-образ (отдельное поле) ──────────────────────────────────
    auto *hashBox = new QGroupBox("Хеш-образ сообщения  (десятичная система счисления)");
    auto *hg      = new QGridLayout(hashBox);
    hg->setSpacing(6);

    m_calcHashBtn = makeBtn("Предпросмотр хеша из файла", "#37474f", "#546e7a");
    hg->addWidget(m_calcHashBtn, 0, 0, 1, 2);
    connect(m_calcHashBtn, &QPushButton::clicked, this, &MainWindow::onCalcHash);

    hg->addWidget(new QLabel("H ="), 1, 0);
    m_hashEdit = makeReadonly("вычисляется при подписи");
    m_hashEdit->setStyleSheet("background:#fff8e1; color:#e65100; font-weight:bold; font-size:14px;");
    m_hashEdit->setFont(QFont("Courier", 13));
    hg->addWidget(m_hashEdit, 1, 1);

    // ── Результат ───────────────────────────────────────────────────
    auto *outBox = new QGroupBox("Вычисления и результат ЭЦП");
    auto *ol     = new QVBoxLayout(outBox);
    m_signOutput = new QTextEdit;
    m_signOutput->setReadOnly(true);
    m_signOutput->setFont(QFont("Courier", 10));
    m_signOutput->setMinimumHeight(180);
    m_signOutput->setPlaceholderText("Здесь появятся шаги вычисления...");
    ol->addWidget(m_signOutput);

    outer->addWidget(gBox);
    outer->addWidget(yBox);
    outer->addWidget(m_signParamStatus);
    outer->addWidget(signBox);
    outer->addWidget(hashBox);
    outer->addWidget(outBox, 1);

    scroll->setWidget(tab);
    scroll->setWidgetResizable(true);
    return scroll;
}

// ─── Вкладка: Проверка ───────────────────────────────────────────────────────

QWidget *MainWindow::buildVerifyTab() {
    auto *scroll = new QScrollArea;
    auto *tab    = new QWidget;
    auto *outer  = new QVBoxLayout(tab);
    outer->setContentsMargins(10, 10, 10, 10);
    outer->setSpacing(8);

    auto *paramsBox = new QGroupBox("Публичные параметры  (q, p, g, y)");
    auto *pg = new QGridLayout(paramsBox);
    pg->setSpacing(6);

    auto addField = [&](int row, const QString &lbl, QLineEdit *&edit, const QString &ph) {
        pg->addWidget(new QLabel(lbl), row, 0);
        edit = new QLineEdit;
        edit->setPlaceholderText(ph);
        edit->setValidator(new QIntValidator(0, 2000000000, tab));
        pg->addWidget(edit, row, 1);
    };

    addField(0, "q  (простое):",                m_vqEdit, "то же q, что при подписи");
    addField(1, "p  (простое):",                m_vpEdit, "то же p, что при подписи");
    addField(2, "h  (1 < h < p−1):",            m_vhEdit, "то же h, что при подписи");
    addField(3, "y  = g^x mod p (откр. ключ):", m_vyEdit, "открытый ключ");
    pg->addWidget(new QLabel("H₀ (начальное значение хеша):"), 4, 0);
    m_vh0Edit = new QLineEdit;
    m_vh0Edit->setPlaceholderText("то же H₀, что при подписи");
    m_vh0Edit->setText("100");
    m_vh0Edit->setValidator(new QIntValidator(0, 2000000000, tab));
    pg->addWidget(m_vh0Edit, 4, 1);

    auto *fileBox = new QGroupBox("Подписанный файл");
    auto *fl      = new QHBoxLayout(fileBox);
    m_verifyFileEdit = new QLineEdit;
    m_verifyFileEdit->setPlaceholderText("Путь к подписанному файлу...");
    m_verifyBrowseBtn = new QPushButton("Обзор...");
    m_verifyBrowseBtn->setFixedWidth(80);
    connect(m_verifyBrowseBtn, &QPushButton::clicked, this, &MainWindow::onVerifyBrowse);
    fl->addWidget(m_verifyFileEdit);
    fl->addWidget(m_verifyBrowseBtn);

    m_verifyBtn = makeBtn("Проверить ЭЦП", "#1b5e20", "#2e7d32");
    m_verifyBtn->setFixedHeight(36);
    connect(m_verifyBtn, &QPushButton::clicked, this, &MainWindow::onVerify);

    m_verifyStatus = new QLabel("—");
    m_verifyStatus->setAlignment(Qt::AlignCenter);
    m_verifyStatus->setFont(QFont("Arial", 15, QFont::Bold));
    m_verifyStatus->setMinimumHeight(48);
    m_verifyStatus->setStyleSheet(
        "color:#555; border:1px solid #ccc; border-radius:6px; padding:6px;");

    auto *outBox = new QGroupBox("Вычисленные при проверке значения");
    auto *ol     = new QVBoxLayout(outBox);
    m_verifyOutput = new QTextEdit;
    m_verifyOutput->setReadOnly(true);
    m_verifyOutput->setFont(QFont("Courier", 10));
    m_verifyOutput->setPlaceholderText("Здесь появятся вычисленные значения...");
    ol->addWidget(m_verifyOutput);

    outer->addWidget(paramsBox);
    outer->addWidget(fileBox);
    outer->addWidget(m_verifyBtn);
    outer->addWidget(m_verifyStatus);
    outer->addWidget(outBox, 1);

    scroll->setWidget(tab);
    scroll->setWidgetResizable(true);
    return scroll;
}

// ─── Конструктор ─────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("ЭЦП на основе DSA — Лабораторная работа 4");
    setMinimumSize(860, 720);

    auto *tabs = new QTabWidget;
    tabs->addTab(buildSignTab(),   "  Подпись файла  ");
    tabs->addTab(buildVerifyTab(), "  Проверка ЭЦП   ");
    setCentralWidget(tabs);

    statusBar()->showMessage("Шаг 1: введите q, p, h и нажмите «Вычислить g»");
}

// ─── Слоты: Подпись ──────────────────────────────────────────────────────────

void MainWindow::onComputeG() {
    bool qOk, pOk, hOk;
    int64_t q = m_qEdit->text().toLongLong(&qOk);
    int64_t p = m_pEdit->text().toLongLong(&pOk);
    int64_t h = m_hEdit->text().toLongLong(&hOk);

    m_gEdit->clear();

    if (!qOk || q < 2 || !dsa::is_prime(q)) {
        setStatus(m_signParamStatus,
            QString("Ошибка: q = %1 должно быть простым числом.").arg(q), false); return;
    }
    if (!pOk || !dsa::is_prime(p)) {
        setStatus(m_signParamStatus,
            QString("Ошибка: p = %1 не является простым числом.").arg(p), false); return;
    }
    if ((p - 1) % q != 0) {
        setStatus(m_signParamStatus,
            QString("Ошибка: (p−1) = %1 не делится на q = %2.").arg(p - 1).arg(q), false); return;
    }
    if (!hOk || h <= 1 || h >= p - 1) {
        setStatus(m_signParamStatus,
            QString("Ошибка: h должно быть строго в (1, %1).").arg(p - 1), false); return;
    }

    int64_t g = dsa::power_mod(h, (p - 1) / q, p);
    if (g == 1) {
        setStatus(m_signParamStatus,
            "Ошибка: g = 1. Выберите другое h.", false); return;
    }

    m_gEdit->setText(QString::number(g));
    setStatus(m_signParamStatus,
        QString("g = %1  (введите x и нажмите «Вычислить y»)").arg(g), true);
    statusBar()->showMessage("Шаг 2: введите x и нажмите «Вычислить y»");
}

void MainWindow::onComputeY() {
    bool xOk;
    int64_t x = m_xEdit->text().toLongLong(&xOk);

    m_yEdit->clear();

    bool qOk, pOk;
    int64_t q = m_qEdit->text().toLongLong(&qOk);
    int64_t p = m_pEdit->text().toLongLong(&pOk);

    if (!qOk || !pOk || m_gEdit->text().isEmpty()) {
        setStatus(m_signParamStatus,
            "Ошибка: сначала вычислите g (Шаг 1).", false); return;
    }
    bool gOk;
    int64_t g = m_gEdit->text().toLongLong(&gOk);
    if (!gOk) {
        setStatus(m_signParamStatus, "Ошибка: g не вычислено.", false); return;
    }
    if (!xOk || x <= 0 || x >= q) {
        setStatus(m_signParamStatus,
            QString("Ошибка: x должно быть строго в (0, %1).").arg(q), false); return;
    }

    int64_t y = dsa::power_mod(g, x, p);
    m_yEdit->setText(QString::number(y));
    setStatus(m_signParamStatus,
        QString("y = %1  (открытый ключ). Теперь введите k и выберите файл.").arg(y), true);
    statusBar()->showMessage("Шаг 3: введите k, выберите файл и нажмите «Подписать файл»");
}

void MainWindow::onSignBrowse() {
    QString path = QFileDialog::getOpenFileName(
        this, "Выберите файл для подписи", {}, "Все файлы (*)");
    if (!path.isEmpty()) m_signFileEdit->setText(path);
}

void MainWindow::onCalcHash() {
    bool qOk;
    int64_t q = m_qEdit->text().toLongLong(&qOk);
    if (!qOk || !dsa::is_prime(q)) {
        setStatus(m_signParamStatus, "Ошибка: введите корректное q.", false); return;
    }
    bool h0Ok;
    int64_t h0 = m_h0Edit->text().toLongLong(&h0Ok);
    if (!h0Ok || h0 < 0) {
        setStatus(m_signParamStatus, "Ошибка: введите корректное H₀.", false);
        return;
    }
    QString inPath = m_signFileEdit->text().trimmed();
    if (inPath.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Укажите файл для вычисления хеша."); return;
    }
    QFile fin(inPath);
    if (!fin.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось открыть файл:\n" + inPath); return;
    }
    QByteArray rawData = fin.readAll();
    fin.close();

    std::vector<dsa::HashStep> steps;
    int64_t H = dsa::hash_message(toVec(rawData), q, h0, &steps);
    m_hashEdit->setText(QString::number(H));

    QString out = "=== ХЕШИРОВАНИЕ (формула 3.2) ===\n";
    out += QString("  Hᵢ = (Hᵢ₋₁ + Mᵢ)² mod q,  q = %1\n\n").arg(q);
    out += formatHashSteps(steps, q, h0);
    out += QString("\n  Хеш-образ: H = %1\n").arg(H);
    m_signOutput->setPlainText(out);

    setStatus(m_signParamStatus,
        QString("Хеш вычислен: H = %1  (H₀ = %2, q = %3)").arg(H).arg(h0).arg(q), true);
    statusBar()->showMessage(QString("Хеш вычислен: H = %1").arg(H));
}

void MainWindow::onSign() {
    // ── Читаем и валидируем все входные параметры ─────────────────────
    bool qOk, pOk, hOk, xOk, kOk;
    int64_t q = m_qEdit->text().toLongLong(&qOk);
    int64_t p = m_pEdit->text().toLongLong(&pOk);
    int64_t h = m_hEdit->text().toLongLong(&hOk);
    int64_t x = m_xEdit->text().toLongLong(&xOk);
    int64_t k = m_kEdit->text().toLongLong(&kOk);

    if (!qOk || !dsa::is_prime(q)) {
        setStatus(m_signParamStatus,
            QString("Ошибка: q = %1 должно быть простым числом.").arg(q), false); return;
    }
    if (!pOk || !dsa::is_prime(p) || (p - 1) % q != 0) {
        setStatus(m_signParamStatus,
            "Ошибка: p должно быть простым и (p−1) кратно q.", false); return;
    }
    if (!hOk || h <= 1 || h >= p - 1) {
        setStatus(m_signParamStatus,
            QString("Ошибка: h должно быть строго в (1, %1).").arg(p - 1), false); return;
    }
    if (!xOk || x <= 0 || x >= q) {
        setStatus(m_signParamStatus,
            QString("Ошибка: x должно быть в (0, %1).").arg(q), false); return;
    }
    if (!kOk || k <= 0 || k >= q) {
        setStatus(m_signParamStatus,
            QString("Ошибка: k должно быть строго в (0, %1).").arg(q), false); return;
    }

    // ── Вычисляем g и y непосредственно из введённых параметров ──────
    int64_t g = dsa::power_mod(h, (p - 1) / q, p);
    if (g == 1) {
        setStatus(m_signParamStatus,
            "Ошибка: g = 1. Выберите другое h.", false); return;
    }
    int64_t y = dsa::power_mod(g, x, p);

    // Обновляем readonly-поля чтобы отображение было актуальным
    m_gEdit->setText(QString::number(g));
    m_yEdit->setText(QString::number(y));

    QString inPath = m_signFileEdit->text().trimmed();
    if (inPath.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Укажите файл для подписи."); return;
    }
    QFile fin(inPath);
    if (!fin.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось открыть файл:\n" + inPath); return;
    }
    QByteArray rawData = fin.readAll();
    fin.close();

    // ── Хеш: вычисляем автоматически из файла ────────────────────────
    bool h0Ok;
    int64_t h0 = m_h0Edit->text().toLongLong(&h0Ok);
    if (!h0Ok || h0 < 0) {
        setStatus(m_signParamStatus, "Ошибка: введите корректное H₀.", false); return;
    }
    std::vector<dsa::HashStep> hashSteps;
    int64_t H = dsa::hash_message(toVec(rawData), q, h0, &hashSteps);
    m_hashEdit->setText(QString::number(H));

    // ── r, s с повторным вводом k при r=0 или s=0 ────────────────────
    int64_t r, s, k_inv;
    while (true) {
        k_inv        = dsa::mod_inverse(k, q);
        r            = dsa::power_mod(g, k, p) % q;
        int64_t xr   = static_cast<int64_t>((__int128)x * r % q);
        int64_t h_xr = (H + xr) % q;
        s            = static_cast<int64_t>((__int128)k_inv * h_xr % q);

        if (r != 0 && s != 0) break;

        bool ok = false;
        int64_t newK = QInputDialog::getInt(
            this, "Повторный ввод k",
            QString("r = %1, s = %2 — одно из значений равно 0!\n"
                    "Введите другое k (0 < k < %3):").arg(r).arg(s).arg(q),
            static_cast<int>(k % (q - 1) + 1), 1, static_cast<int>(q - 1), 1, &ok);
        if (!ok) {
            setStatus(m_signParamStatus, "Подпись отменена.", false); return;
        }
        k = newK;
        m_kEdit->setText(QString::number(k));
    }

    // ── Вывод вычислений ──────────────────────────────────────────────
    QString out;
    out += "=== ПАРАМЕТРЫ DSA ===\n";
    out += QString("  q = %1\n  p = %2\n  h = %3\n").arg(q).arg(p).arg(h);
    out += QString("  g = h^((p−1)/q) mod p = %1\n").arg(g);
    out += QString("  x = %1  (закрытый ключ)\n").arg(x);
    out += QString("  y = g^x mod p = %1  (открытый ключ)\n").arg(y);
    out += QString("  k = %1\n\n").arg(k);
    out += "=== ХЕШИРОВАНИЕ (формула 3.2) ===\n";
    out += QString("  Hᵢ = (Hᵢ₋₁ + Mᵢ)² mod q,  q = %1\n\n").arg(q);
    out += formatHashSteps(hashSteps, q, h0);
    out += QString("\n  Используемый хеш-образ: H = %1\n\n").arg(H);
    out += "=== ВЫЧИСЛЕНИЕ ЭЦП ===\n";
    out += QString("  k⁻¹ mod q = k^(q−2) mod q = %1^%2 mod %3 = %4\n")
               .arg(k).arg(q - 2).arg(q).arg(k_inv);
    out += QString("  r = (g^k mod p) mod q = (%1^%2 mod %3) mod %4 = %5\n")
               .arg(g).arg(k).arg(p).arg(q).arg(r);
    out += QString("  s = k⁻¹·(H + x·r) mod q = %1·(%2 + %3·%4) mod %5 = %6\n\n")
               .arg(k_inv).arg(H).arg(x).arg(r).arg(q).arg(s);
    out += "=== ЦИФРОВАЯ ПОДПИСЬ (ЭЦП) ===\n";
    out += QString("  r = %1\n  s = %2\n").arg(r).arg(s);
    m_signOutput->setPlainText(out);
    setStatus(m_signParamStatus, "ЭЦП вычислена успешно.", true);

    // ── Запись: исходное содержимое + r и s ──────────────────────────
    QFileInfo fi(inPath);
    QString outPath = fi.dir().filePath(
        fi.baseName() + "_signed" +
        (fi.suffix().isEmpty() ? "" : "." + fi.suffix()));

    QFile fout(outPath);
    if (!fout.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось создать файл:\n" + outPath); return;
    }
    fout.write(rawData);
    fout.write(QString("\n%1\n%2").arg(r).arg(s).toUtf8());
    fout.close();

    statusBar()->showMessage("Файл подписан: " + outPath);
    QMessageBox::information(this, "Готово",
        QString("Файл подписан!\n\nХеш-образ:  H = %1\n\nЭЦП:\n  r = %2\n  s = %3\n\n"
                "Подписанный файл:\n%4").arg(H).arg(r).arg(s).arg(outPath));
}

// ─── Слоты: Проверка ─────────────────────────────────────────────────────────

void MainWindow::onVerifyBrowse() {
    QString path = QFileDialog::getOpenFileName(
        this, "Выберите подписанный файл", {}, "Все файлы (*)");
    if (!path.isEmpty()) m_verifyFileEdit->setText(path);
}

void MainWindow::onVerify() {
    bool qOk, pOk, hOk, yOk;
    int64_t q = m_vqEdit->text().toLongLong(&qOk);
    int64_t p = m_vpEdit->text().toLongLong(&pOk);
    int64_t h = m_vhEdit->text().toLongLong(&hOk);
    int64_t y = m_vyEdit->text().toLongLong(&yOk);

    if (!qOk || !dsa::is_prime(q)) {
        QMessageBox::warning(this, "Ошибка", "Введите корректное q."); return;
    }
    if (!pOk || !dsa::is_prime(p) || (p - 1) % q != 0) {
        QMessageBox::warning(this, "Ошибка", "Введите корректное p."); return;
    }
    if (!hOk || h <= 1 || h >= p - 1) {
        QMessageBox::warning(this, "Ошибка", "Введите корректное h."); return;
    }
    if (!yOk || y <= 0) {
        QMessageBox::warning(this, "Ошибка", "Введите корректное y."); return;
    }
    int64_t g = dsa::power_mod(h, (p - 1) / q, p);

    QString path = m_verifyFileEdit->text().trimmed();
    if (path.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Укажите файл для проверки."); return;
    }
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось открыть файл:\n" + path); return;
    }
    QByteArray fileData = f.readAll();
    f.close();

    // Формат подписи: <содержимое>\n<r>\n<s>
    // Ищем точные позиции двух последних \n
    int lastNl = fileData.lastIndexOf('\n');
    int secNl   = lastNl > 0 ? fileData.lastIndexOf('\n', lastNl - 1) : -1;

    if (lastNl < 0 || secNl < 0) {
        QMessageBox::warning(this, "Файл изменён",
            "Подпись не обнаружена: файл был изменён или не подписан данной программой."); return;
    }

    QByteArray r_bytes = fileData.mid(secNl + 1, lastNl - secNl - 1);
    QByteArray s_bytes = fileData.mid(lastNl + 1);
    QByteArray message = fileData.left(secNl);

    bool rOk, sOk;
    int64_t r = r_bytes.toLongLong(&rOk);
    int64_t s = s_bytes.toLongLong(&sOk);
    if (!rOk || !sOk || r <= 0 || s <= 0) {
        QMessageBox::warning(this, "Файл изменён",
            QString("Подпись повреждена: ожидались числа r и s, найдено «%1» и «%2».\n"
                    "Файл был изменён после подписания.")
                .arg(QString(r_bytes)).arg(QString(s_bytes))); return;
    }

    bool vh0Ok;
    int64_t vh0 = m_vh0Edit->text().toLongLong(&vh0Ok);
    if (!vh0Ok || vh0 < 0) {
        QMessageBox::warning(this, "Ошибка", "Введите корректное начальное значение H₀."); return;
    }
    int64_t H = dsa::hash_message(toVec(message), q, vh0);
    int64_t s_inv = dsa::mod_inverse(s, q);
    int64_t u1    = static_cast<int64_t>((__int128)H * s_inv % q);
    int64_t u2    = static_cast<int64_t>((__int128)r * s_inv % q);
    int64_t gu1   = dsa::power_mod(g, u1, p);
    int64_t yu2   = dsa::power_mod(y, u2, p);
    int64_t v     = static_cast<int64_t>((__int128)gu1 * yu2 % p) % q;
    bool valid    = (v == r);

    QString out;
    out += "=== ПАРАМЕТРЫ ОТКРЫТОГО КЛЮЧА ===\n";
    out += QString("  q = %1\n  p = %2\n  h = %3\n").arg(q).arg(p).arg(h);
    out += QString("  g = h^((p−1)/q) mod p = %1\n").arg(g);
    out += QString("  y = %1\n\n").arg(y);
    out += "=== ИЗВЛЕЧЁННАЯ ПОДПИСЬ (из файла) ===\n";
    out += QString("  r = %1\n  s = %2\n\n").arg(r).arg(s);
    out += "=== ХЕШ СООБЩЕНИЯ ===\n";
    out += QString("  H₀ = %1,  Hᵢ = (Hᵢ₋₁ + Mᵢ)² mod q\n").arg(vh0);
    out += QString("  H = %1  (десятичная)\n\n").arg(H);
    out += "=== ВЫЧИСЛЕНИЯ ПРИ ПРОВЕРКЕ ===\n";
    out += QString("  s⁻¹ mod q = s^(q−2) mod q = %1^%2 mod %3 = %4\n")
               .arg(s).arg(q - 2).arg(q).arg(s_inv);
    out += QString("  u1 = H·s⁻¹ mod q = %1·%2 mod %3 = %4\n")
               .arg(H).arg(s_inv).arg(q).arg(u1);
    out += QString("  u2 = r·s⁻¹ mod q = %1·%2 mod %3 = %4\n")
               .arg(r).arg(s_inv).arg(q).arg(u2);
    out += QString("  g^u1 mod p = %1^%2 mod %3 = %4\n").arg(g).arg(u1).arg(p).arg(gu1);
    out += QString("  y^u2 mod p = %1^%2 mod %3 = %4\n").arg(y).arg(u2).arg(p).arg(yu2);
    out += QString("  v = (g^u1·y^u2 mod p) mod q = (%1·%2 mod %3) mod %4 = %5\n\n")
               .arg(gu1).arg(yu2).arg(p).arg(q).arg(v);
    out += "=== РЕЗУЛЬТАТ ===\n";
    if (valid) {
        out += QString("  v = r = %1  →  ПОДПИСЬ ВЕРНА\n").arg(v);
        out += "  Хеш сообщения совпадает — файл не изменён.\n";
    } else {
        out += QString("  v = %1  ≠  r = %2  →  ПОДПИСЬ НЕВЕРНА\n").arg(v).arg(r);
        out += "  Причина: вычисленное v не равно r из подписи.\n";
        out += "  Файл был изменён после подписания.\n";
    }
    m_verifyOutput->setPlainText(out);

    if (valid) {
        m_verifyStatus->setText("✔  ПОДПИСЬ ВЕРНА");
        m_verifyStatus->setStyleSheet(
            "color:#1b5e20; font-weight:bold; font-size:16px;"
            " border:2px solid #1b5e20; border-radius:6px; padding:6px;");
    } else {
        m_verifyStatus->setText("✘  ПОДПИСЬ НЕВЕРНА");
        m_verifyStatus->setStyleSheet(
            "color:#b71c1c; font-weight:bold; font-size:16px;"
            " border:2px solid #b71c1c; border-radius:6px; padding:6px;");
    }
    statusBar()->showMessage(valid ? "Подпись верна." : "Подпись неверна.");
}
