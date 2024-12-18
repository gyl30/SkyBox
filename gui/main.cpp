#include <QApplication>
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
    return QApplication::exec();
}
