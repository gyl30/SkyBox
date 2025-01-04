#ifndef LEAF_GUI_FILES_WIDGET_H
#define LEAF_GUI_FILES_WIDGET_H

#include <QObject>
#include <QTableWidget>

namespace leaf
{
class files_widget : public QTableWidget
{
   public:
    explicit files_widget(QWidget *parent);
};
}    // namespace leaf

#endif
