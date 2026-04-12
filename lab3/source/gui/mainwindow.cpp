#include "mainwindow.h"
#include "../rsa.h"

#include <wx/notebook.h>
#include <wx/filename.h>
#include <wx/filedlg.h>
#include <wx/statbox.h>
#include <fstream>
#include <vector>

#ifdef __APPLE__
static wxString macChooseFile(const wxString& prompt) {
    wxString cmd = wxString::Format(
        "osascript -e 'try' "
        "-e 'POSIX path of (choose file with prompt \"%s\")' "
        "-e 'end try'", prompt);
    FILE* p = popen(cmd.mb_str(), "r");
    if (!p) return {};
    char buf[4096] = {};
    fgets(buf, sizeof(buf), p);
    pclose(p);
    wxString result = wxString::FromUTF8(buf);
    result.Trim();
    return result;
}
#endif


static void writeLE16(std::ofstream& f, uint16_t v) {
    char bytes[2] = { char(v & 0xFF), char((v >> 8) & 0xFF) };
    f.write(bytes, 2);
}

static uint16_t readLE16(const std::vector<uint8_t>& data, size_t offset) {
    return uint16_t(data[offset]) | (uint16_t(data[offset + 1]) << 8);
}

void MainWindow::setStatus(wxStaticText* lbl, const wxString& msg, bool ok) {
    lbl->SetLabel(msg);
    lbl->SetForegroundColour(ok ? wxColour(27, 94, 32) : wxColour(183, 28, 28));
    wxFont f = lbl->GetFont();
    f.SetWeight(wxFONTWEIGHT_BOLD);
    lbl->SetFont(f);
    lbl->GetParent()->Layout();
    lbl->GetParent()->Refresh();
}

bool MainWindow::validateEncParams(int64_t& p, int64_t& q, int64_t& kc,
                                    int64_t& r, int64_t& euler, int64_t& ko)
{
    wxString pStr = m_pEdit->GetValue();   pStr.Trim(true).Trim(false);
    wxString qStr = m_qEdit->GetValue();   qStr.Trim(true).Trim(false);
    wxString kStr = m_kcEncEdit->GetValue(); kStr.Trim(true).Trim(false);

    long long pp, qq, kp;
    bool pOk  = pStr.ToLongLong(&pp);
    bool qOk  = qStr.ToLongLong(&qq);
    bool kcOk = kStr.ToLongLong(&kp);
    p = pp; q = qq; kc = kp;

    if (!pOk || !qOk) {
        setStatus(m_encParamStatus, "Введите оба числа p и q.", false);
        return false;
    }
    if (!rsa::is_prime(p)) {
        setStatus(m_encParamStatus, wxString::Format("p = %lld \u2014 не простое число.", (long long)p), false);
        return false;
    }
    if (!rsa::is_prime(q)) {
        setStatus(m_encParamStatus, wxString::Format("q = %lld \u2014 не простое число.", (long long)q), false);
        return false;
    }
    if (p == q) {
        setStatus(m_encParamStatus, "p и q должны быть различными простыми числами.", false);
        return false;
    }

    r     = p * q;
    euler = (p - 1) * (q - 1);

    if (r <= 255) {
        setStatus(m_encParamStatus,
            wxString::Format("r = %lld \u2264 255: возьмите бо\u0301льшие p и q "
                "(нужно r > 255, чтобы M < r для любого байта).", (long long)r), false);
        return false;
    }
    if (r >= 65536) {
        setStatus(m_encParamStatus,
            wxString::Format("r = %lld \u2265 65536: зашифрованный блок не влезет в 2 байта. "
                "Уменьшите p или q.", (long long)r), false);
        return false;
    }

    if (!kcOk) {
        setStatus(m_encParamStatus, "Введите закрытый ключ KC.", false);
        return false;
    }
    if (kc <= 1 || kc >= euler) {
        setStatus(m_encParamStatus,
            wxString::Format("KC должен быть в диапазоне (1, %lld).", (long long)euler), false);
        return false;
    }
    int64_t g = rsa::gcd(kc, euler);
    if (g != 1) {
        setStatus(m_encParamStatus,
            wxString::Format("НОД(KC=%lld, \u03c6(r)=%lld) = %lld \u2260 1.\n"
                "KC и \u03c6(r) должны быть взаимно простыми.",
                (long long)kc, (long long)euler, (long long)g), false);
        return false;
    }

    ko = rsa::mod_inverse(kc, euler);
    return true;
}

