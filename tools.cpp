//
// Created by leonardo on 04/11/20.
//

#include "tools.h"
#include "message.h"
#include <boost/algorithm/hex.hpp>
#include <boost/asio.hpp>

using boost::uuids::detail::md5;

std::pair<bool, std::vector<std::string>> tools::match_and_parse(boost::regex const &regex, std::string const &line) {
    boost::smatch match_results{};
    std::vector<std::string> results;
    if (boost::regex_match(line, match_results, regex)) {
        for (int i = 1; i < match_results.size(); i++) {
            results.emplace_back(match_results[i]);
        }
        return {true, results};
    } else return {false, results};
}

std::string tools::create_sign(boost::filesystem::path const &path,
                               std::string const &digest) {
    std::ostringstream oss;
    oss << path.generic_path().string() << '\x00' << digest;
    return oss.str();
}

std::vector<std::string> tools::split_sign(std::string const &sign) {
    std::istringstream oss{sign};
    std::string temp;
    std::vector<std::string> results(2);
    while (std::getline(oss, temp, '\x00')) results.push_back(temp);
    return results;
}

void tools::retry(std::function<void(void)> const &func, int attempts) {
    while (attempts--) {
        try {
            func();
            attempts = 0;
        }
        catch (boost::system::system_error &ex) {
            if (!attempts) throw ex;
        }
    }
}

std::string tools::hash(boost::filesystem::path const &path) {
    boost::filesystem::ifstream ifs;
    ifs.open(path, std::ios_base::binary);
    ifs.unsetf(std::ios::skipws);           // Stop eating new lines in binary mode!!!
    std::streampos length;
    ifs.seekg(0, std::ios::end);
    length = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    md5 hash;
    md5::digest_type digest;

    std::string path_str {path.generic_path().string()};
    hash.process_bytes(path_str.c_str(), path_str.size());

    std::vector<char> file_buffer(length);
    ifs.read(&*file_buffer.begin(), length);
    hash.process_bytes(&*file_buffer.cbegin(), length);
    hash.get_digest(digest);

//    std::cout << "HASH1 " << hash_to_string(digest) << std::endl;
    return hash_to_string(digest);
}

std::string tools::hash_to_string(boost::uuids::detail::md5::digest_type const &digest) {
    const auto int_digest = reinterpret_cast<const int *>(&digest);
    std::string result;

    boost::algorithm::hex(int_digest, int_digest + (sizeof(md5::digest_type) / sizeof(int)),
                          std::back_inserter(result));
    return result;
}