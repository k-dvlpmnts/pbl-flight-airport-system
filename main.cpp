#include "mainwindow.h"
#include "theme.h"
#include "serverapi.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    qRegisterMetaType<Reply>("Reply");
    Theme::apply(app);

    MainWindow w;
    w.show();
    return app.exec();
}
