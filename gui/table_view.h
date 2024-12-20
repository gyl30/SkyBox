#ifndef LEAF_GUI_TABLE_VIEW_H
#define LEAF_GUI_TABLE_VIEW_H

#include <QObject>
#include <QTableView>

namespace leaf
{
class task_table_view : public QTableView
{
   public:
    explicit task_table_view(QWidget *parent);

   protected:
    void mousePressEvent(QMouseEvent *event) override;
};

}    // namespace leaf

#endif
