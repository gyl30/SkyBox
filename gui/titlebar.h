#ifndef LEAF_GUI_TITLEBAR_H
#define LEAF_GUI_TITLEBAR_H

#include <QWidget>
#include <QMouseEvent>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>

namespace leaf
{
class title_bar : public QWidget
{
    Q_OBJECT

   public:
    explicit title_bar(QWidget *parent = nullptr);

   signals:
    void minimizeClicked();
    void closeClicked();

   protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

   private:
    QPoint drag_position_;
    QLabel *title_label_;
    QPushButton *min_btn_;
    QPushButton *close_btn_;
};
}    // namespace leaf

#endif
