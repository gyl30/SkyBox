#include <QApplication>

#include "log/log.h"
#include "gui/loginwindow.h"
#include "net/scoped_exit.hpp"

int main(int argc, char *argv[])
{
    leaf::init_log("gui.log");
    leaf::set_log_level("trace");

    DEFER(leaf::shutdown_log());

    QApplication a(argc, argv);
    login_widget w;
    w.show();
    return QApplication::exec();
}
