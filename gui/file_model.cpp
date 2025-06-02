#include "file_model.h"
#include <QPainter>
#include <QPixmap>
#include <QIcon>
#include <QFont>
#include <QMessageBox>
#include <QFileInfo>
#include <string>

static QIcon emoji_to_icon(const QString &emoji)
{
    QPixmap pix(64, 64);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    QFont font("Noto Color Emoji");    // 确保这个字体存在
    font.setPointSize(32);
    p.setFont(font);
    p.drawText(pix.rect(), Qt::AlignCenter, emoji);
    QIcon icon(pix);
    return icon;
}

static std::string format_file_size(int64_t bytes)
{
    if (bytes < 0)
    {
        return "未知大小";
    }
    if (bytes < 1024)
    {
        return std::to_string(bytes) + " B";
    }
    double kb = static_cast<double>(bytes) / 1024.0;
    if (kb < 1024)
    {
        return std::to_string(kb) + " KB";
    }
    double mb = kb / 1024.0;
    if (mb < 1024)
    {
        return std::to_string(mb) + " MB";
    }
    double gb = mb / 1024.0;
    return std::to_string(gb) + " GB";
}

file_model::file_model(QObject *parent) : QAbstractListModel(parent) {}

int file_model::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || !current_dir_)
    {
        return 0;
    }
    return static_cast<int>(current_dir_->children_.size());
}

QVariant file_model::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !current_dir_ || index.row() >= current_dir_->children_.size())
    {
        return {};
    }
    const std::shared_ptr<file_item> &item = current_dir_->children_[index.row()];

    if (role == Qt::DisplayRole)
    {
        return item->display_name_;
    }
    if (role == Qt::DecorationRole)
    {
        if (item->type_ == file_item_type::Folder)
        {
            return emoji_to_icon("📁");
        }
        return emoji_to_icon("📄");
    }
    if (role == Qt::ToolTipRole)
    {
        if (item->type_ == file_item_type::Folder)
        {
            return QString("文件夹\n包含 %1 个项目").arg(item->children_.size());
        }
        return QString("文件: %1\n大小: %2\n修改日期: %3")
            .arg(item->display_name_)
            .arg(QString::fromStdString(format_file_size(item->file_size_)))
            .arg(item->last_modified_.toString("yyyy-MM-dd hh:mm:ss"));
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
    if (!index.isValid() || role != Qt::EditRole || !current_dir_ || index.row() >= current_dir_->children_.size())
    {
        return false;
    }
    QString new_name = value.toString().trimmed();
    if (new_name.isEmpty())
    {
        return false;
    }

    const auto &item_being_edited = current_dir_->children_[index.row()];
    for (const auto &child : current_dir_->children_)
    {
        if (child != item_being_edited && child->display_name_.compare(new_name, Qt::CaseInsensitive) == 0)
        {
            QMessageBox::warning(nullptr, "重命名失败", "名称已存在！");
            return false;
        }
    }

    current_dir_->children_[index.row()]->display_name_ = new_name;
    emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole, Qt::ToolTipRole});
    return true;
}

void file_model::set_current_dir(const std::shared_ptr<file_item> &dir)
{
    beginResetModel();
    current_dir_ = dir;
    endResetModel();
}

std::shared_ptr<file_item> file_model::item_at(int row) const
{
    if (!current_dir_ || row < 0 || row >= current_dir_->children_.size())
    {
        return nullptr;
    }
    return current_dir_->children_[row];
}

bool file_model::add_folder(const QString &displayName, std::shared_ptr<file_item> &folder_out)
{
    if (!current_dir_ || name_exists(displayName, file_item_type::Folder))
    {
        return false;
    }
    folder_out = std::make_shared<file_item>(displayName, file_item_type::Folder);
    folder_out->parent_ = current_dir_;
    folder_out->last_modified_ = QDateTime::currentDateTime();

    beginInsertRows(QModelIndex(), static_cast<int>(current_dir_->children_.size()), static_cast<int>(current_dir_->children_.size()));
    current_dir_->children_.push_back(folder_out);
    endInsertRows();
    return true;
}

bool file_model::name_exists(const QString &displayName, file_item_type type) const
{
    if (!current_dir_)
    {
        return false;
    }
    for (const auto &child : current_dir_->children_)    // NOLINT
    {
        if (child->type_ != type)
        {
            continue;
        }
        if (child->display_name_.compare(displayName, Qt::CaseInsensitive) != 0)
        {
            continue;
        }
        return true;
    }
    return false;
}

bool file_model::add_file_from_path(const QString &file_path)
{
    if (!current_dir_)
    {
        return false;
    }
    QFileInfo fileInfo(file_path);
    QString displayName = fileInfo.fileName();

    if (name_exists(displayName, file_item_type::File))
    {
        QMessageBox::warning(nullptr, "添加失败", "同名文件已存在: " + displayName);
        return false;
    }

    auto new_item = std::make_shared<file_item>(displayName, file_item_type::File);
    new_item->file_size_ = fileInfo.size();
    new_item->last_modified_ = fileInfo.lastModified();
    new_item->parent_ = current_dir_;

    beginInsertRows(QModelIndex(), static_cast<int>(current_dir_->children_.size()), static_cast<int>(current_dir_->children_.size()));
    current_dir_->children_.push_back(new_item);
    endInsertRows();

    return true;
}
