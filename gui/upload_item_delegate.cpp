#include <QStyle>
#include <QPainter>
#include <QFileInfo>
#include <QMouseEvent>
#include <QApplication>
#include <QAbstractItemView>
#include "gui/util.h"
#include "file/event.h"
#include "gui/upload_item_delegate.h"

static QRect get_cancel_button_rect(const QStyleOptionViewItem &option)
{
    const int button_size = 24;
    const int margin = 5;
    return {option.rect.right() - margin - button_size, option.rect.top() + ((option.rect.height() - button_size) / 2), button_size, button_size};
}

static QRect get_action_button_rect(const QStyleOptionViewItem &option)
{
    const int button_size = 24;
    const int spacing = 2;
    QRect cancel_rect = get_cancel_button_rect(option);
    return {cancel_rect.left() - spacing - button_size, cancel_rect.top(), button_size, button_size};
}
upload_item_delegate::upload_item_delegate(QObject *parent) : QStyledItemDelegate(parent) {}

QSize upload_item_delegate::sizeHint(const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const { return {400, 40}; }

void upload_item_delegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return;
    }

    painter->save();

    QStyle *style = (option.widget != nullptr) ? option.widget->style() : QApplication::style();

    auto task = index.data(static_cast<int>(leaf::task_role::kFullEventRole)).value<leaf::upload_event>();

    int percentage = (task.file_size > 0) ? static_cast<int>((task.upload_size * 100) / task.file_size) : 0;
    int progress_width = (option.rect.width() * percentage) / 100;
    QRect progress_rect = QRect(option.rect.left(), option.rect.top(), progress_width, option.rect.height());

    QColor progress_color = option.palette.highlight().color();
    progress_color.setHsv(progress_color.hsvHue(), static_cast<int>(progress_color.hsvSaturation() * 0.4), 235);

    QColor base_background_color;
    QColor text_color;

    if ((option.state & QStyle::State_Selected) != 0U)
    {
        base_background_color = option.palette.highlight().color();
        text_color = option.palette.highlightedText().color();
    }
    else
    {
        base_background_color =
            ((option.features & QStyleOptionViewItem::Alternate) != 0U) ? option.palette.alternateBase().color() : option.palette.base().color();
        text_color = option.palette.text().color();
    }
    painter->fillRect(option.rect, base_background_color);
    painter->fillRect(progress_rect, progress_color);

    if ((option.state & QStyle::State_Sunken) != 0U)
    {
        painter->fillRect(option.rect, QColor(0, 0, 0, 50));
    }

    QRect text_rect = option.rect.adjusted(8, 0, -60, 0);
    QString filename_text = QFileInfo(QString::fromStdString(task.filename)).fileName();
    painter->setPen(text_color);
    painter->drawText(
        text_rect, Qt::AlignLeft | Qt::AlignVCenter, QFontMetrics(option.font).elidedText(filename_text, Qt::ElideRight, text_rect.width()));

    QRect action_button_rect = get_action_button_rect(option);
    QRect cancel_button_rect = get_cancel_button_rect(option);

    const auto *view = qobject_cast<const QAbstractItemView *>(option.widget);
    if ((view != nullptr) && ((option.state & QStyle::State_MouseOver) != 0U))
    {
        QPoint mouse_pos = view->viewport()->mapFromGlobal(QCursor::pos());
        if (action_button_rect.contains(mouse_pos))
        {
            painter->fillRect(action_button_rect, QColor(255, 255, 255, 40));
        }
        if (cancel_button_rect.contains(mouse_pos))
        {
            painter->fillRect(cancel_button_rect, QColor(255, 255, 255, 40));
        }
    }

    leaf::emoji_to_icon("⏸️", 64).paint(painter, action_button_rect, Qt::AlignCenter);
    leaf::emoji_to_icon("❌", 64).paint(painter, cancel_button_rect, Qt::AlignCenter);

    if ((option.state & QStyle::State_HasFocus) != 0U)
    {
        QStyleOptionFocusRect focus_rect_option;
        focus_rect_option.rect = option.rect;
        focus_rect_option.state = option.state;
        focus_rect_option.palette = option.palette;
        style->drawPrimitive(QStyle::PE_FrameFocusRect, &focus_rect_option, painter);
    }

    painter->restore();
}

bool upload_item_delegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    const auto *view = qobject_cast<const QAbstractItemView *>(option.widget);
    if ((view != nullptr) && !view->hasMouseTracking())
    {
        const_cast<QAbstractItemView *>(view)->setMouseTracking(true);
    }

    if (event->type() == QEvent::MouseButtonRelease)
    {
        auto *mouse_event = static_cast<QMouseEvent *>(event);
        if (mouse_event->button() == Qt::LeftButton)
        {
            QRect action_button_rect = get_action_button_rect(option);
            if (action_button_rect.contains(mouse_event->pos()))
            {
                emit pause_button_clicked(index);
                return true;
            }

            QRect cancel_button_rect = get_cancel_button_rect(option);
            if (cancel_button_rect.contains(mouse_event->pos()))
            {
                emit cancel_button_clicked(index);
                return true;
            }
        }
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
