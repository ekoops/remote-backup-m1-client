#include "tools.h"
#include <boost/algorithm/hex.hpp>
#include <boost/asio.hpp>

using boost::uuids::detail::md5;
namespace fs = boost::filesystem;

/**
* Create a sign to uniquely identify a specific resource.
*
* @param relative_path the resource relative path
* @param digest the resource digest
* @return an std::string representing a sign in the form PATH\x00DIGEST
*/
std::string tools::create_sign(
        fs::path const &relative_path,
        std::string const &digest
) {
    std::ostringstream oss;
    oss << relative_path.generic_path().string() << '\x00' << digest;
    return oss.str();
}

/**
* Allow to split a resource sign to obtain the corresponding relative
* path and digest.
*
* @param sign the sign that has to be split
* @return an std::pair containing both path and digest
*/
std::pair<fs::path, std::string> tools::split_sign(std::string const &sign) {
    std::istringstream oss{sign};
    std::string temp;
    boost::regex regex{"(.*)\\x00(.*)"};
    auto pair = tools::match_and_parse(regex, sign);
    if (pair.first) return {pair.second[0], pair.second[1]};
    else throw std::runtime_error{"Failed to parse sign"};
}

/**
* Allow to validate a string format using a regex and optionally return all
* grouped items.
*
* @param regex the regular expression for string format validation
* @param str the string that has to be validated
* @return an std::pair containing a bool indicating if the provided
 * string has the right format and conditionally a vector containing
 * all grouped items
*/
std::pair<bool, std::vector<std::string>> tools::match_and_parse(boost::regex const &regex, std::string const &str) {
    boost::smatch match_results{};
    std::vector<std::string> results;
    if (boost::regex_match(str, match_results, regex)) {
        for (int i = 1; i < match_results.size(); i++) {
            results.emplace_back(match_results[i]);
        }
        return {true, results};
    } else return {false, results};
}

/**
* An MD5 based hash function to obtain a file digest
*
* @param absolute_path the absolute path location of the file
* @param relative_path the relative path location that has to be included in digest computation
* @return a string representation of file MD5 digest
*/
std::string tools::MD5_hash(fs::path const &absolute_path, fs::path const &relative_path) {
    fs::ifstream ifs;
    ifs.open(absolute_path, std::ios_base::binary);
    ifs.unsetf(std::ios::skipws);
    std::streampos length;
    ifs.seekg(0, std::ios::end);
    length = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    md5 hash;
    md5::digest_type digest;

    std::vector<char> file_buffer(length);
    file_buffer.reserve(length);
    file_buffer.insert(file_buffer.begin(),
                       std::istream_iterator<char>(ifs),
                       std::istream_iterator<char>());

    std::string relative_path_str{relative_path.generic_path().string()};
    hash.process_bytes(relative_path_str.c_str(), relative_path_str.size());
    hash.process_bytes(&*file_buffer.cbegin(), length);
    hash.get_digest(digest);

    return MD5_to_string(digest);
}

/**
* Allow to obtain a string representation from an MD5 digest
*
* @param digest the digest that has to be converted
* @return a string representation of an MD5 digest
*/
std::string tools::MD5_to_string(boost::uuids::detail::md5::digest_type const &digest) {
    const auto int_digest = reinterpret_cast<const int *>(&digest);
    std::string result;

    boost::algorithm::hex(int_digest, int_digest + (sizeof(md5::digest_type) / sizeof(int)),
                          std::back_inserter(result));
    return result;
}