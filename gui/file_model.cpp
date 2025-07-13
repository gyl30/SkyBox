#include <QPainter>
#include <QPixmap>
#include <QIcon>
#include <QFont>
#include <QMessageBox>
#include <QFileInfo>
#include <string>
#include "gui/util.h"
#include "gui/file_model.h"

namespace leaf
{
file_model::file_model(QObject *parent) : QAbstractListModel(parent) {}

int file_model::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || files_.empty())
    {
        return 0;
    }
    return static_cast<int>(files_.size());
}

QVariant file_model::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || files_.empty() || index.row() >= files_.size())
    {
        return {};
    }
    auto item = files_[index.row()];

    if (role == Qt::DisplayRole)
    {
        return QString::fromStdString(item.display_name);
    }
    if (role == Qt::DecorationRole)
    {
        if (item.type == file_item_type::Folder)
        {
            return leaf::emoji_to_icon("📁", 64);
        }
        return leaf::emoji_to_icon("📄", 64);
    }
    return {};
}

Qt::ItemFlags file_model::flags(const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return Qt::NoItemFlags;
    }
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
}

bool file_model::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || role != Qt::EditRole || files_.empty() || index.row() >= files_.size())
    {
        return false;
    }
    QString new_name = value.toString().trimmed();
    if (new_name.isEmpty())
    {
        return false;
    }

    auto &item = files_[index.row()];
    for (const auto &file : files_)
    {
        if (file.display_name == new_name.toStdString())
        {
            QMessageBox::warning(nullptr, "重命名失败", "名称已存在！");
            return false;
        }
    }
    item.display_name = new_name.toStdString();
    item.storage_name = new_name.toStdString();
    emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole, Qt::ToolTipRole});
    return true;
}

void file_model::set_files(const std::vector<leaf::file_item> &files)
{
    beginResetModel();
    files_ = files;
    endResetModel();
}

std::optional<leaf::file_item> file_model::item_at(int row) const
{
    if (files_.empty() || row < 0 || row >= files_.size())
    {
        return {};
    }
    return files_[row];
}

bool file_model::add_folder(const QString &name, const leaf::file_item &folder)
{
    if (name_exists(name, file_item_type::Folder))
    {
        return false;
    }

    beginInsertRows(QModelIndex(), static_cast<int>(files_.size()), static_cast<int>(files_.size()));
    files_.push_back(folder);
    endInsertRows();
    return true;
}

bool file_model::name_exists(const QString &displayName, file_item_type type) const
{
    for (const auto &file : files_)    // NOLINT
    {
        if (file.type != type)
        {
            continue;
        }
        if (file.display_name == displayName.toStdString())
        {
            return true;
        }
    }
    return false;
}

}    // namespace leaf