void MainWindow::onEncryptParamsChanged() {
    m_rLabel->SetLabel("\u2014");
    m_eulerLabel->SetLabel("\u2014");
    m_koLabel->SetLabel("\u2014");

    int64_t p, q, kc, r, euler, ko;
    if (!validateEncParams(p, q, kc, r, euler, ko)) {
        wxString pStr = m_pEdit->GetValue();   pStr.Trim(true).Trim(false);
        wxString qStr = m_qEdit->GetValue();   qStr.Trim(true).Trim(false);
        long long pp, qq;
        bool pOk = pStr.ToLongLong(&pp);
        bool qOk = qStr.ToLongLong(&qq);
        if (pOk && qOk && rsa::is_prime(pp) && rsa::is_prime(qq) && pp != qq) {
            m_rLabel->SetLabel(wxString::Format("%lld", pp * qq));
            m_eulerLabel->SetLabel(wxString::Format("%lld", (pp - 1) * (qq - 1)));
        }
        return;
    }

    m_rLabel->SetLabel(wxString::Format("%lld", (long long)r));
    m_eulerLabel->SetLabel(wxString::Format("%lld", (long long)euler));
    m_koLabel->SetLabel(wxString::Format("%lld", (long long)ko));

    setStatus(m_encParamStatus,
        wxString::Format("Параметры верны. r=%lld, \u03c6(r)=%lld, KO=%lld",
            (long long)r, (long long)euler, (long long)ko), true);
}


