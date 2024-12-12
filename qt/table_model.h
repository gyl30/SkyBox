#ifndef LEAF_TABLE_MODEL_H
#define LEAF_TABLE_MODEL_H

#include <QTimer>
#include <QObject>
#include <QVariant>
#include <QAbstractTableModel>
#include "qt/task.h"

namespace leaf
{
class task_model : public QAbstractTableModel
{
   public:
    explicit task_model(QObject *parent = nullptr);

   public:
    void add_or_update_task(const leaf::task &task);
    void delete_task(const leaf::task &task);

   public:
    [[nodiscard]] int rowCount(const QModelIndex & /*parent*/) const override;

    [[nodiscard]] int columnCount(const QModelIndex & /*parent*/) const override;

    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    [[nodiscard]] static QVariant set_header_data(int section, int role);
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;

   private:
    QTimer *timer_ = nullptr;
    std::vector<leaf::task> tasks_;
};

}    // namespace leaf

#endif
