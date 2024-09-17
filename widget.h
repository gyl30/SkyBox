#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QListView>
#include <QTimer>
#include <QTableView>
#include <QTableWidget>
#include <QMainWindow>

class Widget : public QWidget
{
    Q_OBJECT

   public:
    explicit Widget(QWidget* parent = nullptr);
    ~Widget() override;

   private:
    QTableView* list_view_ = nullptr;
    QTimer* timer_ = nullptr;
};
#endif    // WIDGET_H
