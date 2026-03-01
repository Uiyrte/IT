#include "mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QSplitter>
#include <QRegularExpressionValidator>

namespace {
struct PreparedPlayfairKey {
    QString normalized;
    bool hadDuplicates;
};

PreparedPlayfairKey preparePlayfairKey(const QString& rawKey) {
    QString filtered;
    filtered.reserve(rawKey.size());

    for (QChar ch : rawKey) {
        QChar upper = ch.toUpper();
        if (upper >= QChar('A') && upper <= QChar('Z')) {
            if (upper == QChar('J')) {
                upper = QChar('I');
            }
            filtered += upper;
        }
    }

    QString normalized;
    normalized.reserve(filtered.size());
    bool seen[26] = {false};
    bool hadDuplicates = false;

    for (QChar ch : filtered) {
        int index = ch.unicode() - 'A';
        if (seen[index]) {
            hadDuplicates = true;
            continue;
        }
        seen[index] = true;
        normalized += ch;
    }

    return {normalized, hadDuplicates};
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Приложение для шифрования - Плейфера и Виженера");
    setMinimumSize(900, 700);
    
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    tabWidget = new QTabWidget(this);
    tabWidget->addTab(createPlayfairTab(), "Шифр Плейфера 4 таблицы (Английский)");
    tabWidget->addTab(createVigenereTab(), "Шифр Виженера (Русский)");
    
    mainLayout->addWidget(tabWidget);
}

MainWindow::~MainWindow() {}

QWidget* MainWindow::createPlayfairTab() {
    QWidget *tab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(tab);
    
    QGroupBox *keyGroup = new QGroupBox("Ключи шифрования (для таблиц 3 и 4)");
    QVBoxLayout *keysLayout = new QVBoxLayout();

    QHBoxLayout *key1Layout = new QHBoxLayout();
    QLabel *key1Label = new QLabel("Ключ 1:");
    playfairKey1 = new QLineEdit();
    playfairKey1->setPlaceholderText("Введите первый ключ (только английские буквы)");
    playfairKey1->setContextMenuPolicy(Qt::DefaultContextMenu);
    QRegularExpressionValidator *englishValidator1 = new QRegularExpressionValidator(QRegularExpression("[A-Za-z]*"), playfairKey1);
    playfairKey1->setValidator(englishValidator1);
    key1Layout->addWidget(key1Label);
    key1Layout->addWidget(playfairKey1);
    keysLayout->addLayout(key1Layout);

    QHBoxLayout *key2Layout = new QHBoxLayout();
    QLabel *key2Label = new QLabel("Ключ 2:");
    playfairKey2 = new QLineEdit();
    playfairKey2->setPlaceholderText("Введите второй ключ (только английские буквы)");
    playfairKey2->setContextMenuPolicy(Qt::DefaultContextMenu);
    QRegularExpressionValidator *englishValidator2 = new QRegularExpressionValidator(QRegularExpression("[A-Za-z]*"), playfairKey2);
    playfairKey2->setValidator(englishValidator2);
    key2Layout->addWidget(key2Label);
    key2Layout->addWidget(playfairKey2);
    keysLayout->addLayout(key2Layout);

    keyGroup->setLayout(keysLayout);
    layout->addWidget(keyGroup);
    
    QSplitter *splitter = new QSplitter(Qt::Vertical);
    
    QGroupBox *inputGroup = new QGroupBox("Входной текст");
    QVBoxLayout *inputLayout = new QVBoxLayout();
    playfairInput = new QTextEdit();
    playfairInput->setPlaceholderText("Введите текст для шифрования/дешифрования или загрузите из файла...\n(Вставить: Cmd+V)");
    playfairInput->setContextMenuPolicy(Qt::DefaultContextMenu);
    inputLayout->addWidget(playfairInput);
    inputGroup->setLayout(inputLayout);
    splitter->addWidget(inputGroup);
    
    QGroupBox *outputGroup = new QGroupBox("Выходной текст");
    QVBoxLayout *outputLayout = new QVBoxLayout();
    playfairOutput = new QTextEdit();
    playfairOutput->setPlaceholderText("Зашифрованный/расшифрованный текст появится здесь...");
    playfairOutput->setReadOnly(true);
    playfairOutput->setContextMenuPolicy(Qt::DefaultContextMenu);
    outputLayout->addWidget(playfairOutput);
    outputGroup->setLayout(outputLayout);
    splitter->addWidget(outputGroup);
    
    layout->addWidget(splitter);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    playfairEncryptBtn = new QPushButton("Зашифровать");
    playfairDecryptBtn = new QPushButton("Расшифровать");
    playfairLoadBtn = new QPushButton("Загрузить");
    playfairSaveBtn = new QPushButton("Сохранить");
    playfairClearBtn = new QPushButton("Очистить");
    
    
    buttonLayout->addWidget(playfairEncryptBtn);
    buttonLayout->addWidget(playfairDecryptBtn);
    buttonLayout->addWidget(playfairLoadBtn);
    buttonLayout->addWidget(playfairSaveBtn);
    buttonLayout->addWidget(playfairClearBtn);
    
    layout->addLayout(buttonLayout);
    
    connect(playfairEncryptBtn, &QPushButton::clicked, this, &MainWindow::onPlayfairEncrypt);
    connect(playfairDecryptBtn, &QPushButton::clicked, this, &MainWindow::onPlayfairDecrypt);
    connect(playfairLoadBtn, &QPushButton::clicked, this, &MainWindow::onPlayfairLoadFile);
    connect(playfairSaveBtn, &QPushButton::clicked, this, &MainWindow::onPlayfairSaveFile);
    connect(playfairClearBtn, &QPushButton::clicked, this, &MainWindow::onPlayfairClear);
    
    return tab;
}

QWidget* MainWindow::createVigenereTab() {
    QWidget *tab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(tab);
    
    QGroupBox *keyGroup = new QGroupBox("Ключ шифрования");
    QHBoxLayout *keyLayout = new QHBoxLayout();
    QLabel *keyLabel = new QLabel("Ключ:");
    vigenereKey = new QLineEdit();
    vigenereKey->setPlaceholderText("Введите ключ (только русские буквы)");
    vigenereKey->setContextMenuPolicy(Qt::DefaultContextMenu);
    keyLayout->addWidget(keyLabel);
    keyLayout->addWidget(vigenereKey);
    keyGroup->setLayout(keyLayout);
    layout->addWidget(keyGroup);
    
    QSplitter *splitter = new QSplitter(Qt::Vertical);
    
    QGroupBox *inputGroup = new QGroupBox("Входной текст");
    QVBoxLayout *inputLayout = new QVBoxLayout();
    vigenereInput = new QTextEdit();
    vigenereInput->setPlaceholderText("Введите текст для шифрования/дешифрования или загрузите из файла...\n(Вставить: Cmd+V)");
    vigenereInput->setContextMenuPolicy(Qt::DefaultContextMenu);
    inputLayout->addWidget(vigenereInput);
    inputGroup->setLayout(inputLayout);
    splitter->addWidget(inputGroup);
    
    QGroupBox *outputGroup = new QGroupBox("Выходной текст");
    QVBoxLayout *outputLayout = new QVBoxLayout();
    vigenereOutput = new QTextEdit();
    vigenereOutput->setPlaceholderText("Зашифрованный/расшифрованный текст появится здесь...");
    vigenereOutput->setReadOnly(true);
    vigenereOutput->setContextMenuPolicy(Qt::DefaultContextMenu);
    outputLayout->addWidget(vigenereOutput);
    outputGroup->setLayout(outputLayout);
    splitter->addWidget(outputGroup);
    
    layout->addWidget(splitter);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    vigenereEncryptBtn = new QPushButton("Зашифровать");
    vigenereDecryptBtn = new QPushButton("Расшифровать");
    vigenereLoadBtn = new QPushButton("Загрузить");
    vigenereSaveBtn = new QPushButton("Сохранить");
    vigenereClearBtn = new QPushButton("Очистить");
    
    
    buttonLayout->addWidget(vigenereEncryptBtn);
    buttonLayout->addWidget(vigenereDecryptBtn);
    buttonLayout->addWidget(vigenereLoadBtn);
    buttonLayout->addWidget(vigenereSaveBtn);
    buttonLayout->addWidget(vigenereClearBtn);
    
    layout->addLayout(buttonLayout);
    
    connect(vigenereEncryptBtn, &QPushButton::clicked, this, &MainWindow::onVigenereEncrypt);
    connect(vigenereDecryptBtn, &QPushButton::clicked, this, &MainWindow::onVigenereDecrypt);
    connect(vigenereLoadBtn, &QPushButton::clicked, this, &MainWindow::onVigenereLoadFile);
    connect(vigenereSaveBtn, &QPushButton::clicked, this, &MainWindow::onVigenereSaveFile);
    connect(vigenereClearBtn, &QPushButton::clicked, this, &MainWindow::onVigenereClear);
    
    return tab;
}

void MainWindow::onPlayfairEncrypt() {
    PreparedPlayfairKey preparedKey1 = preparePlayfairKey(playfairKey1->text());
    PreparedPlayfairKey preparedKey2 = preparePlayfairKey(playfairKey2->text());

    playfairKey1->setText(preparedKey1.normalized);
    playfairKey2->setText(preparedKey2.normalized);

    QString key1 = preparedKey1.normalized;
    QString key2 = preparedKey2.normalized;
    QString input = playfairInput->toPlainText();
    
    if (key1.isEmpty() || key2.isEmpty()) {
        QMessageBox::warning(this, "Внимание", "Пожалуйста, введите оба ключа!");
        return;
    }
    
    if (input.isEmpty()) {
        QMessageBox::warning(this, "Внимание", "Пожалуйста, введите текст для шифрования!");
        return;
    }

    if (preparedKey1.hadDuplicates || preparedKey2.hadDuplicates) {
        QMessageBox::information(this, "Уведомление о ключе Playfair",
            "Повторяющиеся буквы удалены из ключа(ей) Playfair.");
    }
    
    std::string result = playfairCipher.encrypt(input.toStdString(), key1.toStdString(), key2.toStdString());
    playfairOutput->setPlainText(QString::fromStdString(result));
    
    if (!playfairLoadedFilePath.isEmpty()) {
        QFileInfo fileInfo(playfairLoadedFilePath);
        QString baseName = fileInfo.completeBaseName();
        QString dirPath = fileInfo.absolutePath();
        QString outputFileName = dirPath + "/" + baseName + "_enc.txt";
        
        QFile outputFile(outputFileName);
        if (outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&outputFile);
            out << QString::fromStdString(result);
            outputFile.close();
            QMessageBox::information(this, "Успешно", "Зашифрованный файл сохранен как:\n" + outputFileName);
        } else {
            QMessageBox::critical(this, "Ошибка", "Не удалось сохранить файл: " + outputFileName);
        }
    }
}

