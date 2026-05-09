#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("DSA Lab4");
    app.setOrganizationName("BSUIR");
    MainWindow w;
    w.show();
    return app.exec();
}
