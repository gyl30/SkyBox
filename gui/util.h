#ifndef LEAF_GUI_UTIL_H
#define LEAF_GUI_UTIL_H

#include <QIcon>
#include <QString>

namespace leaf
{
enum class task_role
{
    kFullEventRole = Qt::UserRole + 1,
};

QIcon emoji_to_icon(const QString &emoji, int size);
}    // namespace leaf

#endif
