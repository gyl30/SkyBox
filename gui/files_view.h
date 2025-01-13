#ifndef LEAF_GUI_FILES_WIDGET_H
#define LEAF_GUI_FILES_WIDGET_H

#include <QWidget>
#include <QAction>
#include <QTableView>
#include <QContextMenuEvent>

#include <map>
#include <vector>

#include "gui/gfile.h"

namespace leaf
{
class files_view : public QTableView
{
   public:
    explicit files_view(QWidget *parent);
    ~files_view();

   public:
    void add_gfile(const gfile &gfile);
    void add_gfiles(const std::vector<gfile> &gfiles);

   public:
    void contextMenuEvent(QContextMenuEvent *event) override;

   public:
    void on_new_file_clicked();
    void on_new_directory_clicked();

   private:
    std::string current_path_;
    QAction *rename_action_;
    QAction *new_directory_action_;
    std::map<std::string, std::vector<gfile>> gfiles_;
};
}    // namespace leaf

#endif
