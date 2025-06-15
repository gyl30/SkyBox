#ifndef LEAF_GUI_UPLOAD_ITEM_WIDGET_H
#define LEAF_GUI_UPLOAD_ITEM_WIDGET_H

#include <QWidget>
#include "file/event.h"

// 前向声明
class QLabel;
class QProgressBar;
class QPushButton;

class upload_item_widget : public QWidget
{
    Q_OBJECT

   public:
    explicit upload_item_widget(QWidget *parent = nullptr);
    void set_data(const leaf::upload_event &task);

    [[nodiscard]] QPushButton *get_action_button() const { return action_button_; }
    [[nodiscard]] QPushButton *get_cancel_button() const { return cancel_button_; }

   private:
    void setup_ui();

    QLabel *file_name_label_;
    QLabel *size_label_;
    QProgressBar *progress_bar_;
    QLabel *time_label_;
    QLabel *speed_label_;
    QPushButton *action_button_;
    QPushButton *cancel_button_;
};

#endif
