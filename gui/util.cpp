#include <QPainter>
#include <QPixmap>
#include "gui/util.h"

namespace leaf
{
QIcon emoji_to_icon(const QString &emoji, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    QFont font("EmojiOne");
    font.setPointSizeF(size * 0.65);
    painter.setFont(font);
    painter.setPen(Qt::black);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, emoji);
    painter.end();
    return pixmap;
}
}    // namespace leaf
