#include "file_item.h"

file_item::file_item(const QString &displayName, file_item_type type)
    : display_name_(displayName), type_(type), file_size_(0)
{
    if (type_ == file_item_type::File) {
        last_modified_ = QDateTime::currentDateTime();
    }
}

QString file_item::get_storage_name() const {
    return display_name_;
}

