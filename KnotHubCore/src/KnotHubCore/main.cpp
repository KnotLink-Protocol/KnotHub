#include "knothubcore.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    KnotHubCore w;
    w.show();

    return a.exec();
}
