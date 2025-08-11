#ifndef LEAF_GUI_SETTINGS_WINDOW_H
#define LEAF_GUI_SETTINGS_WINDOW_H

#include <QWidget>

class QLineEdit;
class QPushButton;

class settings_window : public QWidget
{
    Q_OBJECT

   public:
    explicit settings_window(QWidget *parent = nullptr);

   signals:
    void switch_to_other(QString ip, uint16_t port);

   private slots:
    void on_save_clicked();

   private:
    QLineEdit *ip_ = nullptr;
    QLineEdit *port_ = nullptr;
    QPushButton *save_btn_ = nullptr;
};

#endif
