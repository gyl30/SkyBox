#include <QApplication>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMetaType>
#include <QTime>
#include <QMenu>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QStringList>
#include <QHeaderView>
#include <QLineEdit>
#include <QStyleFactory>
#include <QStyle>
#include <QPainter>
#include <QButtonGroup>
#include <QMap>
#include <QMessageBox>

#include <memory>
#include "log/log.h"
#include "gui/titlebar.h"
#include "file/file_item.h"
#include "gui/file_widget.h"
#include "file/event_manager.h"

file_widget::file_widget(std::string user, std::string password, std::string token, QWidget *parent)
    : QWidget(parent), token_(std::move(token)), user_(std::move(user)), password_(std::move(password))
{
    resize(1200, 800);
    qRegisterMetaType<leaf::notify_event>("leaf::notify_event");
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

    leaf::event_manager::instance().startup();

    path_manager_ = std::make_shared<leaf::path_manager>(std::make_shared<leaf::directory>("", "."));

    main_layout = new QVBoxLayout(this);
    stack_ = new QStackedWidget(this);
    file_page_ = new QWidget(stack_);
    upload_list_widget_ = new upload_list_widget(stack_);
    stack_->addWidget(file_page_);
    stack_->addWidget(upload_list_widget_);
    stack_->setCurrentWidget(file_page_);

    setup_side_ui();

    setup_files_ui();

    setup_connections();

    auto *title_bar = new leaf::title_bar(this);
    connect(title_bar, &leaf::title_bar::minimizeClicked, this, &QWidget::showMinimized);
    connect(title_bar, &leaf::title_bar::closeClicked, this, &QWidget::close);

    content_layout_ = new QHBoxLayout();
    content_layout_->addLayout(side_layout_);
    content_layout_->addWidget(stack_);
    content_layout_->setSpacing(8);
    content_layout_->setContentsMargins(0, 0, 0, 0);

    main_layout->addWidget(title_bar);
    main_layout->addLayout(content_layout_);
    main_layout->setContentsMargins(8, 0, 0, 0);

    update_breadcrumb();
}

void file_widget::setup_side_ui()
{
    side_layout_ = new QVBoxLayout();
    btn_file_page_ = new QPushButton("📁 我的文件");
    btn_file_page_->setFlat(true);
    btn_file_page_->setCheckable(true);
    btn_file_page_->setChecked(true);
    btn_file_page_->setFixedHeight(30);

    btn_upload_page_ = new QPushButton("⏳ 上传任务");
    btn_upload_page_->setFlat(true);
    btn_upload_page_->setCheckable(true);
    btn_upload_page_->setFixedHeight(30);

    side_layout_->addWidget(btn_file_page_);
    side_layout_->addWidget(btn_upload_page_);
}

