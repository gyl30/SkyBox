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
#include "gui/widget.h"
#include "gui/titlebar.h"
#include "file/file_item.h"
#include "file/event_manager.h"

Widget::Widget(std::string user, std::string password, std::string token, QWidget *parent)
    : QWidget(parent), token_(std::move(token)), user_(std::move(user)), password_(std::move(password))
{
    resize(1200, 800);
    qRegisterMetaType<leaf::notify_event>("leaf::notify_event");
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

    leaf::event_manager::instance().startup();

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

    setup_demo_data();

    current_dir_ = root_;
    model_->set_current_dir(current_dir_);

    update_breadcrumb();
}

void Widget::setup_demo_data()
{
    root_ = std::make_shared<leaf::file_item>();
    root_->storage_name = "root";
    root_->last_modified = QDateTime::currentSecsSinceEpoch();
    root_->display_name = "根目录";
    root_->type = leaf::file_item_type::Folder;
    auto folder_a = std::make_shared<leaf::file_item>();
    folder_a->storage_name = "文档";
    folder_a->display_name = "文档";
    folder_a->type = leaf::file_item_type::Folder;
    folder_a->parent = root_;
    folder_a->last_modified = QDateTime::currentSecsSinceEpoch();
    ;

    auto file_a1 = std::make_shared<leaf::file_item>();
    file_a1->display_name = "产品需求文档.docx";
    file_a1->storage_name = "产品需求文档.docx";
    file_a1->type = leaf::file_item_type::File;
    file_a1->parent = folder_a;
    file_a1->file_size = 1024L * 350;
    file_a1->last_modified = QDateTime::currentSecsSinceEpoch();

    auto file_a2 = std::make_shared<leaf::file_item>();
    file_a2->display_name = "会议纪要.txt";
    file_a2->storage_name = "会议纪要.txt";
    file_a2->type = leaf::file_item_type::File;
    file_a2->parent = folder_a;
    file_a2->file_size = 1024L * 2;
    file_a2->last_modified = QDateTime::currentSecsSinceEpoch();

    folder_a->children = {file_a1, file_a2};

    auto folder_b = std::make_shared<leaf::file_item>();
    folder_b->display_name = "图片收藏";
    folder_b->storage_name = "图片收藏";
    folder_b->type = leaf::file_item_type::Folder;
    folder_b->parent = root_;
    folder_b->last_modified = QDateTime::currentSecsSinceEpoch();

    auto file_b1 = std::make_shared<leaf::file_item>();
    file_b1->display_name = "风景照 (1).jpg";
    file_b1->storage_name = "风景照 (1).jpg";
    file_b1->type = leaf::file_item_type::File;
    file_b1->parent = folder_b;
    file_b1->file_size = 1024L * 1024 * 2;
    file_b1->last_modified = QDateTime::currentSecsSinceEpoch();

    folder_b->children = {file_b1};

    auto file_c = std::make_shared<leaf::file_item>();
    file_c->storage_name = "项目计划.pdf";
    file_c->display_name = "项目计划.pdf";
    file_c->type = leaf::file_item_type::File;
    file_c->parent = root_;
    file_c->file_size = 1024L * 780;
    file_c->last_modified = QDateTime::currentSecsSinceEpoch();

    root_->children = {folder_a, folder_b, file_c};
}

void Widget::setup_side_ui()
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

void Widget::setup_files_ui()
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
    view_->setEditTriggers(QAbstractItemView::EditKeyPressed | QAbstractItemView::SelectedClicked);
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
    loading_overlay_ = new QWidget(file_page_);
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
    file_page_->installEventFilter(this);
}

