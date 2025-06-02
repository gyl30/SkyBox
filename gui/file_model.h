#ifndef FILE_MODEL_H
#define FILE_MODEL_H

#include <QAbstractListModel>
#include <memory>
#include "file_item.h"

class file_model : public QAbstractListModel
{
    Q_OBJECT

   public:
    explicit file_model(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    void set_current_dir(const std::shared_ptr<file_item> &dir);
    std::shared_ptr<file_item> item_at(int row) const;
    bool add_folder(const QString &name, std::shared_ptr<file_item> &folder);
    bool name_exists(const QString &name, file_item_type type) const;
    bool add_file_from_path(const QString &file_path);    // <--- 新增声明

   private:
    std::shared_ptr<file_item> current_dir_;
};

#endif    // FILE_MODEL_H

