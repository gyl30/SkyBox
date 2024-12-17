#ifndef LEAF_TABLE_VIEW_H
#define LEAF_TABLE_VIEW_H

#include <QObject>
#include <QTableView>

namespace leaf
{
class task_table_view : public QTableView
{
   public:
    explicit task_table_view(QWidget *parent);
};

}    // namespace leaf

#endif