void file_widget::setup_files_ui()
{
    new_folder_btn_ = new QPushButton("📁 新建文件夹");
    new_folder_btn_->setFixedHeight(30);
    upload_file_btn_ = new QPushButton("⏫ 上传文件");
    upload_file_btn_->setFixedHeight(30);

    auto *file_page_layout = new QVBoxLayout(file_page_);
    file_page_layout->setContentsMargins(0, 0, 0, 0);
    file_page_layout->setSpacing(0);

    auto *breadcrumb_container = new QWidget(file_page_);
    breadcrumb_container->setFixedHeight(40);
    breadcrumb_container->setStyleSheet("QWidget { background-color: #fafafa; border-bottom: 1px solid #e0e0e0; }");
    auto *breadcrumb_container_layout = new QHBoxLayout(breadcrumb_container);
    breadcrumb_container_layout->setContentsMargins(10, 0, 10, 0);
    breadcrumb_widget_ = new QWidget(breadcrumb_container);
    breadcrumb_layout_ = new QHBoxLayout(breadcrumb_widget_);
    breadcrumb_layout_->setContentsMargins(0, 0, 10, 0);
    breadcrumb_layout_->setSpacing(0);
    breadcrumb_container_layout->addWidget(breadcrumb_widget_);
    breadcrumb_container_layout->addStretch();
    breadcrumb_container_layout->addWidget(new_folder_btn_);
    breadcrumb_container_layout->setSpacing(5);
    breadcrumb_container_layout->addWidget(upload_file_btn_);
    file_page_layout->addWidget(breadcrumb_container);

    view_ = new QListView(file_page_);
    view_->setViewMode(QListView::IconMode);
    view_->setIconSize(QSize(56, 56));
    view_->setGridSize(QSize(110, 100));
    view_->setSpacing(15);
    view_->setUniformItemSizes(true);
    view_->setSelectionMode(QAbstractItemView::SingleSelection);
    view_->setContextMenuPolicy(Qt::CustomContextMenu);
    view_->setEditTriggers(QAbstractItemView::EditKeyPressed);

    view_->setWordWrap(true);
    view_->setContentsMargins(10, 10, 10, 10);

    view_->setStyleSheet(
        "QListView {"
        "    background-color: white;"
        "    outline: 0;"
        "    border: none;"
        "}"
        "QListView::item {"
        "    background-color: transparent;"
        "    color: #212121;"
        "    padding: 4px;"
        "    border-radius: 3px;"
        "    text-align: center;"
        "}"
        "QListView::item:selected {"
        "    background-color: #e3f2fd;"
        "    color: #0d47a1;"
        "}"
        "QListView::item:hover {"
        "    background-color: #f5f5f5;"
        "}"
        "QListView::item:selected:hover {"
        "    background-color: #bbdefb;"
        "}"
        "QListView QScrollBar:vertical { border: none; background: transparent; width: 0px; margin: 0px; }"
        "QListView QScrollBar::handle:vertical { background: transparent; min-height: 0px; }"
        "QListView QScrollBar::add-line:vertical, QListView QScrollBar::sub-line:vertical { border: none; background: "
        "transparent; height: 0px; }"
        "QListView QScrollBar::up-arrow:vertical, QListView QScrollBar::down-arrow:vertical { background: transparent; "
        "}"
        "QListView QScrollBar::add-page:vertical, QListView QScrollBar::sub-page:vertical { background: transparent; }"
        "QListView QScrollBar:horizontal { border: none; background: transparent; height: 0px; margin: 0px; }"
        "QListView QScrollBar::handle:horizontal { background: transparent; min-width: 0px; }"
        "QListView QScrollBar::add-line:horizontal, QListView QScrollBar::sub-line:horizontal { border: none; "
        "background: transparent; width: 0px; }"
        "QListView QScrollBar::left-arrow:horizontal, QListView QScrollBar::right-arrow:horizontal { background: "
        "transparent; }"
        "QListView QScrollBar::add-page:horizontal, QListView QScrollBar::sub-page:horizontal { background: "
        "transparent; }");
    view_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    view_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    model_ = new leaf::file_model(this);
    view_->setModel(model_);

    file_page_layout->addWidget(view_, 1);
    loading_overlay_ = new QWidget(view_);
    loading_overlay_->setObjectName("loadingOverlay");
    loading_overlay_->setStyleSheet(
        "#loadingOverlay {"
        "   background-color: rgba(255, 255, 255, 220);"
        "   border-radius: 5px;"
        "}");

    auto *overlay_v_layout = new QVBoxLayout(loading_overlay_);
    loading_label_ = new QLabel("🔄 加载中...", loading_overlay_);
    loading_label_->setAlignment(Qt::AlignCenter);
    QFont loading_font = loading_label_->font();
    loading_font.setPointSize(14);
    loading_font.setBold(true);
    loading_label_->setFont(loading_font);
    loading_label_->setStyleSheet("QLabel { color: #555555; background-color: transparent; }");
    overlay_v_layout->addStretch();
    overlay_v_layout->addWidget(loading_label_);
    overlay_v_layout->addStretch();
    loading_overlay_->hide();
    view_->installEventFilter(this);
}