wxPanel* MainWindow::buildEncryptTab(wxNotebook* nb) {
    wxPanel* panel = new wxPanel(nb, wxID_ANY);
    wxBoxSizer* outer = new wxBoxSizer(wxVERTICAL);

    wxFont mono(wxFontInfo(10).Family(wxFONTFAMILY_TELETYPE));

    wxStaticBox*     paramsBox   = new wxStaticBox(panel, wxID_ANY, "Параметры RSA");
    wxStaticBoxSizer* paramsSizer = new wxStaticBoxSizer(paramsBox, wxVERTICAL);

    wxFlexGridSizer* pg = new wxFlexGridSizer(3, 2, 6, 6);
    pg->AddGrowableCol(1, 1);

    pg->Add(new wxStaticText(panel, wxID_ANY, "Простое p:"), 0, wxALIGN_CENTER_VERTICAL);
    m_pEdit = new wxTextCtrl(panel, wxID_ANY);
    m_pEdit->SetHint("напр. 251");
    pg->Add(m_pEdit, 1, wxEXPAND);

    pg->Add(new wxStaticText(panel, wxID_ANY, "Простое q:"), 0, wxALIGN_CENTER_VERTICAL);
    m_qEdit = new wxTextCtrl(panel, wxID_ANY);
    m_qEdit->SetHint("напр. 257");
    pg->Add(m_qEdit, 1, wxEXPAND);

    pg->Add(new wxStaticText(panel, wxID_ANY, "Закрытый ключ KC:"), 0, wxALIGN_CENTER_VERTICAL);
    m_kcEncEdit = new wxTextCtrl(panel, wxID_ANY);
    m_kcEncEdit->SetHint("1 < KC < \u03c6(r),  НОД(KC,\u03c6(r))=1");
    pg->Add(m_kcEncEdit, 1, wxEXPAND);

    paramsSizer->Add(pg, 0, wxEXPAND | wxALL, 5);

    wxButton* calcBtn = new wxButton(panel, wxID_ANY, "Вычислить KO");
    calcBtn->SetBackgroundColour(wxColour(21, 101, 192));
    calcBtn->SetForegroundColour(*wxWHITE);
    paramsSizer->Add(calcBtn, 0, wxEXPAND | wxALL, 5);

    calcBtn->Bind(wxEVT_BUTTON,
        [this](wxCommandEvent&) { onEncryptParamsChanged(); });
    m_pEdit->Bind(wxEVT_KILL_FOCUS,
        [this](wxFocusEvent& e) { onEncryptParamsChanged(); e.Skip(); });
    m_qEdit->Bind(wxEVT_KILL_FOCUS,
        [this](wxFocusEvent& e) { onEncryptParamsChanged(); e.Skip(); });
    m_kcEncEdit->Bind(wxEVT_KILL_FOCUS,
        [this](wxFocusEvent& e) { onEncryptParamsChanged(); e.Skip(); });

    outer->Add(paramsSizer, 0, wxEXPAND | wxALL, 5);

    wxStaticBox*     calcBox   = new wxStaticBox(panel, wxID_ANY, "Вычисленные значения");
    wxStaticBoxSizer* calcSizer = new wxStaticBoxSizer(calcBox, wxVERTICAL);

    wxFlexGridSizer* cg = new wxFlexGridSizer(3, 3, 4, 4);
    cg->AddGrowableCol(1, 1);

    cg->Add(new wxStaticText(panel, wxID_ANY, "r = p \u00b7 q:"), 0, wxALIGN_CENTER_VERTICAL);
    m_rLabel = new wxStaticText(panel, wxID_ANY, "\u2014");
    m_rLabel->SetFont(mono);
    cg->Add(m_rLabel, 0, wxALIGN_CENTER_VERTICAL);
    cg->Add(new wxStaticText(panel, wxID_ANY, "  (нужно: 255 < r < 65536)"),
            0, wxALIGN_CENTER_VERTICAL);

    cg->Add(new wxStaticText(panel, wxID_ANY, "\u03c6(r) = (p\u22121)\u00b7(q\u22121):"),
            0, wxALIGN_CENTER_VERTICAL);
    m_eulerLabel = new wxStaticText(panel, wxID_ANY, "\u2014");
    m_eulerLabel->SetFont(mono);
    cg->Add(m_eulerLabel, 0, wxALIGN_CENTER_VERTICAL);
    cg->Add(new wxStaticText(panel, wxID_ANY, ""), 0);

    cg->Add(new wxStaticText(panel, wxID_ANY, "KO (открытый ключ):"), 0, wxALIGN_CENTER_VERTICAL);
    m_koLabel = new wxStaticText(panel, wxID_ANY, "\u2014");
    {
        wxFont koFont = mono;
        koFont.SetWeight(wxFONTWEIGHT_BOLD);
        m_koLabel->SetFont(koFont);
    }
    m_koLabel->SetForegroundColour(wxColour(13, 71, 161));
    cg->Add(m_koLabel, 0, wxALIGN_CENTER_VERTICAL);
    cg->Add(new wxStaticText(panel, wxID_ANY, ""), 0);

    calcSizer->Add(cg, 0, wxEXPAND | wxALL, 5);
    outer->Add(calcSizer, 0, wxEXPAND | wxALL, 5);

    m_encParamStatus = new wxStaticText(panel, wxID_ANY, "",
        wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);
    outer->Add(m_encParamStatus, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

    wxStaticBox*     fileBox   = new wxStaticBox(panel, wxID_ANY, "Входной файл");
    wxStaticBoxSizer* fileSizer = new wxStaticBoxSizer(fileBox, wxHORIZONTAL);
    m_encFileEdit = new wxTextCtrl(panel, wxID_ANY);
    m_encFileEdit->SetHint("Путь к файлу...");
    m_encBrowseBtn = new wxButton(panel, wxID_ANY, "Обзор...",
        wxDefaultPosition, wxSize(80, -1));
    m_encBrowseBtn->Bind(wxEVT_BUTTON,
        [this](wxCommandEvent&) { onEncryptBrowse(); });
    fileSizer->Add(m_encFileEdit, 1, wxEXPAND | wxALL, 5);
    fileSizer->Add(m_encBrowseBtn, 0, wxALL, 5);
    outer->Add(fileSizer, 0, wxEXPAND | wxALL, 5);

    m_encryptBtn = new wxButton(panel, wxID_ANY, "Зашифровать");
    m_encryptBtn->SetMinSize(wxSize(-1, 34));
    m_encryptBtn->SetBackgroundColour(wxColour(13, 71, 161));
    m_encryptBtn->SetForegroundColour(*wxWHITE);
    m_encryptBtn->Bind(wxEVT_BUTTON,
        [this](wxCommandEvent&) { onEncrypt(); });
    outer->Add(m_encryptBtn, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

    wxStaticBox*     encOutBox   = new wxStaticBox(panel, wxID_ANY,
        "Зашифрованные блоки (исходный -> зашифрованный)");
    wxStaticBoxSizer* encOutSizer = new wxStaticBoxSizer(encOutBox, wxVERTICAL);
    m_encOutput = new wxTextCtrl(panel, wxID_ANY, "",
        wxDefaultPosition, wxDefaultSize,
        wxTE_MULTILINE | wxTE_READONLY | wxHSCROLL);
    m_encOutput->SetFont(mono);
    encOutSizer->Add(m_encOutput, 1, wxEXPAND | wxALL, 5);
    outer->Add(encOutSizer, 1, wxEXPAND | wxALL, 5);

    panel->SetSizer(outer);
    return panel;
}


wxPanel* MainWindow::buildDecryptTab(wxNotebook* nb) {
    wxPanel* panel = new wxPanel(nb, wxID_ANY);
    wxBoxSizer* outer = new wxBoxSizer(wxVERTICAL);

    wxFont mono(wxFontInfo(10).Family(wxFONTFAMILY_TELETYPE));

    wxStaticBox*     paramsBox   = new wxStaticBox(panel, wxID_ANY, "Параметры RSA");
    wxStaticBoxSizer* paramsSizer = new wxStaticBoxSizer(paramsBox, wxVERTICAL);

    wxFlexGridSizer* pg = new wxFlexGridSizer(2, 2, 6, 6);
    pg->AddGrowableCol(1, 1);

    pg->Add(new wxStaticText(panel, wxID_ANY, "Модуль r = p\u00b7q:"), 0, wxALIGN_CENTER_VERTICAL);
    m_rDecEdit = new wxTextCtrl(panel, wxID_ANY);
    m_rDecEdit->SetHint("использовался при шифровании (256..65535)");
    pg->Add(m_rDecEdit, 1, wxEXPAND);

    pg->Add(new wxStaticText(panel, wxID_ANY, "Закрытый ключ KC:"), 0, wxALIGN_CENTER_VERTICAL);
    m_kcDecEdit = new wxTextCtrl(panel, wxID_ANY);
    m_kcDecEdit->SetHint("использовался при шифровании");
    pg->Add(m_kcDecEdit, 1, wxEXPAND);

    paramsSizer->Add(pg, 0, wxEXPAND | wxALL, 5);
    outer->Add(paramsSizer, 0, wxEXPAND | wxALL, 5);

    m_decParamStatus = new wxStaticText(panel, wxID_ANY, "",
        wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);
    outer->Add(m_decParamStatus, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

    wxStaticBox*     fileBox   = new wxStaticBox(panel, wxID_ANY, "Зашифрованный файл");
    wxStaticBoxSizer* fileSizer = new wxStaticBoxSizer(fileBox, wxHORIZONTAL);
    m_decFileEdit = new wxTextCtrl(panel, wxID_ANY);
    m_decFileEdit->SetHint("Путь к зашифрованному файлу...");
    m_decBrowseBtn = new wxButton(panel, wxID_ANY, "Обзор...",
        wxDefaultPosition, wxSize(80, -1));
    m_decBrowseBtn->Bind(wxEVT_BUTTON,
        [this](wxCommandEvent&) { onDecryptBrowse(); });
    fileSizer->Add(m_decFileEdit, 1, wxEXPAND | wxALL, 5);
    fileSizer->Add(m_decBrowseBtn, 0, wxALL, 5);
    outer->Add(fileSizer, 0, wxEXPAND | wxALL, 5);

    m_decryptBtn = new wxButton(panel, wxID_ANY, "Расшифровать");
    m_decryptBtn->SetMinSize(wxSize(-1, 34));
    m_decryptBtn->SetBackgroundColour(wxColour(27, 94, 32));
    m_decryptBtn->SetForegroundColour(*wxWHITE);
    m_decryptBtn->Bind(wxEVT_BUTTON,
        [this](wxCommandEvent&) { onDecrypt(); });
    outer->Add(m_decryptBtn, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

    wxStaticBox*     outBox   = new wxStaticBox(panel, wxID_ANY,
        "Расшифрованные блоки (зашифрованный -> исходный)");
    wxStaticBoxSizer* outSizer = new wxStaticBoxSizer(outBox, wxVERTICAL);
    m_decOutput = new wxTextCtrl(panel, wxID_ANY, "",
        wxDefaultPosition, wxDefaultSize,
        wxTE_MULTILINE | wxTE_READONLY | wxHSCROLL);
    m_decOutput->SetFont(mono);
    outSizer->Add(m_decOutput, 1, wxEXPAND | wxALL, 5);
    outer->Add(outSizer, 1, wxEXPAND | wxALL, 5);

    panel->SetSizer(outer);
    return panel;
}


MainWindow::MainWindow(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(900, 680))
{
    CreateStatusBar();
    SetStatusText("Введите p, q (простые, 255 < p\u00b7q < 65536) и закрытый ключ KC");

    wxNotebook* nb = new wxNotebook(this, wxID_ANY);
    nb->AddPage(buildEncryptTab(nb), "  Шифрование  ");
    nb->AddPage(buildDecryptTab(nb), "  Расшифрование  ");

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(nb, 1, wxEXPAND);
    SetSizer(mainSizer);
    SetMinSize(wxSize(900, 680));
}


void MainWindow::onEncryptBrowse() {
#ifdef __APPLE__
    wxString path = macChooseFile("Выберите файл для шифрования");
    if (!path.IsEmpty()) m_encFileEdit->SetValue(path);
#else
    wxFileDialog dlg(nullptr, "Выберите файл для шифрования", "", "",
        "Все файлы (*)|*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() == wxID_OK)
        m_encFileEdit->SetValue(dlg.GetPath());
#endif
}

void MainWindow::onDecryptBrowse() {
#ifdef __APPLE__
    wxString path = macChooseFile("Выберите зашифрованный файл");
    if (!path.IsEmpty()) m_decFileEdit->SetValue(path);
#else
    wxFileDialog dlg(nullptr, "Выберите зашифрованный файл", "", "",
        "Все файлы (*)|*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() == wxID_OK)
        m_decFileEdit->SetValue(dlg.GetPath());
#endif
}


void MainWindow::onEncrypt() {
    int64_t p, q, kc, r, euler, ko;
    if (!validateEncParams(p, q, kc, r, euler, ko)) {
        wxMessageBox(m_encParamStatus->GetLabel(), "Ошибка параметров",
            wxOK | wxICON_WARNING, this);
        return;
    }

    wxString inputPath = m_encFileEdit->GetValue();
    inputPath.Trim(true).Trim(false);
    if (inputPath.IsEmpty()) {
        wxMessageBox("Укажите входной файл.", "Ошибка", wxOK | wxICON_WARNING, this);
        return;
    }

    std::ifstream fin(inputPath.ToStdString(), std::ios::binary);
    if (!fin) {
        wxMessageBox("Не удалось открыть:\n" + inputPath, "Ошибка",
            wxOK | wxICON_ERROR, this);
        return;
    }
    std::vector<uint8_t> data(
        (std::istreambuf_iterator<char>(fin)),
        std::istreambuf_iterator<char>());
    fin.close();

    if (data.empty()) {
        wxMessageBox("Файл пуст.", "Предупреждение", wxOK | wxICON_WARNING, this);
        return;
    }

    {
        int64_t testM  = static_cast<int64_t>(data[0]);
        int64_t testC  = rsa::power_mod(testM, ko, r);
        int64_t testM2 = rsa::power_mod(testC, kc, r);
        if (testM2 != testM) {
            wxMessageBox(
                wxString::Format("Проверка round-trip ПРОВАЛИЛАСЬ:\n"
                    "M=%lld  \u2192  C=%lld  \u2192  M'=%lld\n\n"
                    "Параметры p, q, KC не обеспечивают корректное шифрование/расшифрование.",
                    (long long)testM, (long long)testC, (long long)testM2),
                "Ошибка параметров RSA", wxOK | wxICON_ERROR, this);
            return;
        }
    }

    wxFileName encInfo(inputPath);
    wxString baseName = encInfo.GetName();
    wxString ext = encInfo.GetExt();
    wxString outputPath = encInfo.GetPath() + wxFileName::GetPathSeparator() +
        baseName + "_enc" + (ext.IsEmpty() ? "" : "." + ext);

    std::ofstream fout(outputPath.ToStdString(), std::ios::binary);
    if (!fout) {
        wxMessageBox("Не удалось создать:\n" + outputPath, "Ошибка",
            wxOK | wxICON_ERROR, this);
        return;
    }

    wxString display;
    for (uint8_t byte : data) {
        int64_t M = static_cast<int64_t>(byte);
        int64_t C = rsa::power_mod(M, ko, r);
        writeLE16(fout, static_cast<uint16_t>(C));
        display += wxString::Format("%lld -> %lld\n", (long long)M, (long long)C);
    }
    fout.close();

    m_encOutput->SetValue(display);

    m_rDecEdit->SetValue(wxString::Format("%lld", (long long)r));
    m_kcDecEdit->SetValue(wxString::Format("%lld", (long long)kc));
    m_decFileEdit->SetValue(outputPath);
    setStatus(m_decParamStatus,
        wxString::Format("Заполнено автоматически: r=%lld, KC=%lld",
            (long long)r, (long long)kc), true);

    SetStatusText(wxString::Format("Зашифровано %zu байт  \u2192  %s",
        data.size(), outputPath));

    wxMessageBox(
        wxString::Format(
            "Файл зашифрован успешно.\n\n"
            "Входной файл: %s\n"
            "Выходной файл: %s\n"
            "Байт: %zu\n\n"
            "p=%lld, q=%lld, r=%lld\n"
            "\u03c6(r)=%lld\n"
            "KC (закрытый) = %lld\n"
            "KO (открытый) = %lld\n\n"
            "Параметры r и KC автоматически перенесены\nна вкладку \u00abРасшифрование\u00bb.",
            inputPath, outputPath, data.size(),
            (long long)p, (long long)q, (long long)r,
            (long long)euler, (long long)kc, (long long)ko),
        "Готово", wxOK | wxICON_INFORMATION, this);
}


void MainWindow::onDecrypt() {
    wxString rStr = m_rDecEdit->GetValue();   rStr.Trim(true).Trim(false);
    wxString kStr = m_kcDecEdit->GetValue();  kStr.Trim(true).Trim(false);

    long long rv, kcv;
    bool rOk  = rStr.ToLongLong(&rv);
    bool kcOk = kStr.ToLongLong(&kcv);
    int64_t r = rv, kc = kcv;

    if (!rOk || !kcOk) {
        wxMessageBox("Заполните поля r и KC.", "Ошибка", wxOK | wxICON_WARNING, this);
        return;
    }
    if (r <= 255 || r >= 65536) {
        setStatus(m_decParamStatus,
            wxString::Format("r = %lld должно быть в диапазоне (255, 65536).", (long long)r), false);
        wxMessageBox(m_decParamStatus->GetLabel(), "Ошибка", wxOK | wxICON_WARNING, this);
        return;
    }
    if (kc <= 1) {
        setStatus(m_decParamStatus, "KC должен быть > 1.", false);
        wxMessageBox(m_decParamStatus->GetLabel(), "Ошибка", wxOK | wxICON_WARNING, this);
        return;
    }
    setStatus(m_decParamStatus,
        wxString::Format("r = %lld,  KC = %lld", (long long)r, (long long)kc), true);

    wxString inputPath = m_decFileEdit->GetValue();
    inputPath.Trim(true).Trim(false);
    if (inputPath.IsEmpty()) {
        wxMessageBox("Укажите зашифрованный файл.", "Ошибка", wxOK | wxICON_WARNING, this);
        return;
    }

    std::ifstream fin(inputPath.ToStdString(), std::ios::binary);
    if (!fin) {
        wxMessageBox("Не удалось открыть:\n" + inputPath, "Ошибка",
            wxOK | wxICON_ERROR, this);
        return;
    }
    std::vector<uint8_t> enc(
        (std::istreambuf_iterator<char>(fin)),
        std::istreambuf_iterator<char>());
    fin.close();

    if (enc.size() % 2 != 0) {
        wxMessageBox(
            wxString::Format("Размер файла (%zu байт) не кратен 2.\n"
                "Последний неполный блок будет пропущен.", enc.size()),
            "Предупреждение", wxOK | wxICON_WARNING, this);
    }

    wxFileName decInfo(inputPath);
    wxString baseName = decInfo.GetName();
    if (baseName.EndsWith("_enc") || baseName.EndsWith("_ENC"))
        baseName = baseName.Left(baseName.Length() - 4);
    wxString ext = decInfo.GetExt();
    wxString outputPath = decInfo.GetPath() + wxFileName::GetPathSeparator() +
        baseName + "_dec" + (ext.IsEmpty() ? "" : "." + ext);

    std::ofstream fout(outputPath.ToStdString(), std::ios::binary);
    if (!fout) {
        wxMessageBox("Не удалось создать:\n" + outputPath, "Ошибка",
            wxOK | wxICON_ERROR, this);
        return;
    }

    wxString display;
    bool hasRangeWarn = false;
    size_t count = 0;

    for (size_t i = 0; i + 1 < enc.size(); i += 2) {
        uint16_t c16 = readLE16(enc, i);
        int64_t  C   = static_cast<int64_t>(c16);
        int64_t  M   = rsa::power_mod(C, kc, r);

        if (M > 255) hasRangeWarn = true;

        uint8_t byte = static_cast<uint8_t>(M & 0xFF);
        fout.write(reinterpret_cast<const char*>(&byte), 1);
        display += wxString::Format("%lld -> %d\n", (long long)C, (int)byte);
        ++count;
    }
    fout.close();

    m_decOutput->SetValue(display);

    SetStatusText(wxString::Format("Расшифровано %zu байт  \u2192  %s",
        count, outputPath));

    if (hasRangeWarn) {
        wxMessageBox(
            "Некоторые расшифрованные значения вышли за диапазон [0..255].\n"
            "Возможно, параметры r или KC введены неверно.",
            "Предупреждение", wxOK | wxICON_WARNING, this);
    } else {
        wxMessageBox(
            wxString::Format("Файл расшифрован успешно.\n\nФайл: %s\nБайт: %zu",
                outputPath, count),
            "Готово", wxOK | wxICON_INFORMATION, this);
    }
}
