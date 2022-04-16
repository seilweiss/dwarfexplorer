#include "MainWindow.h"

#include <qapplication.h>

#include "Icons.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Icons::init();

    MainWindow w;
    w.show();

    return a.exec();
}