void Widget::setup_upload_ui() {}
void Widget::setup_connections()
{
    connect(btn_file_page_, &QPushButton::clicked, this, &Widget::show_file_page);
    connect(btn_upload_page_, &QPushButton::clicked, this, &Widget::show_upload_page);

    connect(new_folder_btn_, &QPushButton::clicked, this, &Widget::on_new_folder);
    connect(upload_file_btn_, &QPushButton::clicked, this, &Widget::on_upload_file);

    connect(view_, &QListView::doubleClicked, this, [this](const QModelIndex &index) { view_dobule_clicked(index); });
    connect(view_, &QListView::customContextMenuRequested, this, [this](const QPoint &pos) { view_custom_context_menu_requested(pos); });
}
void Widget::view_dobule_clicked(const QModelIndex &index)
{
    if (!index.isValid())
    {
        return;
    }
    auto item = model_->item_at(index.row());
    if (!item)
    {
        return;
    }

    if (item->type == leaf::file_item_type::Folder)
    {
        current_dir_ = item;
        model_->set_current_dir(current_dir_);
        update_breadcrumb();
    }
}

void Widget::view_custom_context_menu_requested(const QPoint &pos)
{
    QModelIndex index = view_->indexAt(pos);
    if (!index.isValid())
    {
        return;
    }

    std::shared_ptr<leaf::file_item> item = model_->item_at(index.row());
    if (!item)
    {
        return;
    }

    QMenu context_menu(view_);
    QAction *rename_action = context_menu.addAction("✏️ 重命名");
    QAction *delete_action = context_menu.addAction("🗑️ 删除");
    context_menu.addSeparator();
    QAction *properties_action = context_menu.addAction("ℹ️ 属性");

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
    else if (selected_action == properties_action)
    {
        auto last_modified = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(item->last_modified)).toString("yyyy-MM-dd hh:mm:ss");
        QMessageBox::information(this,
                                 "属性",
                                 QString("名称: %1\n类型: %2\n大小: %3\n最后修改: %4\n存储名 : %5")
                                     .arg(QString::fromStdString(item->display_name))
                                     .arg(item->type == leaf::file_item_type::Folder ? "文件夹" : "文件")
                                     .arg(item->file_size)
                                     .arg(last_modified)
                                     .arg(QString::fromStdString(item->display_name)));
    }
}
bool Widget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == file_page_ && event->type() == QEvent::Resize)
    {
        if (loading_overlay_->isVisible())
        {
            loading_overlay_->setGeometry(0, 0, file_page_->width(), file_page_->height());
        }
    }
    return QWidget::eventFilter(watched, event);
}

void Widget::show_file_page()
{
    stack_->setCurrentWidget(file_page_);
    btn_file_page_->setChecked(true);
    btn_upload_page_->setChecked(false);
    new_folder_btn_->setEnabled(true);
    upload_file_btn_->setEnabled(true);
}

void Widget::show_upload_page()
{
    stack_->setCurrentWidget(upload_list_widget_);
    btn_file_page_->setChecked(false);
    btn_upload_page_->setChecked(true);
}

void Widget::on_upload_file()
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
        fi.file_size = std::filesystem::file_size(fi.filename);
        fi.dir = current_dir_->storage_name;
        ff.push_back(fi);
    }
    file_client_->add_upload_files(ff);
}

void Widget::on_new_folder()
{
    QString folder_name_base = "新建文件夹";
    QString unique_name = folder_name_base;
    int count = 1;
    while (model_->name_exists(unique_name, leaf::file_item_type::Folder))
    {
        unique_name = QString("%1 (%2)").arg(folder_name_base).arg(count++);
    }

    std::shared_ptr<leaf::file_item> new_folder_item;
    if (!model_->add_folder(unique_name, new_folder_item))
    {
        QMessageBox::warning(this, "创建失败", "无法创建文件夹，可能名称不合法或已存在。");
        return;
    }
    int row_count = model_->rowCount(QModelIndex());
    if (row_count > 0)
    {
        QModelIndex new_index = model_->index(row_count - 1, 0);
        for (int i = 0; i < row_count; ++i)
        {
            if (model_->item_at(i) == new_folder_item)
            {
                new_index = model_->index(i, 0);
                break;
            }
        }
        if (new_index.isValid())
        {
            view_->setCurrentIndex(new_index);
            view_->edit(new_index);
        }
    }
}

