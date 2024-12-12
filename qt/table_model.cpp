#include "table_model.h"

namespace leaf
{
task_model::task_model(QObject *parent) : QAbstractTableModel(parent) {}

void task_model::add_or_update_task(const leaf::task &task)
{
    bool update = false;
    beginResetModel();
    for (auto &&t : tasks_)
    {
        if (t.id == task.id && task.op == t.op && task.filename == t.filename)
        {
            t.process_size = task.process_size;
            update = true;
            break;
        }
    }
    if (!update)
    {
        tasks_.push_back(task);
    }

    endResetModel();
}

void task_model::delete_task(const leaf::task &task)
{
    beginResetModel();
    tasks_.erase(std::remove_if(tasks_.begin(),
                                tasks_.end(),
                                [&task](const leaf::task &t)
                                { return t.id == task.id && t.op == task.op && t.filename == task.filename; }),
                 tasks_.end());
    endResetModel();
}
int task_model::rowCount(const QModelIndex & /*parent*/) const { return static_cast<int>(tasks_.size()); }

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
QVariant task_model::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole)
    {
        return {};
    }
    if (!index.isValid() || index.column() >= columnCount(index) || index.row() >= rowCount(index))
    {
        return {};
    }
    const size_t row = index.row();
    if (row >= tasks_.size())
    {
        return {};
    }
    const auto &t = tasks_[row];
    const int column = index.column();
    if (column == 0)
    {
        return QString::fromStdString(t.filename);
    }
    if (column == 1)
    {
        return static_cast<double>(t.process_size) * 100 / static_cast<double>(t.file_size);
    }
    return {};
}

}    // namespace leaf