void file_widget::setup_upload_ui() {}
void file_widget::setup_connections()
{
    connect(btn_file_page_, &QPushButton::clicked, this, &file_widget::show_file_page);
    connect(btn_upload_page_, &QPushButton::clicked, this, &file_widget::show_upload_page);

    connect(new_folder_btn_, &QPushButton::clicked, this, &file_widget::on_new_folder);
    connect(upload_file_btn_, &QPushButton::clicked, this, &file_widget::on_upload_file);

    connect(view_, &QListView::doubleClicked, this, [this](const QModelIndex &index) { view_double_clicked(index); });
    connect(view_, &QListView::customContextMenuRequested, this, [this](const QPoint &pos) { view_custom_context_menu_requested(pos); });

    connect(model_, &leaf::file_model::rename, this, &file_widget::on_rename);
}
void file_widget::view_double_clicked(const QModelIndex &index)
{
    if (loading_overlay_->isVisible())
    {
        return;
    }
    if (!index.isValid())
    {
        return;
    }
    LOG_DEBUG("click index {}", index.row());
    auto item = model_->item_at(index.row());
    if (!item)
    {
        return;
    }
    LOG_DEBUG("click index {} name {}", index.row(), item->display_name);
    if (item->type != leaf::file_item_type::Folder)
    {
        return;
    }
    if (path_manager_ == nullptr)
    {
        LOG_ERROR("path manager is null");
        return;
    }

    {
        auto current_dir = path_manager_->current_directory();
        LOG_DEBUG("enter directory {} {}", current_dir->path(), item->display_name);
        path_manager_->enter_directory(current_dir->path(), item->display_name);
    }
    auto current_dir = path_manager_->current_directory();
    auto it = directory_cache_.find(current_dir->path());
    if (it != directory_cache_.end())
    {
        auto cache_dir = it->second;
        current_dir->reset_files(cache_dir->files());
        current_dir->reset_dir(cache_dir->subdirectories());
        LOG_DEBUG("path {} files empty load cache {} files", current_dir->path(), current_dir->files().size());
        model_->set_files(current_dir->files());
    }
    else
    {
        model_->set_files({});
        if (loading_overlay_->isHidden())
        {
            loading_overlay_->resize(view_->size());
            view_->setEnabled(false);
            loading_overlay_->show();
        }
    }

    LOG_DEBUG("update current directory {}", current_dir->path());
    if (file_client_ != nullptr)
    {
        file_client_->change_current_dir(current_dir->path());
    }
    update_breadcrumb();
}

void file_widget::view_custom_context_menu_requested(const QPoint &pos)
{
    QModelIndex index = view_->indexAt(pos);
    if (!index.isValid())
    {
        return;
    }

    auto item = model_->item_at(index.row());
    if (!item)
    {
        return;
    }

    QMenu context_menu(view_);
    QAction *rename_action = context_menu.addAction("✏️ 重命名");
    QAction *delete_action = context_menu.addAction("🗑️ 删除");
    context_menu.addSeparator();

    QAction *selected_action = context_menu.exec(view_->viewport()->mapToGlobal(pos));

    if (selected_action == rename_action)
    {
        view_->edit(index);
    }
    else if (selected_action == delete_action)
    {
        if (QMessageBox::question(this,
                                  "确认删除",
                                  QString("确定要删除 \"%1\"吗？此操作不可恢复。").arg(QString::fromStdString(item->display_name)),
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No) == QMessageBox::Yes)
        {
            QMessageBox::information(this, "删除", "删除：" + QString::fromStdString(item->display_name));
        }
    }
}

void file_widget::on_rename(const QModelIndex &index, const QString &old_name, const QString &new_name)
{
    auto item_opt = model_->item_at(index.row());
    if (!item_opt)
    {
        return;
    }

    leaf::rename_request rq;
    rq.parent = path_manager_->current_directory()->path();
    rq.old_name = old_name.toStdString();
    rq.new_name = new_name.toStdString();
    rq.type = item_opt->type == leaf::file_item_type::Folder ? "dir" : "file";
    rq.token = token_;
    LOG_DEBUG("parent {} rename file from {} to {} type {}", rq.parent, rq.old_name, rq.new_name, rq.type);
    if (file_client_)
    {
        file_client_->rename(rq);
    }
}
bool file_widget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == view_ && event->type() == QEvent::Resize)
    {
        if (loading_overlay_->isVisible())
        {
            loading_overlay_->resize(view_->size());
        }
    }

    return QWidget::eventFilter(watched, event);
}

