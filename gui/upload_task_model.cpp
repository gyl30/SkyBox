#include <QFileInfo>
#include "gui/util.h"
#include "file/event.h"
#include "gui/upload_task_model.h"

file_task_model::file_task_model(QObject *parent) : QAbstractListModel(parent) {}

int file_task_model::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
    {
        return 0;
    }
    return static_cast<int>(tasks_.count());
}

QVariant file_task_model::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= tasks_.count())
    {
        return {};
    }

    const leaf::file_event &task = tasks_[index.row()];

    if (role == static_cast<int>(leaf::task_role::kFullEventRole))
    {
        return QVariant::fromValue(task);
    }

    if (role == Qt::DisplayRole)
    {
        return QFileInfo(QString::fromStdString(task.filename)).fileName();
    }

    return {};
}

int file_task_model::find_task_index(const std::string &filename) const
{
    for (int i = 0; i < tasks_.count(); ++i)
    {
        if (tasks_[i].filename == filename)
        {
            return i;
        }
    }
    return -1;
}

void file_task_model::add_task(const leaf::file_event &e)
{
    int existing_index = find_task_index(e.filename);
    if (existing_index != -1)
    {
        update_task(e);
        return;
    }

    beginInsertRows(QModelIndex(), rowCount(QModelIndex()), rowCount(QModelIndex()));
    tasks_.append(e);
    endInsertRows();
}

void file_task_model::update_task(const leaf::file_event &e)
{
    int index = find_task_index(e.filename);
    if (index != -1)
    {
        tasks_[index] = e;
        QModelIndex model_index = this->index(index, 0);
        emit dataChanged(model_index, model_index, {static_cast<int>(leaf::task_role::kFullEventRole)});
    }
}

void file_task_model::remove_task(const leaf::file_event &e)
{
    int index = find_task_index(e.filename);
    if (index != -1)
    {
        beginRemoveRows(QModelIndex(), index, index);
        tasks_.removeAt(index);
        endRemoveRows();
    }
}
