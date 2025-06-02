#ifndef FILE_ITEM_H
#define FILE_ITEM_H

#include <memory>
#include <QString>
#include <QVector>
#include <QDateTime>    // 新增: 用于最后修改日期

enum class file_item_type
{
    Folder,
    File
};

class file_item
{
   public:
    file_item(const QString &displayName, file_item_type type);

    QString display_name_;    // 用户看到的名称 (之前是 name_)
    // QString storage_name_; // 我们将通过 get_storage_name() 动态生成
    file_item_type type_;
    qint64 file_size_;
    QDateTime last_modified_;    // 新增: 文件的最后修改日期
    std::weak_ptr<file_item> parent_;
    QVector<std::shared_ptr<file_item>> children_;

    // 动态获取存储名 (例如Base64编码displayName)
    QString get_storage_name() const;
};

#endif    // FILE_ITEM_H