void file_widget::show_file_page()
{
    stack_->setCurrentWidget(file_page_);
    btn_file_page_->setChecked(true);
    btn_upload_page_->setChecked(false);
    new_folder_btn_->setEnabled(true);
    upload_file_btn_->setEnabled(true);
}

void file_widget::show_upload_page()
{
    stack_->setCurrentWidget(upload_list_widget_);
    btn_file_page_->setChecked(false);
    btn_upload_page_->setChecked(true);
}

void file_widget::on_upload_file()
{
    QStringList files = QFileDialog::getOpenFileNames(this, "选择要上传的文件", QDir::homePath());
    if (files.isEmpty())
    {
        return;
    }
    std::vector<leaf::file_info> ff;
    for (const auto &f : files)
    {
        leaf::file_info fi;
        fi.local_path = f.toStdString();
        fi.filename = std::filesystem::path(f.toStdString()).filename().string();
        fi.file_size = std::filesystem::file_size(fi.local_path);
        fi.dir = path_manager_->current_directory()->path();
        ff.push_back(fi);
    }
    file_client_->add_upload_files(ff);
}

void file_widget::on_new_folder()
{
    QString folder_name_base = "新建文件夹";
    int count = 1;
    QString unique_name = folder_name_base;
    auto current_dir = path_manager_->current_directory();

    auto new_dir = std::make_shared<leaf::directory>(current_dir->path(), unique_name.toStdString());
    while (true)
    {
        if (!current_dir->dir_exist(new_dir))
        {
            break;
        }
        unique_name = QString("%2_%1").arg(folder_name_base).arg(count++);
        new_dir = std::make_shared<leaf::directory>(current_dir->path(), unique_name.toStdString());
    }

    LOG_INFO("new dir parent {} dir {}", path_manager_->current_directory()->path(), unique_name.toStdString());
    leaf::file_item new_folder_item;
    new_folder_item.display_name = unique_name.toStdString();
    new_folder_item.storage_name = unique_name.toStdString();
    new_folder_item.type = leaf::file_item_type::Folder;
    new_folder_item.file_size = 0;
    path_manager_->current_directory()->add_dir(new_folder_item, new_dir);

    if (file_client_ != nullptr)
    {
        leaf::create_dir cd;
        cd.dir = unique_name.toStdString();
        cd.parent = path_manager_->current_directory()->path();
        cd.token = token_;
        file_client_->create_directory(cd);
    }
    model_->set_files(current_dir->files());
}

void file_widget::on_breadcrumb_clicked()
{
    QObject *sender_obj = sender();
    if (sender_obj == nullptr)
    {
        return;
    }

    navigate_to_breadcrumb(sender_obj);
}

