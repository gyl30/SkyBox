#include <QApplication>
#include <QFile>
#include <QTextStream>

#include <absl/cleanup/cleanup.h>

#include "log/log.h"
#include "gui/widget.h"

int main(int argc, char *argv[])
{
    leaf::init_log("gui.log");
    leaf::set_log_level("trace");
    auto cleanup = absl::MakeCleanup(leaf::shutdown_log);

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
