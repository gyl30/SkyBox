#ifndef LEAF_GUI_LOGINWIDGET_H
#define LEAF_GUI_LOGINWIDGET_H

#include <QWidget>

class QLineEdit;

class LoginWidget : public QWidget
{
    Q_OBJECT
   public:
    explicit LoginWidget(QWidget *parent = nullptr);

   private slots:
    void handleLoginClicked();

   private:
    QLineEdit *user_ = nullptr;
    QLineEdit *pass_ = nullptr;
};

#endif