void file_widget::navigate_to_breadcrumb(QObject *obj)
{
    bool ok;
    int idx = obj->property("crumbIndex").toInt(&ok);
    if (!ok)
    {
        return;
    }

    if (idx < 0)
    {
        return;
    }
    if (idx > static_cast<int>(breadcrumb_list_.size()))
    {
        return;
    }
    if (loading_overlay_->isVisible())
    {
        return;
    }
    path_manager_->navigate_to_breadcrumb(idx);
    auto files = path_manager_->current_directory()->files();
    if (files.empty())
    {
        files = directory_cache_[path_manager_->current_directory()->path()]->files();
    }
    LOG_DEBUG("breadcrumb clicked path {} file size {}", path_manager_->current_directory()->path(), files.size());
    model_->set_files(files);
    if (file_client_ != nullptr)
    {
        file_client_->change_current_dir(path_manager_->current_directory()->path());
    }
    update_breadcrumb();
}
void file_widget::update_breadcrumb()
{
    clear_breadcrumb_layout();

    build_breadcrumb_path();

    if (breadcrumb_list_.empty())
    {
        breadcrumb_layout_->addStretch();
        return;
    }

    const int MAX_VISIBLE = 5;
    const int START_COUNT = 1;
    const int END_COUNT = 2;
    bool truncate = breadcrumb_list_.size() > MAX_VISIBLE;

    QList<QWidget *> widgets;

    for (int i = 0; i < static_cast<int>(breadcrumb_list_.size()); ++i)
    {
        if (truncate && i >= START_COUNT && i < static_cast<int>(breadcrumb_list_.size()) - END_COUNT)
        {
            if (i == START_COUNT)
            {
                widgets.append(create_ellipsis_button(i));
            }
            continue;
        }
        widgets.append(create_breadcrumb_button(i));
    }

    for (int i = 0; i < widgets.size(); ++i)
    {
        breadcrumb_layout_->addWidget(widgets[i]);
        if (i < widgets.size() - 1)
        {
            auto *btn = qobject_cast<QToolButton *>(widgets[i]);
            if ((nullptr != btn) && btn->isEnabled())
            {
                auto *sep = new QLabel("❯", breadcrumb_widget_);
                sep->setStyleSheet("QLabel { margin: 0 3px; color: #757575; }");
                breadcrumb_layout_->addWidget(sep);
            }
        }
    }

    breadcrumb_layout_->addStretch();
}
void file_widget::clear_breadcrumb_layout()
{
    QLayoutItem *child;
    while ((child = breadcrumb_layout_->takeAt(0)) != nullptr)
    {
        if (child->widget() != nullptr)
        {
            child->widget()->deleteLater();
        }
        delete child;
    }
    breadcrumb_list_.clear();
}

void file_widget::build_breadcrumb_path() { breadcrumb_list_ = path_manager_->breadcrumb_paths(); }

QToolButton *file_widget::create_breadcrumb_button(int index)
{
    auto *btn = new QToolButton(breadcrumb_widget_);
    auto &item = breadcrumb_list_[index];
    auto text = (index == 0 && item->path() == ".") ? "🏠 " + std::string("根目录") : item->name();

    btn->setText(QString::fromStdString(text));
    btn->setToolTip(QString::fromStdString(item->name()));
    btn->setProperty("crumbIndex", index);
    btn->setAutoRaise(true);
    btn->setCursor(Qt::PointingHandCursor);

    if (index == static_cast<int>(breadcrumb_list_.size() - 1))
    {
        btn->setEnabled(false);
        QFont font = btn->font();
        font.setBold(true);
        btn->setFont(font);
        btn->setStyleSheet("QToolButton:disabled { color: black; }");
    }
    else
    {
        connect(btn, &QToolButton::clicked, this, &file_widget::on_breadcrumb_clicked);
    }

    return btn;
}

