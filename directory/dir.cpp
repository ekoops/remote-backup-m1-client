#include "dir.h"

using namespace directory;

/**
 * Construct a new directory instance.
 *
 * @param path Path associated with the created instance.
 * @param concurrent_accessed A boolean value specifying if the directory is accessed in concurrent mode
 * @return a new constructed directory instance
 */
dir::dir(boost::filesystem::path path,
         bool concurrent_accessed)
         : path_{std::move(path)}
         , concurrent_accessed_{concurrent_accessed} {}

/**
* Construct a new directory instance shared_ptr.
*
* @param path Path associated with the created instance.
* @param concurrent_accessed A boolean value specifying if the directory is accessed in concurrent mode
* @return a new constructed directory instance shared_ptr
*/
std::shared_ptr<dir> dir::get_instance(boost::filesystem::path path, bool concurrent_accessed) {
    return std::shared_ptr<dir>(new dir{std::move(path), concurrent_accessed});
}

/**
 * Insert or assign a new directory entry.
 *
 * @param path Path of the entry.
 * @param rsrc Resource associated to path.
 * @return true if a new entry has been inserted, false if an assigment has been performed.
 */
bool dir::insert_or_assign(boost::filesystem::path const &path, resource const& rsrc) {
    std::unique_lock ul{this->m_, std::defer_lock};
    if (concurrent_accessed_) ul.lock();
    return this->content_.insert_or_assign(path, rsrc).second;
}

/**
 * Erase a directory entry.
 *
 * @param path Path of the entry.
 * @return true if an entry has been deleted, false otherwise.
 */
bool dir::erase(boost::filesystem::path const& path) {
    std::unique_lock ul{this->m_, std::defer_lock};
    if (concurrent_accessed_) ul.lock();
    return this->content_.erase(path) == 1;
}

/**
 * Check if directory contains a specified path.
 *
 * @param path Path of the entry.
 * @return true if an entry associated with the specified path has been found, false otherwise.
 */
bool dir::contains(boost::filesystem::path const& path) const {
    std::unique_lock ul{this->m_, std::defer_lock};
    if (concurrent_accessed_) ul.lock();
    return this->content_.find(path) != this->content_.end();
}

/**
 * Provide the directory associated path
 *
 * @return the directory associated path.
 */
boost::filesystem::path const& dir::path() const {
    return this->path_;
}

/**
 * Provide the provided path associated resource
 *
 * @param path Path of the entry.
 * @return an std::optional<resource> containing the provided path associated resource
 * or std::nullopt if directory doesn't contain an entry associated to path
 */
std::optional<resource> dir::rsrc(boost::filesystem::path const& path) const {
    std::unique_lock ul{this->m_, std::defer_lock};
    if (concurrent_accessed_) ul.lock();
    auto it = this->content_.find(path);
    return it != this->content_.end() ? std::optional<directory::resource>{it->second} : std::nullopt;

}

/**
 * Allows to execute a specified function on each directory entry
 *
 * @param fn Function that have to be executed on each directory entry.
 * @return void
 */
void dir::for_each(std::function<void(std::pair<boost::filesystem::path, directory::resource> const&)> const& fn) const {
    std::unique_lock ul{this->m_, std::defer_lock};
    if (concurrent_accessed_) ul.lock();
    auto it = this->content_.cbegin();
    while (it != this->content_.cend()) fn(*it++);
}

/**
 * Allows to erase all directory entries
 *
 * @return void
 */
void dir::clear() {
    std::unique_lock ul{this->m_, std::defer_lock};
    if (concurrent_accessed_) ul.lock();
    this->content_.clear();
}
