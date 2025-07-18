#ifndef LEAF_FILE_DIRECTORY_H
#define LEAF_FILE_DIRECTORY_H

#include <utility>
#include <vector>
#include <string>
#include <memory>
#include "file/file_item.h"

namespace leaf
{
class directory
{
   public:
    explicit directory(std::string dir) : name_(std::move(dir)) {}

   public:
    void add_subdirectory(const std::shared_ptr<directory>& dir)
    {
        leaf::file_item item;
        item.type = leaf::file_item_type::Folder;
        item.display_name = dir->name();
        item.storage_name = dir->name();
        item.file_size = 0;
        files_.push_back(item);
        subdirectories_.push_back(dir);
    }
    void add_file(const leaf::file_item& file) { files_.push_back(file); }
    void reset_files(std::vector<leaf::file_item> files) { files_ = std::move(files); }
    bool dir_exist(const std::shared_ptr<directory>& dir)
    {
        for (const auto& sub_dir : subdirectories_)
        {
            if (sub_dir->name() == dir->name())
            {
                return true;
            }
        }
        return false;
    }
    bool file_exist(const leaf::file_item& file)
    {
        for (const auto& f : files_)
        {
            if (f.display_name == file.display_name)
            {
                return true;
            }
        }
        return false;
    }

   public:
    [[nodiscard]] const std::string& name() const { return name_; }
    [[nodiscard]] const std::vector<leaf::file_item>& files() const { return files_; }
    [[nodiscard]] const std::vector<std::shared_ptr<directory>>& subdirectories() const { return subdirectories_; }

   private:
    std::string name_;
    std::vector<leaf::file_item> files_;
    std::vector<std::shared_ptr<directory>> subdirectories_;
};

class path_manager
{
   public:
    explicit path_manager(const std::shared_ptr<directory>& root) : root_(root), current_directory_(root) {}

   public:
    bool enter_directory(const std::shared_ptr<directory>& dir)
    {
        const auto& subdirectories = current_directory_->subdirectories();
        for (const auto& sub_dir : subdirectories)    // NOLINT
        {
            if (sub_dir->name() == dir->name())
            {
                paths_.push_back(dir);
                current_directory_ = sub_dir;
                return true;
            }
        }
        return false;
    }

    bool exit_directory()
    {
        if (!paths_.empty())
        {
            current_directory_ = root_;
            for (const auto& dir : paths_)
            {
                for (const auto& sub_dir : current_directory_->subdirectories())
                {
                    if (sub_dir->name() == dir->name())
                    {
                        current_directory_ = sub_dir;
                        break;
                    }
                }
            }
            return true;
        }
        return false;
    }
    bool navigate_to_breadcrumb(int index)
    {
        if (index < 0 || index >= static_cast<int>(paths_.size()))
        {
            return false;
        }

        paths_.resize(index + 1);
        current_directory_ = paths_.back();
        return true;
    }
    [[nodiscard]] const std::vector<std::shared_ptr<directory>>& breadcrumb_paths() const { return paths_; }
    [[nodiscard]] const std::shared_ptr<directory>& current_directory() const { return current_directory_; }

   private:
    std::shared_ptr<directory> root_;
    std::shared_ptr<directory> current_directory_;
    std::vector<std::shared_ptr<directory>> paths_;
};

}    // namespace leaf

#endif