QToolButton *file_widget::create_ellipsis_button(int start_index)
{
    auto *btn = new QToolButton(breadcrumb_widget_);
    btn->setText("...");
    btn->setToolTip("更多路径");
    btn->setAutoRaise(true);
    btn->setCursor(Qt::PointingHandCursor);

    auto *menu = new QMenu(btn);
    menu->setStyleSheet(R"(
    QMenu {
        background-color: white;
        border: 1px solid #cccccc;
        padding: 2px;
        margin: 2px;
        border-radius: 4px;
    }
    QMenu::item {
        background-color: transparent;
        color: #333333;
        padding: 6px 24px;
        border: 1px solid transparent;
        font-size: 14px;
    }
    QMenu::item:selected {
        background-color: #f0f0f0;
        color: black;
        border: 1px solid #aaaaaa;
        border-radius: 3px;
    }
    QMenu::item:disabled {
        color: #999999;
        background-color: transparent;
    }
    QMenu::separator {
        height: 1px;
        background: #e0e0e0;
        margin: 4px 10px;
    }
    QScrollBar:vertical {
        border: none;
        background: #f0f0f0;
        width: 10px;
        margin: 0px 0px 0px 0px;
        border-radius: 5px;
    }
    QScrollBar::handle:vertical {
        background: #cccccc;
        min-height: 20px;
        border-radius: 5px;
    }
    QScrollBar::add-line:vertical,
    QScrollBar::sub-line:vertical {
        height: 0px;
    }
    QScrollBar::add-page:vertical,
    QScrollBar::sub-page:vertical {
        background: none;
    })");

    int end_index = static_cast<int>(breadcrumb_list_.size()) - 2;
    for (int j = start_index; j <= end_index; ++j)
    {
        QAction *action = menu->addAction(QString::fromStdString(breadcrumb_list_[j]->name()));
        action->setProperty("crumbIndex", j);
        connect(action, &QAction::triggered, this, [this, action]() { navigate_to_breadcrumb(action); });
    }

    btn->setMenu(menu);
    btn->setPopupMode(QToolButton::InstantPopup);
    btn->setStyleSheet(R"(
    QToolButton::menu-indicator {
        image: none;
        width: 0px;
        height: 0px;
    })");
    return btn;
}

void file_widget::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
    {
        last_click_pos_ = e->globalPosition().toPoint() - frameGeometry().topLeft();
        e->accept();
    }
}
void file_widget::mouseMoveEvent(QMouseEvent *e)
{
    if ((e->buttons() & Qt::LeftButton) != 0U)
    {
        move(e->globalPosition().toPoint() - last_click_pos_);
        e->accept();
    }
}
void file_widget::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
    {
        hide();
    }
    QWidget::mouseDoubleClickEvent(e);
}
void file_widget::startup(const QString &ip, uint16_t port)
{
    leaf::event_manager::instance().subscribe("error", [this](const std::any &data) { error_progress(data); });
    leaf::event_manager::instance().subscribe("notify", [this](const std::any &data) { notify_progress(data); });
    leaf::event_manager::instance().subscribe("upload", [this](const std::any &data) { upload_progress(data); });
    leaf::event_manager::instance().subscribe("cotrol", [this](const std::any &data) { cotrol_progress(data); });
    leaf::event_manager::instance().subscribe("download", [this](const std::any &data) { download_progress(data); });

    connect(this, &file_widget::upload_notify_signal, this, &file_widget::on_upload_notify);
    connect(this, &file_widget::download_notify_signal, this, &file_widget::on_download_notify);
    connect(this, &file_widget::cotrol_notify_signal, this, &file_widget::on_cotrol_notify);
    connect(this, &file_widget::notify_event_signal, this, &file_widget::on_notify_event);
    file_client_ = std::make_shared<leaf::file_transfer_client>(ip.toStdString(), port, user_, password_, token_);
    file_client_->startup();
    file_client_->change_current_dir(path_manager_->current_directory()->path());
}

void file_widget::error_progress(const std::any &data)
{
    auto e = std::any_cast<leaf::error_event>(data);
    LOG_ERROR("error {}", e.message);
}

void file_widget::cotrol_progress(const std::any &data)
{
    //
    auto e = std::any_cast<leaf::cotrol_event>(data);
    emit cotrol_notify_signal(e);
    LOG_INFO("^^^ cotrol progress {}", e.token);
}

void file_widget::download_progress(const std::any &data)
{
    auto e = std::any_cast<leaf::download_event>(data);
    emit download_notify_signal(e);
    LOG_DEBUG("--> download progress {} {} {}", e.filename, e.download_size, e.file_size);
}

void file_widget::notify_progress(const std::any &data)
{
    auto e = std::any_cast<leaf::notify_event>(data);
    emit notify_event_signal(e);
}

void file_widget::upload_progress(const std::any &data)
{
    auto e = std::any_cast<leaf::upload_event>(data);
    LOG_DEBUG("upload progress: file {} progress {}:{}", e.filename, e.file_size, e.upload_size);
    emit upload_notify_signal(e);
}
void file_widget::on_upload_notify(const leaf::upload_event &e)
{
    upload_list_widget_->add_task_to_view(e);
    if (e.file_size == e.upload_size && e.file_size == 0 && e.upload_size == 0)
    {
        if (file_client_ != nullptr)
        {
            auto current_dir = path_manager_->current_directory();
            file_client_->change_current_dir(current_dir->path());
        }
    }
}

