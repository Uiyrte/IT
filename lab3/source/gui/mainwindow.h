#pragma once
#include <wx/wx.h>
#include <wx/notebook.h>
#include <cstdint>

class MainWindow : public wxFrame {
public:
    explicit MainWindow(const wxString& title);

private:
    wxTextCtrl*   m_pEdit;
    wxTextCtrl*   m_qEdit;
    wxTextCtrl*   m_kcEncEdit;

    wxStaticText* m_rLabel;
    wxStaticText* m_eulerLabel;
    wxStaticText* m_koLabel;
    wxStaticText* m_encParamStatus;

    wxTextCtrl*   m_encFileEdit;
    wxButton*     m_encBrowseBtn;
    wxButton*     m_encryptBtn;
    wxTextCtrl*   m_encOutput;

    wxTextCtrl*   m_rDecEdit;
    wxTextCtrl*   m_kcDecEdit;
    wxStaticText* m_decParamStatus;

    wxTextCtrl*   m_decFileEdit;
    wxButton*     m_decBrowseBtn;
    wxButton*     m_decryptBtn;
    wxTextCtrl*   m_decOutput;

    wxPanel* buildEncryptTab(wxNotebook* nb);
    wxPanel* buildDecryptTab(wxNotebook* nb);
    void     setStatus(wxStaticText* lbl, const wxString& msg, bool ok);
    bool     validateEncParams(int64_t& p, int64_t& q, int64_t& kc,
                                int64_t& r, int64_t& euler, int64_t& ko);

    void onEncryptParamsChanged();
    void onEncryptBrowse();
    void onEncrypt();
    void onDecryptBrowse();
    void onDecrypt();
};