void MainWindow::onPlayfairDecrypt() {
    PreparedPlayfairKey preparedKey1 = preparePlayfairKey(playfairKey1->text());
    PreparedPlayfairKey preparedKey2 = preparePlayfairKey(playfairKey2->text());

    playfairKey1->setText(preparedKey1.normalized);
    playfairKey2->setText(preparedKey2.normalized);

    QString key1 = preparedKey1.normalized;
    QString key2 = preparedKey2.normalized;
    QString input = playfairInput->toPlainText();
    
    if (key1.isEmpty() || key2.isEmpty()) {
        QMessageBox::warning(this, "Внимание", "Пожалуйста, введите оба ключа!");
        return;
    }
    
    if (input.isEmpty()) {
        QMessageBox::warning(this, "Внимание", "Пожалуйста, введите текст для дешифрования!");
        return;
    }

    if (preparedKey1.hadDuplicates || preparedKey2.hadDuplicates) {
        QMessageBox::information(this, "Уведомление о ключе Playfair",
            "Повторяющиеся буквы удалены из ключа(ей) Playfair.");
    }
    
    std::string result = playfairCipher.decrypt(input.toStdString(), key1.toStdString(), key2.toStdString());
    playfairOutput->setPlainText(QString::fromStdString(result));
    
    if (!playfairLoadedFilePath.isEmpty()) {
        QFileInfo fileInfo(playfairLoadedFilePath);
        QString baseName = fileInfo.completeBaseName();
        QString dirPath = fileInfo.absolutePath();
        QString outputFileName = dirPath + "/" + baseName + "_dec.txt";
        
        QFile outputFile(outputFileName);
        if (outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&outputFile);
            out << QString::fromStdString(result);
            outputFile.close();
            QMessageBox::information(this, "Успешно", "Расшифрованный файл сохранен как:\n" + outputFileName);
        } else {
            QMessageBox::critical(this, "Ошибка", "Не удалось сохранить файл: " + outputFileName);
        }
    }
}