void Widget::on_breadcrumb_clicked()
{
    QObject *sender_obj = sender();
    if (sender_obj == nullptr)
    {
        return;
    }

    bool ok;
    int idx = sender_obj->property("crumbIndex").toInt(&ok);

    if (ok && idx >= 0 && idx < breadcrumb_list_.size())
    {
        current_dir_ = breadcrumb_list_[idx];
        model_->set_current_dir(current_dir_);
        update_breadcrumb();
    }
}

void Widget::update_breadcrumb()
{
    clear_breadcrumb_layout();

    build_breadcrumb_path();

    if (breadcrumb_list_.isEmpty())
    {
        breadcrumb_layout_->addStretch();
        return;
    }

    const int MAX_VISIBLE = 5;
    const int START_COUNT = 1;
    const int END_COUNT = 2;
    bool truncate = breadcrumb_list_.size() > MAX_VISIBLE;

    QList<QWidget *> widgets;

    for (int i = 0; i < breadcrumb_list_.size(); ++i)
    {
        if (truncate && i >= START_COUNT && i < breadcrumb_list_.size() - END_COUNT)
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
void Widget::clear_breadcrumb_layout()
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

void Widget::build_breadcrumb_path()
{
    std::shared_ptr<leaf::file_item> dir = current_dir_;
    while (dir)
    {
        breadcrumb_list_.prepend(dir);
        dir = dir->parent.lock();
    }
}

QToolButton *Widget::create_breadcrumb_button(int index)
{
    auto *btn = new QToolButton(breadcrumb_widget_);
    auto &item = breadcrumb_list_[index];
    auto text = (index == 0 && item->display_name == "根目录") ? "🏠 " + item->display_name : item->display_name;

    btn->setText(QString::fromStdString(text));
    btn->setToolTip(QString::fromStdString(item->display_name));
    btn->setProperty("crumbIndex", index);
    btn->setAutoRaise(true);
    btn->setCursor(Qt::PointingHandCursor);

    if (index == breadcrumb_list_.size() - 1)
    {
        btn->setEnabled(false);
        QFont font = btn->font();
        font.setBold(true);
        btn->setFont(font);
        btn->setStyleSheet("QToolButton:disabled { color: black; }");
    }
    else
    {
        connect(btn, &QToolButton::clicked, this, &Widget::on_breadcrumb_clicked);
    }

    return btn;
}

QToolButton *Widget::create_ellipsis_button(int start_index)
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
        QAction *action = menu->addAction(QString::fromStdString(breadcrumb_list_[j]->display_name));
        action->setData(j);
        connect(action,
                &QAction::triggered,
                this,
                [this, action]()
                {
                    bool ok;
                    int idx = action->data().toInt(&ok);
                    if (ok && idx >= 0 && idx < breadcrumb_list_.size())
                    {
                        current_dir_ = breadcrumb_list_[idx];
                        model_->set_current_dir(current_dir_);
                        update_breadcrumb();
                    }
                });
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

void Widget::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
    {
        last_click_pos_ = e->globalPosition().toPoint() - frameGeometry().topLeft();
        e->accept();
    }
}
void Widget::mouseMoveEvent(QMouseEvent *e)
{
    if ((e->buttons() & Qt::LeftButton) != 0U)
    {
        move(e->globalPosition().toPoint() - last_click_pos_);
        e->accept();
    }
}
void Widget::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
    {
        hide();
    }
    QWidget::mouseDoubleClickEvent(e);
}
void Widget::startup()
{
    leaf::event_manager::instance().subscribe("error", [this](const std::any &data) { error_progress(data); });
    leaf::event_manager::instance().subscribe("notify", [this](const std::any &data) { notify_progress(data); });
    leaf::event_manager::instance().subscribe("upload", [this](const std::any &data) { upload_progress(data); });
    leaf::event_manager::instance().subscribe("cotrol", [this](const std::any &data) { cotrol_progress(data); });
    leaf::event_manager::instance().subscribe("download", [this](const std::any &data) { download_progress(data); });

    connect(this, &Widget::upload_notify_signal, this, &Widget::on_upload_notify);
    connect(this, &Widget::download_notify_signal, this, &Widget::on_download_notify);
    connect(this, &Widget::cotrol_notify_signal, this, &Widget::on_cotrol_notify);
    connect(this, &Widget::notify_event_signal, this, &Widget::on_notify_event);

    file_client_ = std::make_shared<leaf::file_transfer_client>("127.0.0.1", 8080, user_, password_, token_);
    file_client_->startup();
}

