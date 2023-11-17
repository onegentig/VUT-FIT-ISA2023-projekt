/**
 * @file NetASCII.hpp
 * @author Filip J. Kramec <xkrame00@vutbr.cz>
 * @brief NetASCII manipulation utility class
 * @date 2023-11-17
 */

#pragma once
#ifndef TFTP_NETASCII_HPP
#     define TFTP_NETASCII_HPP
#     include <cstdint>
#     include <vector>

/**
 * @brief NetASCII manipulation utility class
 * @see https://datatracker.ietf.org/doc/html/rfc764
 * @see https://www.reissenzahn.com/protocols/tftp#netascii
 */
class NetASCII {
   public:
     /* === Primary conversion methods === */

     /**
      * @brief Converts a binary vector to NetASCII vector
      * @details Method replaces all occurences of `\n` with `\r\n` and `\r`
      * with `\r\0`.
      * @param std::vector<char> binary data
      * @return NetASCII binary vector
      */
     static std::vector<char> vec_to_na(const std::vector<char>& data) {
          std::vector<char> netasciiData;
          for (uint64_t i = 0; i < data.size(); i++) {
               if (data[i] == '\n') {
                    // LF -> CR LF
                    netasciiData.push_back('\r');
                    netasciiData.push_back('\n');
               } else if (data[i] == '\r') {
                    if (i + 1 < data.size() && data[i + 1] == '\n') {
                         // CR LF -> CR LF
                         netasciiData.push_back('\r');
                         netasciiData.push_back('\n');
                         i++;
                    } else {
                         // CR -> CR NUL
                         netasciiData.push_back('\r');
                         netasciiData.push_back('\0');
                    }
               } else {
                    // Regular character
                    netasciiData.push_back(data[i]);
               }
          }

          return netasciiData;
     }

     /**
      * @brief Convert NetASCII vector to a binary vector
      * @details Method replaces all occurences of `\r\n` with `\n` and `\r\0`
      * with `\r`.
      * @param std::vector<char> NetASCII binary vector
      * @return Unix binary vector
      */
     static std::vector<char> na_to_vec(const std::vector<char>& data) {
          std::vector<char> bin_data;
          for (uint64_t i = 0; i < data.size(); i++) {
               if (data[i] == '\r') {
                    if (i + 1 < data.size() && data[i + 1] == '\n') {
                         // CR LF -> LF
                         bin_data.push_back('\n');
                         i++;
                    } else if (i + 1 < data.size() && data[i + 1] == '\0') {
                         // CR NUL -> CR
                         bin_data.push_back('\r');
                         i++;
                    } else {
                         // CR -> CR
                         bin_data.push_back('\r');
                    }
               } else {
                    // Regular character
                    bin_data.push_back(data[i]);
               }
          }

          return bin_data;
     }

     /* === Variants === */

     /**
      * @brief Converts std::string to a NetASCII vector
      * @param std::string str String to convert
      * @return NetASCII binary vector
      */
     static std::vector<char> str_to_na(const std::string& str) {
          return vec_to_na(std::vector<char>(str.begin(), str.end()));
     }

     /**
      * @brief Converts NetASCII vector to a std::string
      * @param std::vector<char> NetASCII binary vector
      * @return std::string
      */
     static std::string na_to_str(const std::vector<char>& data) {
          std::vector<char> bin = na_to_vec(data);
          return std::string(bin.begin(), bin.end());
     }
};

#endif