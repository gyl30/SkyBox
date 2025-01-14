#include "gui/files_model.h"

static const int kColumnCount = 5;

namespace leaf
{

files_model::files_model(QObject *parent) : QAbstractTableModel(parent) {}

void files_model::add_or_update_file(const leaf::gfile &file)
{
    bool update = false;
    beginResetModel();
    for (auto &f : files_)
    {
        if (f.filename == file.filename && f.type == file.type && f.parent == file.parent)
        {
            f = file;
            update = true;
            break;
        }
    }
    if (!update)
    {
        files_.push_back(file);
    }
    endResetModel();
}

void files_model::delete_file(const leaf::gfile &file)
{
    beginResetModel();
    files_.erase(
        std::remove_if(files_.begin(),
                       files_.end(),
                       [&file](const leaf::gfile &f)
                       { return f.filename == file.filename && f.type == file.type && f.parent == file.parent; }),
        files_.end());
    endResetModel();
}

int files_model::rowCount(const QModelIndex & /*parent*/) const
{
    uint32_t row = files_.size() / kColumnCount;
    if (files_.size() % kColumnCount != 0)
    {
        row++;
    }
    return static_cast<int>(row);
}

int files_model::columnCount(const QModelIndex & /*parent*/) const { return kColumnCount; }

QVariant files_model::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical)
    {
        return {};
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

QVariant files_model::display_data(const QModelIndex &index) const
{
    if (!index.isValid() || index.column() >= columnCount(index) || index.row() >= rowCount(index))
    {
        return {};
    }
    const size_t row = index.row();
    const size_t column = index.column();
    if (row >= files_.size())
    {
        return {};
    }
    const auto &t = files_[(row * kColumnCount) + column];
    return QString::fromStdString(t.filename);
}

QVariant files_model::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        return display_data(index);
    }
    return {};
}

}    // namespace leaf