void Widget::error_progress(const std::any &data)
{
    auto e = std::any_cast<leaf::error_event>(data);
    LOG_ERROR("error {}", e.message);
}

void Widget::cotrol_progress(const std::any &data)
{
    //
    auto e = std::any_cast<leaf::cotrol_event>(data);
    emit cotrol_notify_signal(e);
    LOG_INFO("^^^ cotrol progress {}", e.token);
}

void Widget::download_progress(const std::any &data)
{
    auto e = std::any_cast<leaf::download_event>(data);
    emit download_notify_signal(e);
    LOG_DEBUG("--> download progress {} {} {}", e.filename, e.download_size, e.file_size);
}

void Widget::notify_progress(const std::any &data)
{
    auto e = std::any_cast<leaf::notify_event>(data);
    emit notify_event_signal(e);
}

void Widget::upload_progress(const std::any &data)
{
    auto e = std::any_cast<leaf::upload_event>(data);
    LOG_DEBUG("upload progress: file {}, progress {}", e.filename, e.upload_size);
    emit upload_notify_signal(e);
}
void Widget::on_upload_notify(const leaf::upload_event &e) { upload_list_widget_->add_task_to_view(e); }

void Widget::on_download_notify(const leaf::download_event &e) {}

void Widget::on_cotrol_notify(const leaf::cotrol_event &e) {}

void Widget::on_error_occurred(const QString &error_msg)
{
    LOG_ERROR("error {}", error_msg.toStdString());

    QMessageBox::critical(this, "错误", error_msg);

    if (file_client_)
    {
        file_client_->shutdown();
        file_client_.reset();
    }
}

void Widget::on_files(const std::vector<leaf::file_node> &files)
{
    auto find_parent = [this](const std::string &file) -> std::shared_ptr<leaf::file_item>
    {
        auto it = item_map_.find(file);
        if (it == item_map_.end())
        {
            return root_;
        }
        return it->second;
    };
    for (const auto &f : files)
    {
        LOG_DEBUG("on file file {} type {} parent {}", f.name, f.type, f.parent);
        auto type = f.type == "dir " ? leaf::file_item_type::Folder : leaf::file_item_type::File;
        auto item = std::make_shared<leaf::file_item>();
        item->display_name = f.name;
        item->storage_name = item->display_name;
        item->type = type;
        item->last_modified = QDateTime::currentSecsSinceEpoch();
        auto parent_item = find_parent(f.parent);
        item->parent = parent_item;
        parent_item->children.push_back(item);
    }
}

void Widget::on_notify_event(const leaf::notify_event &e)
{
    if (e.method == "files")
    {
        on_files(std::any_cast<std::vector<leaf::file_node>>(e.data));
    }
    if (e.method == "rename")
    {
        rename_notify(e);
    }
    if (e.method == "new_directory")
    {
        new_directory_notify(e);
    }
    if (e.method == "change_directory")
    {
        change_directory_notify(e);
    }
}

void Widget::change_directory_notify(const leaf::notify_event &e)
{
    if (file_client_)
    {
        auto dir = std::any_cast<std::string>(e.data);
        file_client_->change_current_dir(dir);
    }
}

void Widget::new_directory_notify(const leaf::notify_event &e)
{
    if (file_client_)
    {
        auto dir = std::any_cast<std::string>(e.data);
        file_client_->create_directory(dir);
    }
}

void Widget::rename_notify(const leaf::notify_event &e) {}

Widget::~Widget()
{
    if (file_client_ != nullptr)
    {
        file_client_->shutdown();
        file_client_.reset();
    }

    leaf::event_manager::instance().shutdown();
}

void Widget::on_new_file_clicked()
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
    f.dir = current_dir_->storage_name;
    file_client_->add_upload_file(std::move(f));
}
