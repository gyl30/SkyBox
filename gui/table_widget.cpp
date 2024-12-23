#include <QMouseEvent>
#include <QHeaderView>
#include "gui/table_widget.h"

namespace leaf
{
file_table_widget::file_table_widget(QWidget *parent) : QTableWidget(parent)
{
    setShowGrid(false);
    setAlternatingRowColors(true);
    setMouseTracking(true);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QStringList header;
    header << "文件名" << "操作" << "大小" << "时间";
    setColumnCount(4);
    clear();
    setHorizontalHeaderLabels(header);
    horizontalHeader()->setStyleSheet("QHeaderView::section { border: none; }");
    verticalHeader()->setStyleSheet("QHeaderView::section { border: none; }");
}
void file_table_widget::mousePressEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());
    if (!index.isValid())
    {
        clearSelection();
    }
    else
    {
        QTableView::mousePressEvent(event);
    }
}
}    // namespace leaf
