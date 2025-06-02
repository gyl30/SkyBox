#include "file_model.h"
#include <QPainter>
#include <QPixmap>
#include <QIcon>
#include <QFont>
#include <QFileInfo>
#include <QLocale> // 用于格式化文件大小

// 假设这个函数定义在某处，如果不在，你需要添加它
// 或者移动到 file_model 类内部作为私有辅助方法
// 我将它放在这里作为静态辅助函数，你也可以移到 file_model 类中
// 确保之前定义的 formatFileSize 在这里可用
namespace Utils { // 或者放在 file_model 的私有部分
    static QString formatFileSize(qint64 bytes) {
        if (bytes < 0) return QStringLiteral("未知大小");
        QLocale locale(QLocale::English); // 使用特定locale保证点号作为小数分隔符
        if (bytes < 1024) return locale.toString(bytes) + QStringLiteral(" B");
        double kb = bytes / 1024.0;
        if (kb < 1024) return locale.toString(kb, 'f', 1) + QStringLiteral(" KB");
        double mb = kb / 1024.0;
        if (mb < 1024) return locale.toString(mb, 'f', 1) + QStringLiteral(" MB");
        double gb = mb / 1024.0;
        return locale.toString(gb, 'f', 1) + QStringLiteral(" GB");
    }
}


static QIcon emojiIcon(const QString &emoji)
{
    QPixmap pix(64, 64);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    QFont font("EmojiOne"); // 确保这个字体存在
    if (!QFontInfo(font).exactMatch()) { // 字体回退
        font.setFamily("Noto Color Emoji"); // 尝试另一个常见的Emoji字体
         if (!QFontInfo(font).exactMatch()) {
            font.setFamily(QFontInfo(p.font()).family()); // 使用系统默认字体
         }
    }
    font.setPointSize(32);
    p.setFont(font);
    p.drawText(pix.rect(), Qt::AlignCenter, emoji);
    return QIcon(pix);
}

file_model::file_model(QObject *parent) : QAbstractListModel(parent) {}

int file_model::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || !current_dir_)
    {
        return 0;
    }
    return current_dir_->children_.size();
}

QVariant file_model::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !current_dir_ || index.row() >= current_dir_->children_.size())
    {
        return QVariant();
    }
    const std::shared_ptr<file_item> &item = current_dir_->children_[index.row()];

    if (role == Qt::DisplayRole)
    {
        return item->display_name_; // 使用 display_name_
    }
    if (role == Qt::DecorationRole)
    {
        if (item->type_ == file_item_type::Folder)
        {
            return emojiIcon("📁");
        }
        else
        {
            return emojiIcon("📄");
        }
    }
    if (role == Qt::ToolTipRole) // 新增: 处理ToolTip
    {
        if (item->type_ == file_item_type::Folder)
        {
            return QString("文件夹\n包含 %1 个项目").arg(item->children_.size());
        }
        else
        {
            QString tooltip = QString("文件: %1\n大小: %2\n修改日期: %3")
                                  .arg(item->display_name_)
                                  .arg(Utils::formatFileSize(item->file_size_)) // 使用辅助函数
                                  .arg(item->last_modified_.toString("yyyy-MM-dd hh:mm:ss"));
            return tooltip;
        }
    }
    return QVariant();
}

Qt::ItemFlags file_model::flags(const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return Qt::NoItemFlags;
    }
    // 允许编辑以进行重命名
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
}

bool file_model::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || role != Qt::EditRole || !current_dir_ || index.row() >= current_dir_->children_.size())
    {
        return false;
    }
    QString new_name = value.toString().trimmed();
    if (new_name.isEmpty()) {
        // (可选) 提示用户名称不能为空，或者还原
        return false;
    }

    // 检查同级目录下 displayName 是否已存在 (不区分大小写通常更好)
    // (不包括正在编辑的项自身)
    const auto& item_being_edited = current_dir_->children_[index.row()];
    for (const auto& child : current_dir_->children_) {
        if (child != item_being_edited && child->display_name_.compare(new_name, Qt::CaseInsensitive) == 0) {
            // (可选) 弹出消息提示重名
            // QMessageBox::warning(nullptr, "重命名失败", "该名称已存在！");
            return false; // 名称已存在
        }
    }

    current_dir_->children_[index.row()]->display_name_ = new_name;
    // storage_name 会通过 get_storage_name() 自动更新
    emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole, Qt::ToolTipRole}); // ToolTipRole也可能改变
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

// 在 add_folder 和 (未来可能的) add_file 中，需要确保 displayName 唯一
// 并且在创建 file_item 时，其 file_size_ 和 last_modified_ 被正确设置
bool file_model::add_folder(const QString &displayName, std::shared_ptr<file_item> &folder_out)
{
    if (!current_dir_ || name_exists(displayName, file_item_type::Folder)) // name_exists 应该检查 displayName
    {
        return false;
    }
    folder_out = std::make_shared<file_item>(displayName, file_item_type::Folder);
    folder_out->parent_ = current_dir_;
    // 文件夹的 last_modified_ 通常是创建或内容变更时间，这里简单处理
    folder_out->last_modified_ = QDateTime::currentDateTime();


    beginInsertRows(QModelIndex(), current_dir_->children_.size(), current_dir_->children_.size());
    current_dir_->children_.push_back(folder_out);
    endInsertRows();
    return true;
}

// name_exists 应该检查 displayName
bool file_model::name_exists(const QString &displayName, file_item_type type) const
{
    if (!current_dir_)
    {
        return false;
    }
    for (const auto &child : current_dir_->children_)
    {
        // 通常不区分大小写比较以获得更好的用户体验
        if (child->display_name_.compare(displayName, Qt::CaseInsensitive) == 0 && child->type_ == type)
        {
            return true;
        }
    }
    return false;
}

// (未来) 你会需要一个 add_file_item 函数
bool file_model::add_file_from_path(const QString& file_path) {
    if (!current_dir_) return false;
    QFileInfo fileInfo(file_path);
    QString displayName = fileInfo.fileName();

    // 检查 displayName 是否已存在于当前目录
    if (name_exists(displayName, file_item_type::File)) {
        // (可选) 自动重命名逻辑，例如 "file (1).txt"
        // 或者提示用户
        // QMessageBox::warning(nullptr, "添加失败", "同名文件已存在: " + displayName);
        return false;
    }

    auto new_item = std::make_shared<file_item>(displayName, file_item_type::File);
    new_item->file_size_ = fileInfo.size();
    new_item->last_modified_ = fileInfo.lastModified();
    new_item->parent_ = current_dir_;

    beginInsertRows(QModelIndex(), current_dir_->children_.size(), current_dir_->children_.size());
    current_dir_->children_.push_back(new_item);
    endInsertRows();

    return true;
}

