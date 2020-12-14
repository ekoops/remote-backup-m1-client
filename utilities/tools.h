#ifndef REMOTE_BACKUP_M1_CLIENT_tools_H
#define REMOTE_BACKUP_M1_CLIENT_tools_H

#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <boost/uuid/detail/md5.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include "../communication/message.h"

// Utility static methods
struct tools {
    static std::string MD5_hash(
            boost::filesystem::path const &absolute_path,
            boost::filesystem::path const &relative_path
    );

    static std::string create_sign(
            boost::filesystem::path const &relative_path,
            std::string const &digest
    );

    static std::pair<boost::filesystem::path, std::string> split_sign(std::string const &sign);

    static std::pair<bool, std::vector<std::string>> match_and_parse(
            boost::regex const &regex,
            std::string const &str
    );

private:
    static std::string MD5_to_string(boost::uuids::detail::md5::digest_type const &digest);
};

#endif //REMOTE_BACKUP_M1_CLIENT_tools_H