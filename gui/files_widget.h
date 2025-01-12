#ifndef LEAF_GUI_FILES_WIDGET_H
#define LEAF_GUI_FILES_WIDGET_H

#include <QWidget>
#include <QAction>
#include <QTableWidget>
#include <QContextMenuEvent>

#include <map>
#include <vector>

namespace leaf
{
struct gfile
{
    std::string parent;
    std::string filename;
    std::string type;
};
class files_widget : public QWidget
{
   public:
    explicit files_widget(QWidget *parent);
    ~files_widget();

   public:
    void add_gfile(const gfile &gfile);
    void add_gfiles(const std::vector<gfile> &gfiles);
    void update();

   public:
    void contextMenuEvent(QContextMenuEvent *event) override;

   public:
    void on_new_file_clicked();
    void on_new_directory_clicked();

   private:
    std::string current_path_;
    QAction *rename_action_;
    QAction *new_directory_action_;
    QTableWidget *table_ = nullptr;
    std::map<std::string, std::vector<gfile>> gfiles_;
};
}    // namespace leaf

#endif
