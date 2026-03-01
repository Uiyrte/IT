#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include "playfair.h"
#include "vigenere.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onPlayfairEncrypt();
    void onPlayfairDecrypt();
    void onPlayfairLoadFile();
    void onPlayfairSaveFile();
    void onPlayfairClear();
    
    void onVigenereEncrypt();
    void onVigenereDecrypt();
    void onVigenereLoadFile();
    void onVigenereSaveFile();
    void onVigenereClear();

private:
    QWidget* createPlayfairTab();
    QWidget* createVigenereTab();
    
    QTabWidget *tabWidget;
    
    QTextEdit *playfairInput;
    QTextEdit *playfairOutput;
    QLineEdit *playfairKey1;
    QLineEdit *playfairKey2;
    QPushButton *playfairEncryptBtn;
    QPushButton *playfairDecryptBtn;
    QPushButton *playfairLoadBtn;
    QPushButton *playfairSaveBtn;
    QPushButton *playfairClearBtn;
    
    QTextEdit *vigenereInput;
    QTextEdit *vigenereOutput;
    QLineEdit *vigenereKey;
    QPushButton *vigenereEncryptBtn;
    QPushButton *vigenereDecryptBtn;
    QPushButton *vigenereLoadBtn;
    QPushButton *vigenereSaveBtn;
    QPushButton *vigenereClearBtn;
    
    Playfair playfairCipher;
    Vigenere vigenereCipher;
    
    QString playfairLoadedFilePath;
    QString vigenereLoadedFilePath;
};

#endif 
