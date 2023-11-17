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
 */
class NetASCII {
   public:
     /**
      * @brief Convert plain binary data to NetASCII.
      * Method replaces all occurences of `\n` with `\r\n` and `\r` with `\r\0`.
      * @see https://datatracker.ietf.org/doc/html/rfc764
      * @see https://www.reissenzahn.com/protocols/tftp#netascii
      * @param std::vector<char> binary data
      * @return std::vector<char> NetASCII data
      */
     static std::vector<char> to_netascii(const std::vector<char>& data) {
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
      * @brief Convert NetASCII data to plain binary.
      * Method replaces all occurences of `\r\n` with `\n` and `\r\0` with `\r`.
      * @see https://datatracker.ietf.org/doc/html/rfc764
      * @see https://www.reissenzahn.com/protocols/tftp#netascii
      * @param std::vector<char> NetASCII data
      * @return std::vector<char> Native binary data
      */
     static std::vector<char> from_netascii(const std::vector<char>& data) {
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
};

#endif