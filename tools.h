//
// Created by leonardo on 04/11/20.
//

#ifndef REMOTE_BACKUP_M1_CLIENT_tools_H
#define REMOTE_BACKUP_M1_CLIENT_tools_H

#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <boost/uuid/detail/md5.hpp>
#include "message.h"


struct tools {
    static std::pair<bool, std::vector<std::string>>
    match_and_parse(boost::regex const &regex, std::string const &line);

    static std::string hash(boost::filesystem::path const &path);

    static std::string create_sign(boost::filesystem::path const &path, std::string const &digest);

    static std::vector<std::string> split_sign(std::string const &sign);

    static void retry(std::function<void(void)> const &func, int attempts = 3);

private:
    static std::string hash_to_string(boost::uuids::detail::md5::digest_type const& digest);

};

#endif //REMOTE_BACKUP_M1_CLIENT_tools_H