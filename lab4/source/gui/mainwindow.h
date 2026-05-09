#pragma once
#include <QMainWindow>
#include <cstdint>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
class QTextEdit;
class QLabel;
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onComputeG();
    void onComputeY();
    void onCalcHash();
    void onSignBrowse();
    void onSign();
    void onVerifyBrowse();
    void onVerify();

private:
    // ── Вкладка: Подпись ────────────────────────────────────────────
    QLineEdit   *m_qEdit, *m_pEdit, *m_hEdit, *m_xEdit, *m_kEdit;
    QLineEdit   *m_gEdit, *m_yEdit;   // readonly: вычисленные g и y
    QLineEdit   *m_h0Edit;            // начальное значение H₀
    QLabel      *m_signParamStatus;
    QLineEdit   *m_hashEdit;          // хеш-образ: вычисляется или вводится вручную
    QLineEdit   *m_signFileEdit;
    QPushButton *m_signBrowseBtn, *m_computeGBtn, *m_computeYBtn, *m_calcHashBtn, *m_signBtn;
    QTextEdit   *m_signOutput;

    // ── Вкладка: Проверка ────────────────────────────────────────────
    QLineEdit   *m_vqEdit, *m_vpEdit, *m_vhEdit, *m_vyEdit;
    QLineEdit   *m_vh0Edit;           // начальное значение H₀ для проверки
    QLineEdit   *m_verifyFileEdit;
    QPushButton *m_verifyBrowseBtn, *m_verifyBtn;
    QTextEdit   *m_verifyOutput;
    QLabel      *m_verifyStatus;

    QWidget *buildSignTab();
    QWidget *buildVerifyTab();
    void setStatus(QLabel *lbl, const QString &msg, bool ok);
};
