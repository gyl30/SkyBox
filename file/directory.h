#ifndef LEAF_FILE_DIRECTORY_H
#define LEAF_FILE_DIRECTORY_H

#include <cassert>
#include <utility>
#include <vector>
#include <string>
#include <memory>
#include <filesystem>
#include "file/file_item.h"

namespace leaf
{
class directory
{
   public:
    directory(std::string parent, std::string dir) : parent_(std::move(parent)), name_(std::move(dir)) {}

   public:
    void add_dir(const std::shared_ptr<directory>& dir) { dirs_.push_back(dir); }
    void add_dir(const leaf::file_item& item, const std::shared_ptr<directory>& dir)
    {
        files_.push_back(item);
        dirs_.push_back(dir);
    }

    void add_file(const leaf::file_item& file) { files_.push_back(file); }

    void reset()
    {
        files_.clear();
        dirs_.clear();
    }
    bool dir_exist(const std::shared_ptr<directory>& dir)
    {
        for (const auto& sub_dir : dirs_)
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
    [[nodiscard]] const std::string& parent() const { return parent_; }
    [[nodiscard]] std::string path() const
    {
        if (parent_.empty())
        {
            return name_;
        }
        return std::filesystem::path(parent_).append(name_).string();
    }
    [[nodiscard]] const std::vector<leaf::file_item>& files() const { return files_; }
    [[nodiscard]] const std::vector<std::shared_ptr<directory>>& subdirectories() const { return dirs_; }

   private:
    std::string parent_;
    std::string name_;
    std::vector<leaf::file_item> files_;
    std::vector<std::shared_ptr<directory>> dirs_;
};

class path_manager
{
   public:
    explicit path_manager(const std::shared_ptr<directory>& root) : root_(root), current_directory_(root) { paths_.push_back(root); }

   public:
    bool enter_directory(const std::shared_ptr<directory>& dir)
    {
        const auto& subdirectories = current_directory_->subdirectories();
        for (const auto& sub_dir : subdirectories)    // NOLINT
        {
            assert(sub_dir->parent() == dir->parent());
            assert(current_directory_->path() == sub_dir->parent());
            assert(current_directory_->path() == dir->parent());
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
