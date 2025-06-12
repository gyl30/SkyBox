#include <QPainter>
#include <QApplication>
#include <QStyleOptionProgressBar>
#include <QStyleOptionButton>
#include <QFileInfo>
#include <QMouseEvent>

#include "gui/util.h"
#include "file/event.h"
#include "gui/upload_item_delegate.h"

static QRect get_cancel_button_rect(const QStyleOptionViewItem &option)
{
    const int button_size = 30;
    const int margin = 5;
    return {option.rect.right() - margin - button_size, option.rect.top() + ((option.rect.height() - button_size) / 2), button_size, button_size};
}

static QRect get_action_button_rect(const QStyleOptionViewItem &option)
{
    const int button_size = 30;
    const int spacing = 2;
    QRect cancel_rect = get_cancel_button_rect(option);
    return {cancel_rect.left() - spacing - button_size, cancel_rect.top(), button_size, button_size};
}
upload_item_delegate::upload_item_delegate(QObject *parent) : QStyledItemDelegate(parent) {}

QSize upload_item_delegate::sizeHint(const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const { return {400, 50}; }

void upload_item_delegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return;
    }

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    QStyle *style = (opt.widget != nullptr) ? opt.widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);

    auto task = index.data(static_cast<int>(leaf::task_role::kFullEventRole)).value<leaf::upload_event>();

    painter->save();

    QRect content_rect = option.rect.adjusted(5, 3, -5, -3);
    QRect cancel_button_rect = get_cancel_button_rect(option);
    QRect action_button_rect = get_action_button_rect(option);

    int right_bound = action_button_rect.left() - 8;
    QRect progress_rect(content_rect.left(), content_rect.top() + 22, right_bound - content_rect.left(), 12);
    QRect filename_rect(content_rect.left(), content_rect.top(), right_bound - content_rect.left(), 20);

    QString filename = QFileInfo(QString::fromStdString(task.filename)).fileName();
    QFontMetrics metrics(option.font);
    QString elided_text = metrics.elidedText(filename, Qt::ElideRight, filename_rect.width());
    painter->drawText(filename_rect, Qt::AlignLeft | Qt::AlignVCenter, elided_text);

    QStyleOptionProgressBar progress_bar_option;
    progress_bar_option.rect = progress_rect;
    progress_bar_option.minimum = 0;
    progress_bar_option.maximum = 100;
    progress_bar_option.progress = (task.file_size > 0) ? static_cast<int>((task.upload_size * 100) / task.file_size) : 0;
    progress_bar_option.text = QString::number(progress_bar_option.progress) + "%";
    progress_bar_option.textAlignment = Qt::AlignCenter;
    progress_bar_option.textVisible = true;
    style->drawControl(QStyle::CE_ProgressBar, &progress_bar_option, painter);

    QStyleOptionButton action_btn_option;
    action_btn_option.rect = action_button_rect;
    action_btn_option.state = QStyle::State_Enabled | QStyle::State_Raised;
    action_btn_option.icon = leaf::emoji_to_icon("⏸️", 64);
    action_btn_option.iconSize = QSize(16, 16);
    style->drawControl(QStyle::CE_PushButton, &action_btn_option, painter);

    QStyleOptionButton cancel_btn_option;
    cancel_btn_option.rect = cancel_button_rect;
    cancel_btn_option.state = QStyle::State_Enabled | QStyle::State_Raised;
    cancel_btn_option.icon = leaf::emoji_to_icon("❌", 64);
    cancel_btn_option.iconSize = QSize(16, 16);
    style->drawControl(QStyle::CE_PushButton, &cancel_btn_option, painter);

    painter->restore();
}

bool upload_item_delegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (event->type() == QEvent::MouseButtonRelease)
    {
        auto *mouse_event = static_cast<QMouseEvent *>(event);
        if (get_action_button_rect(option).contains(mouse_event->pos()))
        {
            emit action_button_clicked(index);
            return true;
        }
        if (get_cancel_button_rect(option).contains(mouse_event->pos()))
        {
            emit cancel_button_clicked(index);
            return true;
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
