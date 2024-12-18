#include <QToolTip>
#include <QTableView>
#include <QHeaderView>
#include <QAbstractItemView>

#include "gui/table_view.h"

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
}
}    // namespace leaf
