#include "table_model.h"

namespace leaf
{
task_model::task_model(QObject *parent) : QAbstractTableModel(parent) { start_timer(); }

int task_model::rowCount(const QModelIndex & /*parent*/) const { return static_cast<int>(tasks_.size()); }

int task_model::columnCount(const QModelIndex & /*parent*/) const { return 5; }

QVariant task_model::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (tasks_.empty())
    {
        return {};
    }
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
                value = "源文件 HASH";
                break;
            case 2:
                value = "目标文件";
                break;
            case 3:
                value = "目标文件 HASH";
                break;
            case 4:
                value = "处理进度";
                break;
            default:
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
    return task_to_display_data(index);
}

QVariant task_model::task_to_display_data(const QModelIndex &index) const
{
    if (!index.isValid() || index.column() >= columnCount(index) || index.row() >= rowCount(index))
    {
        return {};
    }
    const int row = index.row();
    const auto &t = tasks_[row]->task_info();
    const int column = index.column();
    if (column == 0)
    {
        return QString::fromStdString(t.name);
    }
    if (column == 1)
    {
        return QString::fromStdString(t.src_hash);
    }
    if (column == 2)
    {
        return QString::fromStdString(t.dst_file);
    }
    if (column == 3)
    {
        return QString::fromStdString(t.dst_hash);
    }
    if (column == 4)
    {
        return t.progress;
    }
    return {};
}

void task_model::add_task(const file_task::ptr &t)
{
    boost::system::error_code ec;
    t->startup(ec);
    t->set_error(ec);
    beginResetModel();
    tasks_.push_back(t);
    endResetModel();
}

void task_model::start_timer()
{
    timer_ = new QTimer(this);
    auto c = connect(timer_, &QTimer::timeout, this, &task_model::on_timer);
    (void)c;
    timer_->start(1000);
}
void task_model::on_timer()
{
    if (tasks_.empty())
    {
        return;
    }
    for (auto &t : tasks_)
    {
        if (t->error())
        {
            continue;
        }
        t->set_error(t->loop());
    }
    beginResetModel();
    endResetModel();
}

}    // namespace leaf
