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
#include "gui/file_item_widget.h"
#include "gui/file_item_delegate.h"

file_item_delegate::file_item_delegate(QObject *parent) : QStyledItemDelegate(parent) {}

QSize file_item_delegate::sizeHint(const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const { return {800, 60}; }

QWidget *file_item_delegate::createEditor(QWidget *parent, const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const
{
    auto *editor = new file_item_widget(parent);

    connect(editor->get_action_button(),
            &QPushButton::clicked,
            [this, editor]()
            {
                auto editorIndex = editor->property("model_index").value<QModelIndex>();
                if (editorIndex.isValid())
                {
                    emit pause_button_clicked(editorIndex);
                }
            });

    connect(editor->get_cancel_button(),
            &QPushButton::clicked,
            [this, editor]()
            {
                auto editorIndex = editor->property("model_index").value<QModelIndex>();
                if (editorIndex.isValid())
                {
                    emit cancel_button_clicked(editorIndex);
                }
            });

    return editor;
}

void file_item_delegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    auto *item_widget = qobject_cast<file_item_widget *>(editor);
    if (item_widget == nullptr)
    {
        return;
    }

    item_widget->setProperty("model_index", QVariant::fromValue(index));

    auto task = index.data(static_cast<int>(leaf::task_role::kFullEventRole)).value<leaf::file_event>();

    item_widget->set_data(task);

    item_widget->get_action_button()->setIcon(leaf::emoji_to_icon("⏸️", 64));
    item_widget->get_cancel_button()->setIcon(leaf::emoji_to_icon("❌", 64));

    auto *progress_bar = item_widget->findChild<QProgressBar *>();
    if (progress_bar != nullptr)
    {
        progress_bar->setStyleSheet(
            "QProgressBar { border: none; background-color: #e9eef8; color: white; text-align: center; }"
            "QProgressBar::chunk { background-color: #5094f3; }");
    }
}

void file_item_delegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex & /*index*/) const
{
    editor->setGeometry(option.rect);
}

void file_item_delegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return;
    }

    painter->save();

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

    painter->restore();
}
