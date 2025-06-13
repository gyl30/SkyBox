#ifndef LEAF_GUI_UPLOAD_TASK_MODEL_H
#define LEAF_GUI_UPLOAD_TASK_MODEL_H

#include <QVector>
#include <QMetaType>
#include <QAbstractListModel>
#include "file/event.h"

class upload_task_model : public QAbstractListModel
{
    Q_OBJECT

   public:
    explicit upload_task_model(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;

   public slots:
    void add_task(const leaf::upload_event &e);
    void update_task(const leaf::upload_event &e);
    void remove_task(const leaf::upload_event &e);

   private:
    [[nodiscard]] int find_task_index(const std::string &filename) const;

    QVector<leaf::upload_event> tasks_;
};

#endif
