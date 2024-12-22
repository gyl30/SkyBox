#ifndef LEAF_GUI_LOGIN_WIDGET_H
#define LEAF_GUI_LOGIN_WIDGET_H

#include <QWidget>
#include <QDialog>

namespace leaf
{

class login_dialog : public QDialog
{
    Q_OBJECT
   public:
    explicit login_dialog(QWidget *parent = nullptr);
    ~login_dialog() override = default;

   public:
   signals:
    void login_data(QString username, QString password);
};
}    // namespace leaf
#endif
