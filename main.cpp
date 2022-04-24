#include "FastStack.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    FastStack w;
    w.show();
    return a.exec();
}