void MainWindow::onPlayfairLoadFile() {
    QString fileName = QFileDialog::getOpenFileName(this, 
        "Открыть текстовый файл", "", "Текстовые файлы (*.txt);;Все файлы (*)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось открыть файл: " + fileName);
        return;
    }
    
    QTextStream in(&file);
    playfairInput->setPlainText(in.readAll());
    file.close();
    
    playfairLoadedFilePath = fileName;
}

void MainWindow::onPlayfairSaveFile() {
    QString output = playfairOutput->toPlainText();
    
    if (output.isEmpty()) {
        QMessageBox::warning(this, "Внимание", "Нет данных для сохранения!");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this, 
        "Сохранить файл", "", "Текстовые файлы (*.txt);;Все файлы (*)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось сохранить файл: " + fileName);
        return;
    }
    
    QTextStream out(&file);
    out << output;
    file.close();
    
    QMessageBox::information(this, "Успешно", "Файл успешно сохранен!");
}

void MainWindow::onPlayfairClear() {
    playfairInput->clear();
    playfairOutput->clear();
    playfairKey1->clear();
    playfairKey2->clear();
    playfairLoadedFilePath.clear();
}

void MainWindow::onVigenereEncrypt() {
    QString key = vigenereKey->text();
    QString input = vigenereInput->toPlainText();
    
    if (key.isEmpty()) {
        QMessageBox::warning(this, "Внимание", "Пожалуйста, введите ключ!\nВведите ключ шифрования!");
        return;
    }
    
    if (input.isEmpty()) {
        QMessageBox::warning(this, "Внимание", "Пожалуйста, введите текст!\nВведите текст для шифрования!");
        return;
    }
    

    bool hasNonRussian = false;
    for (QChar ch : key) {
        if (!((ch >= QChar(0x0410) && ch <= QChar(0x042F)) ||
              (ch >= QChar(0x0430) && ch <= QChar(0x044F)) ||
              ch == QChar(0x0401) || ch == QChar(0x0451))) {
            hasNonRussian = true;
            break;
        }
    }
    
    if (hasNonRussian) {
        QMessageBox::information(this, "Внимание", 
            "Нерусские символы в ключе будут проигнорированы.");
    }
    
    std::string result = vigenereCipher.encrypt(input.toUtf8().toStdString(), key.toUtf8().toStdString());
    vigenereOutput->setPlainText(QString::fromUtf8(result.c_str()));
    
    if (!vigenereLoadedFilePath.isEmpty()) {
        QFileInfo fileInfo(vigenereLoadedFilePath);
        QString baseName = fileInfo.completeBaseName();
        QString dirPath = fileInfo.absolutePath();
        QString outputFileName = dirPath + "/" + baseName + "_enc.txt";
        
        QFile outputFile(outputFileName);
        if (outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&outputFile);
            out << QString::fromUtf8(result.c_str());
            outputFile.close();
            QMessageBox::information(this, "Успешно", "Зашифрованный файл сохранен как:\n" + outputFileName);
        } else {
            QMessageBox::critical(this, "Ошибка", "Не удалось сохранить файл: " + outputFileName);
        }
    }
}

