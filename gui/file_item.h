#ifndef FILE_ITEM_H
#define FILE_ITEM_H

#include <memory>
#include <QString>
#include <QVector>
#include <QDateTime>    // 新增: 用于最后修改日期

enum class file_item_type : uint8_t
{
    Folder,
    File
};

struct file_item
{
    QString display_name;
    QString storage_name;
    file_item_type type;
    int64_t file_size = 0;
    QDateTime last_modified;
    std::weak_ptr<file_item> parent;
    QVector<std::shared_ptr<file_item>> children;
};

#endif    // FILE_ITEM_H