void file_widget::on_download_notify(const leaf::download_event &e) {}

void file_widget::on_cotrol_notify(const leaf::cotrol_event &e) {}

void file_widget::on_error_occurred(const QString &error_msg)
{
    LOG_ERROR("error {}", error_msg.toStdString());

    QMessageBox::critical(this, "错误", error_msg);

    if (file_client_)
    {
        file_client_->shutdown();
        file_client_.reset();
    }
}

void file_widget::on_files(const leaf::files_response &files)
{
    auto current_dir = path_manager_->current_directory();
    LOG_INFO("files response dir {} size {} current_directory {}", files.dir, files.files.size(), current_dir->path());
    if (files.dir != current_dir->path())
    {
        LOG_ERROR("files response dir {} not match current directory {}", files.dir, current_dir->path());
        return;
    }
    if (loading_overlay_->isVisible())
    {
        loading_overlay_->hide();
        view_->setEnabled(true);
    }
    for (const auto &file : files.files)
    {
        LOG_INFO("files response dir {} file {} parent {} type {}", files.dir, file.name, file.parent, file.type);
    }

    current_dir->reset();

    for (const auto &f : files.files)
    {
        leaf::file_item item;
        item.display_name = f.name;
        item.storage_name = f.name;
        item.type = f.type == "dir" ? leaf::file_item_type::Folder : leaf::file_item_type::File;
        item.file_size = f.type == "dir" ? 0 : f.file_size;
        if (item.type == leaf::file_item_type::Folder)
        {
            current_dir->add_dir(item, std::make_shared<leaf::directory>(current_dir->path(), item.display_name));
        }
        if (item.type == leaf::file_item_type::File)
        {
            current_dir->add_file(item);
        }
    }
    directory_cache_[current_dir->path()] = current_dir;
    model_->set_files(current_dir->files());
}

void file_widget::on_notify_event(const leaf::notify_event &e)
{
    if (e.method == "files")
    {
        on_files(std::any_cast<leaf::files_response>(e.data));
    }
    if (e.method == "rename")
    {
        rename_notify(e);
    }
    if (e.method == "create_directory")
    {
        create_directory_notify(e);
    }
}

void file_widget::create_directory_notify(const leaf::notify_event &e)
{
    if (!e.error.empty())
    {
        LOG_ERROR("create directory error: {}", e.error);
        return;
    }
    auto dir = std::any_cast<leaf::create_dir>(e.data);
    auto current_dir = path_manager_->current_directory();
    if (current_dir->path() == dir.parent)
    {
        LOG_INFO("create directory success, current directory {} match {}", current_dir->path(), dir.parent);
    }
    else
    {
        LOG_ERROR("create directory failed, current directory {} not match {}", current_dir->path(), dir.parent);
    }
}
void file_widget::rename_notify(const leaf::notify_event &e) {}

file_widget::~file_widget()
{
    if (file_client_ != nullptr)
    {
        file_client_->shutdown();
        file_client_.reset();
    }

    leaf::event_manager::instance().shutdown();
}

void file_widget::on_new_file_clicked()
{
    auto filename = QFileDialog::getOpenFileName(this, "选择文件");
    if (filename.isEmpty())
    {
        return;
    }
    std::filesystem::path file_path(filename.toStdString());
    leaf::file_info f;
    f.local_path = std::filesystem::absolute(file_path).string();
    f.filename = file_path.filename().string();
    f.file_size = std::filesystem::file_size(f.filename);
    f.dir = path_manager_->current_directory()->path();
    file_client_->add_upload_file(std::move(f));
}
void file_widget::closeEvent(QCloseEvent *event)
{
    emit window_closed();

    QWidget::closeEvent(event);
}
