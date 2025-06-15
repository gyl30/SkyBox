#include <QStyle>
#include <QLocale>
#include <QPainter>
#include <QFileInfo>
#include <QMouseEvent>
#include <QPushButton>
#include <QStyleOption>
#include <QApplication>
#include <QProgressBar>
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

    QColor background = ((opt.features & QStyleOptionViewItem::Alternate) != 0U) ? opt.palette.alternateBase().color() : opt.palette.base().color();
    painter->fillRect(opt.rect, background);

    if ((opt.state & QStyle::State_Selected) != 0U)
    {
        QColor selection_overlay = opt.palette.highlight().color();
        selection_overlay.setAlpha(40);
        painter->fillRect(opt.rect, selection_overlay);
    }

    if ((opt.state & QStyle::State_Sunken) != 0U)
    {
        QColor sunken_overlay = opt.palette.highlight().color();
        sunken_overlay.setAlpha(60);
        painter->fillRect(opt.rect, sunken_overlay);
    }

    auto task = index.data(static_cast<int>(leaf::task_role::kFullEventRole)).value<leaf::upload_event>();
    item_widget_.set_data(task);

    item_widget_.get_action_button()->setIcon(leaf::emoji_to_icon("⏸️", 64));
    item_widget_.get_cancel_button()->setIcon(leaf::emoji_to_icon("❌", 64));

    auto *progress_bar = item_widget_.findChild<QProgressBar *>();
    if (progress_bar != nullptr)
    {
        progress_bar->setStyleSheet(
            "QProgressBar { border: none; background-color: #e9eef8; color: white; text-align: center; }"
            "QProgressBar::chunk { background-color: #5094f3; }");
    }
    item_widget_.resize(opt.rect.size());
    painter->translate(opt.rect.topLeft());
    item_widget_.render(painter, QPoint(), QRegion(), QWidget::RenderFlags(QWidget::DrawChildren));
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
