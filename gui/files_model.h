#ifndef LEAF_GUI_FILES_MODEL_H
#define LEAF_GUI_FILES_MODEL_H

#include <QTimer>
#include <QObject>
#include <QVariant>
#include <QAbstractTableModel>
#include "gui/gfile.h"

namespace leaf
{
class files_model : public QAbstractTableModel
{
   public:
    explicit files_model(QObject *parent = nullptr);

   public:
    void add_or_update_file(const leaf::gfile &file);
    void delete_file(const leaf::gfile &file);

   public:
    [[nodiscard]] int rowCount(const QModelIndex & /*parent*/) const override;
    [[nodiscard]] int columnCount(const QModelIndex & /*parent*/) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QVariant display_data(const QModelIndex &index, int role) const;

   private:
    std::vector<leaf::gfile> files_;
};
}    // namespace leaf
#endif
