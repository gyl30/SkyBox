#ifndef LEAF_GUI_IP_EDIT_H
#define LEAF_GUI_IP_EDIT_H

#include <QLineEdit>

namespace leaf
{
class ip_edit : public QLineEdit
{
    Q_OBJECT

   public:
    explicit ip_edit(QWidget *parent = nullptr);
    ~ip_edit() override;

    void setText(const QString &ip);
    [[nodiscard]] QString text() const;

   protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

   private:
    int edit_index(QLineEdit *edit);
    void number_key_press(QLineEdit *edit, int key);
    void dot_key_press(QLineEdit *edit);
    void left_key_press(QLineEdit *edit);
    void right_key_press(QLineEdit *edit);
    void backspace_key_press(QLineEdit *edit);

   private:
    QLineEdit *line_edits_[4];
};

}    // namespace leaf

#endif
