#include "pch.h"
#include "FastStack.h"

class DarkPalette : public QPalette {
public:
    DarkPalette() {
        this->setColor(QPalette::Window, QColor(39, 39, 39));
        //this->setColor(QPalette::Inactive, QPalette::Window, QColor(69, 69, 69));
        //this->setColor(QPalette::Disabled, QPalette::Window, QColor(69, 69, 69));

        this->setColor(QPalette::WindowText, Qt::white);
        this->setColor(QPalette::Button, QColor(169, 169, 169));

        this->setColor(QPalette::Disabled, QPalette::Base, QColor(96, 96, 96));
        this->setColor(QPalette::Active, QPalette::Base, QColor(169, 169, 169));
        this->setColor(QPalette::Inactive, QPalette::Base, QColor(169, 169, 169));
        this->setColor(QPalette::Disabled, QPalette::Text, QColor(196, 196, 196));

        this->setColor(QPalette::Highlight, QColor(69, 69, 69));

        this->setColor(QPalette::ToolTipBase, QColor(126, 126, 126));
        this->setColor(QPalette::Light, QColor(169,169,169));
        //this->setColor(QPalette::ToolTipText, Qt::white);

        //this->setColor(QPalette::Disabled, QPalette::Text, Qt::black);
        //this->setColor(QPalette::Active, QPalette::Text, Qt::white);
        //this->setColor(QPalette::Inactive, QPalette::Text, Qt::white);
    }

};

int main(int argc, char *argv[])
{

    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    //QGuiApplication::setHighDdpiScaleFactorRoundingPolicy();
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Floor);

    QApplication::setStyle(QStyleFactory::create("Fusion"));
    //QGuiApplication::setAttribute(Qt::setHighDdpiScaleFactorRoundingPolicy,Qt::HighDpiScaleFactorRoundingPolicy::Floor);
    //qputenv("QT_USE_PHYSICAL_DPI","1");
    //qputenv("QT_SCALE_FACTOR", ".8");
    //qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
    //argv[1] = 'darkmode';
    QApplication a(argc, argv);
    //a.setStyle("darkmode");
    //QPalette dp;

    //DarkPalette dp;

    a.setPalette(DarkPalette());
    FastStack w;
    w.show();
    return a.exec();
}
