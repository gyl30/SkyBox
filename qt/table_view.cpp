#include <QToolTip>
#include <QTableView>
#include <QHeaderView>
#include <QAbstractItemView>

#include "table_view.h"

namespace leaf
{
task_table_view::task_table_view(QWidget *parent) : QTableView(parent)
{
    setShowGrid(false);
    horizontalHeader()->setDefaultAlignment(Qt::AlignHCenter);
    setAlternatingRowColors(true);
    verticalHeader()->setVisible(false);
    horizontalHeader()->setStretchLastSection(true);
    setMouseTracking(true);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    horizontalHeader()->setMinimumWidth(60);
    verticalHeader()->setMinimumSectionSize(18);
    verticalHeader()->setMaximumSectionSize(30);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto c = connect(this, &QTableView::entered, this, &task_table_view::show_tooltip);
    (void)c;
}

void task_table_view::show_tooltip(const QModelIndex &index)
{
    if (!index.isValid())
    {
        return;
    }
    QToolTip::showText(QCursor::pos(), index.data().toString());
}

}    // namespace leaf
