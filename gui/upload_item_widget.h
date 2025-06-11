#ifndef LEAF_GUI_UPLOAD_ITEM_WIDGET_H
#define LEAF_GUI_UPLOAD_ITEM_WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include "file/event.h"

class upload_item_widget : public QWidget
{
    Q_OBJECT

   public:
    explicit upload_item_widget(leaf::upload_event e, QWidget *parent = nullptr);

   public:
    void update(const leaf::upload_event &e);
    void pause();
    void cacnel();
    [[nodiscard]] leaf::upload_event ev() const { return event_; };

    QSize sizeHint() const override;

   private:
    void setup_ui();

   private slots:
    void handle_action_button_click();
    void handle_cancel_button_click();

   private:
    leaf::upload_event event_;

    QLabel *filename_label_ = nullptr;
    QProgressBar *progress_bar_ = nullptr;
    QPushButton *action_button_ = nullptr;
    QPushButton *cancel_button_ = nullptr;
};

#endif    // UPLOAD_ITEM_WIDGET_H
