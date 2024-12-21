#ifndef LEAF_GUI_TABLE_WIDGET_H
#define LEAF_GUI_TABLE_WIDGET_H

#include <QObject>
#include <QTableWidget>

namespace leaf
{
class file_table_widget : public QTableWidget
{
   public:
    explicit file_table_widget(QWidget *parent);

   protected:
    void mousePressEvent(QMouseEvent *event) override;
};

}    // namespace leaf

#endif
