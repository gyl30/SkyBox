#include <QMenu>
#include <QLabel>
#include <QIcon>
#include <QString>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileIconProvider>

#include <filesystem>

#include "log/log.h"
#include "gui/files_view.h"

namespace leaf
{
QWidget *create_file_widget(const gfile &file, QWidget *parent)
{
    std::filesystem::path p(file.filename);
    auto *label = new QLabel(QString::fromStdString(p.filename().string()), parent);
    QFileIconProvider iconProvider;
    QIcon dir_icon = iconProvider.icon(QFileIconProvider::Folder);
    QIcon file_icon = iconProvider.icon(QFileIconProvider::File);
    auto pixmap = file.type == "dir" ? dir_icon.pixmap(32, 32) : file_icon.pixmap(32, 32);
    auto *widget = new QWidget(parent);
    auto *layout = new QVBoxLayout(widget);
    auto *icon_label = new QLabel(widget);
    icon_label->setPixmap(pixmap);
    layout->addWidget(icon_label);
    layout->addWidget(label);
    widget->setLayout(layout);
    return widget;
}
files_view::files_view(QWidget *parent) : QTableView(parent)
{
    rename_action_ = new QAction("重命名", this);
    new_directory_action_ = new QAction("新建文件夹", this);

    connect(rename_action_, &QAction::triggered, this, &files_view::on_new_file_clicked);
    connect(new_directory_action_, &QAction::triggered, this, &files_view::on_new_directory_clicked);
}

files_view::~files_view()
{
    delete rename_action_;
    delete new_directory_action_;
}

void files_view::add_gfile(const gfile &gf)
{
    if (current_path_.empty())
    {
        current_path_ = gf.parent;
    }
    LOG_INFO("add gfile {}", gf.filename);
    auto &files = gfiles_[gf.parent];
    auto it =
        std::find_if(files.begin(), files.end(), [&gf](const leaf::gfile &f) { return f.filename == gf.filename; });
    if (it != files.end())
    {
        return;
    }
    files.push_back(gf);
}

void files_view::add_gfiles(const std::vector<gfile> &gfiles)
{
    for (const auto &gfile : gfiles)
    {
        add_gfile(gfile);
    }
}

void files_view::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    menu.addAction(rename_action_);
    menu.addAction(new_directory_action_);
    menu.exec(event->globalPos());
}

void files_view::on_new_file_clicked() { LOG_INFO("new file clicked"); }

void files_view::on_new_directory_clicked() { LOG_INFO("new directory clicked"); }

}    // namespace leaf
