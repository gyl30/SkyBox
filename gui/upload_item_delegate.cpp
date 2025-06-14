#include <QStyle>
#include <QLocale>
#include <QPainter>
#include <QFileInfo>
#include <QMouseEvent>
#include <QPushButton>
#include <QStyleOption>
#include <QApplication>
#include <QAbstractItemView>
#include "gui/util.h"
#include "file/event.h"
#include "gui/upload_item_delegate.h"

upload_item_delegate::upload_item_delegate(QObject *parent) : QStyledItemDelegate(parent)
{
    item_widget_.setVisible(false);
    item_widget_.setAttribute(Qt::WA_TranslucentBackground);
}

QSize upload_item_delegate::sizeHint(const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const { return {800, 60}; }

void upload_item_delegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    if ((opt.state & QStyle::State_Selected) != 0U)
    {
        painter->fillRect(opt.rect, opt.palette.highlight().color());
        painter->fillRect(opt.rect, QColor(0, 0, 0, 30));
    }
    else
    {
        QColor background =
            ((opt.features & QStyleOptionViewItem::Alternate) != 0U) ? opt.palette.alternateBase().color() : opt.palette.base().color();
        painter->fillRect(opt.rect, background);
    }
    if ((opt.state & QStyle::State_Sunken) != 0U)
    {
        painter->fillRect(opt.rect, QColor(0, 0, 0, 60));
    }

    auto task = index.data(static_cast<int>(leaf::task_role::kFullEventRole)).value<leaf::upload_event>();
    item_widget_.set_data(task);

    QPalette p = item_widget_.palette();
    QColor icon_color;
    if ((opt.state & QStyle::State_Selected) != 0U)
    {
        QColor selected_text_color = opt.palette.highlightedText().color();
        p.setColor(QPalette::WindowText, selected_text_color);

        QColor secondary_selected_color = selected_text_color;
        secondary_selected_color.setAlpha(200);
        p.setColor(QPalette::Text, secondary_selected_color);
        icon_color = selected_text_color;
    }
    else
    {
        p.setColor(QPalette::WindowText, opt.palette.text().color());
        p.setColor(QPalette::Text, opt.palette.mid().color());
        icon_color = QColor("#5094f3");
    }
    item_widget_.setPalette(p);

    item_widget_.get_action_button()->setIcon(leaf::emoji_to_icon("⏸️", 64));
    item_widget_.get_cancel_button()->setIcon(leaf::emoji_to_icon("❌", 64));

    item_widget_.resize(opt.rect.size());
    painter->translate(opt.rect.topLeft());
    item_widget_.render(painter, QPoint(), QRegion(), QWidget::RenderFlags(QWidget::DrawChildren));
    painter->translate(-opt.rect.topLeft());

    if ((opt.state & QStyle::State_HasFocus) != 0U)
    {
        QStyleOptionFocusRect focus_rect_option;
        focus_rect_option.rect = opt.rect;
        QApplication::style()->drawPrimitive(QStyle::PE_FrameFocusRect, &focus_rect_option, painter);
    }

    painter->restore();
}

bool upload_item_delegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (event->type() == QEvent::MouseButtonRelease)
    {
        auto *mouse_event = static_cast<QMouseEvent *>(event);
        if (mouse_event->button() == Qt::LeftButton)
        {
            item_widget_.resize(option.rect.size());

            QPoint local_pos = mouse_event->pos() - option.rect.topLeft();

            if (item_widget_.get_action_button()->geometry().contains(local_pos))
            {
                emit pause_button_clicked(index);
                return true;
            }
            if (item_widget_.get_cancel_button()->geometry().contains(local_pos))
            {
                emit cancel_button_clicked(index);
                return true;
            }
        }
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
