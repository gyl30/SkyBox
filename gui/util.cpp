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
QString format_time(int total_seconds)
{
    if (total_seconds < 0)
    {
        return "--:--";
    }
    int hours = total_seconds / 3600;
    int minutes = (total_seconds % 3600) / 60;
    int seconds = total_seconds % 60;
    if (hours > 0)
    {
        return QString("%1:%2:%3").arg(hours, 2, 10, QChar('0')).arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
    }

    return QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
}
}    // namespace leaf
