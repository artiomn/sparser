#include <QtGui/QApplication>
#include <QtDebug>
#include "sparser_gui.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    sparser_gui w;
    w.show();

    return a.exec();
}
