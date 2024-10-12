#include "table_model.h"

namespace leaf
{
task_model::task_model(QObject *parent) : QAbstractTableModel(parent) {}

int task_model::rowCount(const QModelIndex & /*parent*/) const { return 5; }

int task_model::columnCount(const QModelIndex & /*parent*/) const { return 2; }

QVariant task_model::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        return set_header_data(section, role);
    }
    if (orientation == Qt::Vertical)
    {
        return {};
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}
QVariant task_model::set_header_data(int section, int role)
{
    if (role == Qt::DisplayRole)
    {
        QString value;
        switch (section)
        {
            case 0:
                value = "源文件";
                break;
            case 1:
                value = "处理进度";
                break;
        }
        return value;
    }
    return {};
}
QVariant task_model::data(const QModelIndex & /*index*/, int role) const
{
    if (role != Qt::DisplayRole)
    {
        return {};
    }

    return {};
}

}    // namespace leaf
