#include <wx/wx.h>
#include "mainwindow.h"

class RsaApp : public wxApp {
public:
    bool OnInit() override {
        SetAppName("RSA Lab3");
        SetVendorName("BSUIR");

        MainWindow* frame = new MainWindow("RSA \u2014 Лабораторная работа 3");
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(RsaApp);
