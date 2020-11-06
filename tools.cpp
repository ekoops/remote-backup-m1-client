//
// Created by leonardo on 04/11/20.
//

#include "tools.h"
#include "op.h"
#include "tlv_view.h"

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

std::string tools::hash(boost::filesystem::path const &path) {
    std::ifstream ifs{path};
    std::string s;
    md5 hash;
    md5::digest_type digest;
    while (std::getline(ifs, s)) {
        hash.process_bytes(s.data(), s.size());
    }
    hash.get_digest(digest);
    std::ostringstream oss;
    oss << digest[0] << digest[1] << digest[2] << digest[3];
    return oss.str();
}

std::string tools::get_file_sign(boost::filesystem::path const &path,
                                 std::string const &digest) {
    std::ostringstream oss;
    oss << path.generic_path().string() << '\x00' << digest;
    return oss.str();
}
