#ifndef LEAF_GUI_FILE_MODEL_H
#define LEAF_GUI_FILE_MODEL_H

#include <memory>
#include <vector>
#include <string>
#include <optional>
#include <QAbstractListModel>
#include "file/file_item.h"

namespace leaf
{
class file_model : public QAbstractListModel
{
    Q_OBJECT

   public:
    explicit file_model(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    void set_files(const std::vector<leaf::file_item> &files);
    [[nodiscard]] std::optional<leaf::file_item> item_at(int row) const;
    bool add_folder(const QString &name, const leaf::file_item &folder);
    bool name_exists(const QString &name, file_item_type type) const;

   private:
    std::vector<leaf::file_item> files_;
};
}    // namespace leaf

#endif
