#ifndef LEAF_GUI_UPLOAD_TASK_MODEL_H
#define LEAF_GUI_UPLOAD_TASK_MODEL_H

#include <QVector>
#include <QMetaType>
#include <QAbstractListModel>
#include "file/event.h"

Q_DECLARE_METATYPE(leaf::upload_event)

class upload_task_model : public QAbstractListModel
{
    Q_OBJECT

   public:
    explicit upload_task_model(QObject *parent = nullptr);

    [[nodiscard]] explicit upload_task_model(QVector<leaf::upload_event> tasks) : tasks_(std::move(tasks)) {}
    [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;

   public slots:
    void add_task(const leaf::upload_event &e);
    void update_task(const leaf::upload_event &e);
    void remove_task(const leaf::upload_event &e);

   private:
    QVector<leaf::upload_event> tasks_;
    [[nodiscard]] int find_task_index(const std::string &filename) const;
};

#endif
