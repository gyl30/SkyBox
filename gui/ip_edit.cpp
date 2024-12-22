#include <QPainter>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QRegExpValidator>
#else
#include <QRegularExpressionValidator>
#endif

#include "gui/ip_edit.h"

namespace leaf
{
QLabel *create_dot_label(QWidget *parent)
{
    auto *label = new QLabel(parent);
    label->setText(".");
    label->setFixedWidth(2);
    return label;
}
QLineEdit *create_line_edit(QWidget *parent)
{
    static QRegExp rx("(25[0-5]|2[0-4][0-9]|1?[0-9]{1,2})");
    auto *edit = new QLineEdit(parent);
    edit->setProperty("ip", true);
    edit->setFrame(false);
    edit->setMaxLength(3);
    edit->setAlignment(Qt::AlignCenter);
    edit->installEventFilter(parent);
    edit->setValidator(new QRegExpValidator(rx, parent));
    edit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    edit->setStyleSheet("background:transparent;border-width:0;border-style:outset");
    return edit;
}
ip_edit::ip_edit(QWidget *parent) : QLineEdit(parent)
{
    auto *layout = new QHBoxLayout(this);
    layout->setSpacing(2);
    layout->setContentsMargins(2, 2, 2, 2);
    for (int i = 0; i < 4; i++)
    {
        line_edits_[i] = create_line_edit(this);
        layout->addWidget(line_edits_[i]);
        if (i < 3)
        {
            layout->addWidget(create_dot_label(this));
        }
        connect(line_edits_[i], &QLineEdit::textChanged, this, &QLineEdit::textChanged);
    }
    this->setReadOnly(true);
    line_edits_[0]->setFocus();
    line_edits_[0]->selectAll();
}

ip_edit::~ip_edit() = default;

int ip_edit::edit_index(QLineEdit *edit)
{
    int index = 0;
    for (int i = 0; i < 4; i++)
    {
        if (edit == line_edits_[i])
        {
            index = i;
        }
    }
    return index;
}

// 事件过滤器，判断按键输入
bool ip_edit::eventFilter(QObject *obj, QEvent *ev)
{
    if (!children().contains(obj) || QEvent::KeyPress != ev->type())
    {
        return false;
    }
    auto *event = dynamic_cast<QKeyEvent *>(ev);
    auto *edit = qobject_cast<QLineEdit *>(obj);
    int key = event->key();
    if (key >= Qt::Key_0 && key <= Qt::Key_9)
    {
        number_key_press(edit, key);
        return QLineEdit::eventFilter(obj, ev);
    }
    if (key == Qt::Key_Backspace)
    {
        backspace_key_press(edit);
        return QLineEdit::eventFilter(obj, ev);
    }
    if (key == Qt::Key_Left)
    {
        left_key_press(edit);
        return QLineEdit::eventFilter(obj, ev);
    }
    if (key == Qt::Key_Right)
    {
        right_key_press(edit);
        return QLineEdit::eventFilter(obj, ev);
    }
    if (key == Qt::Key_Period)
    {
        dot_key_press(edit);
        return QLineEdit::eventFilter(obj, ev);
    }
    return false;
}
void ip_edit::dot_key_press(QLineEdit *edit)
{
    int index = edit_index(edit);
    if (index == 3)
    {
        return;
    }
    auto *next_edit = line_edits_[index + 1];
    next_edit->setFocus();
    next_edit->setCursorPosition(0);
}

void ip_edit::right_key_press(QLineEdit *edit)
{
    auto edit_text = edit->text();
    if (edit->cursorPosition() != edit_text.length())
    {
        return;
    }

    int index = edit_index(edit);
    if (index == 3)
    {
        return;
    }
    auto *next_edit = line_edits_[index + 1];
    next_edit->setFocus();
    next_edit->setCursorPosition(0);
}
void ip_edit::left_key_press(QLineEdit *edit)
{
    if (edit->cursorPosition() != 0)
    {
        return;
    }
    int index = edit_index(edit);
    if (index == 0)
    {
        return;
    }
    auto *next_edit = line_edits_[index - 1];
    next_edit->setFocus();
    int length = next_edit->text().length();
    next_edit->setCursorPosition(length);
}
void ip_edit::backspace_key_press(QLineEdit *edit)
{
    QString input_text = edit->text();
    if (!input_text.isEmpty())
    {
        return;
    }

    int index = edit_index(edit);
    if (index == 0)
    {
        return;
    }
    auto *next_edit = line_edits_[index - 1];

    next_edit->setFocus();
    int length = next_edit->text().length();
    next_edit->setCursorPosition(length);
}
void ip_edit::number_key_press(QLineEdit *edit, int key)
{
    QString select_text = edit->selectedText();
    if (!select_text.isEmpty())
    {
        edit->text().replace(select_text, QChar(key));
        return;
    }
    QString input_text = edit->text();
    if (input_text.length() <= 3 && input_text.toInt() * 10 > 255)
    {
        int index = edit_index(edit);
        if (index != 3)
        {
            line_edits_[index + 1]->setFocus();
            line_edits_[index + 1]->selectAll();
        }
        return;
    }
    if (input_text.length() == 2 && input_text.toInt() * 10 < 255)
    {
        if (Qt::Key_0 == key && input_text.toInt() != 0)
        {
            edit->setText(input_text.insert(edit->cursorPosition(), QChar(Qt::Key_0)));
        }
    }
}

QString ip_edit::text() const
{
    QStringList edit_texts;
    for (const auto *line_edit : line_edits_)
    {
        edit_texts.append(line_edit->text());
    }
    return edit_texts.join(".");
}

}    // namespace leaf
