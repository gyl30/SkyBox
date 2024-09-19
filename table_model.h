#ifndef LEAF_TABLE_MODEL_H
#define LEAF_TABLE_MODEL_H

#include <vector>
#include <QTimer>
#include <QObject>
#include <QVariant>
#include <QAbstractTableModel>

#include "file_task.h"

namespace leaf
{
class task_model : public QAbstractTableModel
{
   public:
    explicit task_model(QObject *parent = nullptr);

   public:
    [[nodiscard]] int rowCount(const QModelIndex & /*parent*/) const override;

    [[nodiscard]] int columnCount(const QModelIndex & /*parent*/) const override;

    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    [[nodiscard]] static QVariant set_header_data(int section, int role);
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QVariant task_to_display_data(const QModelIndex &index) const;
    void add_task(const file_task::ptr &t);

   private:
    void start_timer();
    void on_timer();

   private:
    std::vector<file_task::ptr> tasks_;
    QTimer *timer_ = nullptr;
};

}    // namespace leaf

#endif