void MainWindow::onVigenereDecrypt() {
    QString key = vigenereKey->text();
    QString input = vigenereInput->toPlainText();
    
    if (key.isEmpty()) {
        QMessageBox::warning(this, "Внимание", "Пожалуйста, введите ключ!\nВведите ключ шифрования!");
        return;
    }
    
    if (input.isEmpty()) {
        QMessageBox::warning(this, "Внимание", "Пожалуйста, введите текст!\nВведите текст для дешифрования!");
        return;
    }
    

    bool hasNonRussian = false;
    for (QChar ch : key) {
        if (!((ch >= QChar(0x0410) && ch <= QChar(0x042F)) ||
              (ch >= QChar(0x0430) && ch <= QChar(0x044F)) ||
              ch == QChar(0x0401) || ch == QChar(0x0451))) {
            hasNonRussian = true;
            break;
        }
    }
    
    if (hasNonRussian) {
        QMessageBox::information(this, "Внимание", 
            "Нерусские символы в ключе будут проигнорированы.");
    }
    
    std::string result = vigenereCipher.decrypt(input.toUtf8().toStdString(), key.toUtf8().toStdString());
    vigenereOutput->setPlainText(QString::fromUtf8(result.c_str()));
    
    if (!vigenereLoadedFilePath.isEmpty()) {
        QFileInfo fileInfo(vigenereLoadedFilePath);
        QString baseName = fileInfo.completeBaseName();
        QString dirPath = fileInfo.absolutePath();
        QString outputFileName = dirPath + "/" + baseName + "_dec.txt";
        
        QFile outputFile(outputFileName);
        if (outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&outputFile);
            out << QString::fromUtf8(result.c_str());
            outputFile.close();
            QMessageBox::information(this, "Успешно", "Расшифрованный файл сохранен как:\n" + outputFileName);
        } else {
            QMessageBox::critical(this, "Ошибка", "Не удалось сохранить файл: " + outputFileName);
        }
    }
}

void MainWindow::onVigenereLoadFile() {
    QString fileName = QFileDialog::getOpenFileName(this, 
        "Открыть текстовый файл", "", "Текстовые файлы (*.txt);;Все файлы (*)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось открыть файл: " + fileName);
        return;
    }
    
    QTextStream in(&file);
    vigenereInput->setPlainText(in.readAll());
    file.close();
    
    vigenereLoadedFilePath = fileName;
}

void MainWindow::onVigenereSaveFile() {
    QString output = vigenereOutput->toPlainText();
    
    if (output.isEmpty()) {
        QMessageBox::warning(this, "Внимание", "Нет данных для сохранения!");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this, 
        "Сохранить файл", "", "Текстовые файлы (*.txt);;Все файлы (*)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось сохранить файл: " + fileName);
        return;
    }
    
    QTextStream out(&file);
    out << output;
    file.close();
    
    QMessageBox::information(this, "Успешно", "Файл успешно сохранен!");
}

void MainWindow::onVigenereClear() {
    vigenereInput->clear();
    vigenereOutput->clear();
    vigenereKey->clear();
    vigenereLoadedFilePath.clear();
}
