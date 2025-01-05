#ifndef LEAF_GUI_FILES_WIDGET_H
#define LEAF_GUI_FILES_WIDGET_H

#include <QWidget>
#include <QAction>
#include <QContextMenuEvent>

namespace leaf
{
class files_widget : public QWidget
{
   public:
    explicit files_widget(QWidget *parent);
    ~files_widget();

   public:
    void contextMenuEvent(QContextMenuEvent *event) override;

   public:
    void on_new_file_clicked();
    void on_new_directory_clicked();

   private:
    QAction *rename_action_;
    QAction *new_directory_action_;
};
}    // namespace leaf

#endif
