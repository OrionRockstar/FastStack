#include "pch.h"
#include "FastStack.h"

int main(int argc, char *argv[])
{

    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    //QGuiApplication::setHighDdpiScaleFactorRoundingPolicy();
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Floor);
    //QGuiApplication::setAttribute(Qt::setHighDdpiScaleFactorRoundingPolicy,Qt::HighDpiScaleFactorRoundingPolicy::Floor);
    //qputenv("QT_USE_PHYSICAL_DPI","1");
    //qputenv("QT_SCALE_FACTOR", ".8");
    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");

    QApplication a(argc, argv);
    FastStack w;
    w.show();
    return a.exec();
}
