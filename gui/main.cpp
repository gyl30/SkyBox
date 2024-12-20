#include <QApplication>
#include <QFile>
#include <QTextStream>

#include "log/log.h"
#include "gui/widget.h"
#include "net/scoped_exit.hpp"

int main(int argc, char *argv[])
{
    leaf::init_log("gui.log");
    leaf::set_log_level("trace");

    DEFER(leaf::shutdown_log());

    QApplication a(argc, argv);
    Widget w;
    w.show();
    Q_INIT_RESOURCE(breeze);
    QFile file(":/light/stylesheet.qss");
    file.open(QFile::ReadOnly | QFile::Text);
    QTextStream stream(&file);
    a.setStyleSheet(stream.readAll());
    return QApplication::exec();
}
