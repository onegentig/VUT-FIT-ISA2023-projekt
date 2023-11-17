/**
 * @file BasePacket.hpp
 * @author Onegen Something <xkrame00@vutbr.cz>
 * @brief Base class for all TFTP packets.
 * @date 2023-09-28
 */

#pragma once
#ifndef TFTP_BASE_PACKET_HPP
#     define TFTP_BASE_PACKET_HPP
#     include <iomanip>

#     include "common.hpp"

/**
 * @brief Base class for all TFTP packets.
 * @note This class does is only to be inherited from.
 */
class BasePacket {
   public:
     virtual ~BasePacket() = default;

     /* === Core Methods === */

     /**
      * @brief Returns the binary representation of the packet.
      * @return std::vector<char> - binary representation of the packet
      */
     virtual std::vector<char> to_binary() const = 0;

     /**
      * @brief Compares and checks equality of two BasePacket objects.
      * @param other BasePacket object
      * @return true when equal, otherwise false
      */
     bool operator==(const BasePacket& other) const {
          return this->opcode == other.opcode;
     }

     /* === Helper Methods === */

     /**
      * @brief Searches for a null-terminated string in a binary data vector.
      * @param std::vector<char> binary data vector
      * @param size_t offset to start searching from
      * @param std::string& string to store the result in
      * @return size_t - position of the next character after the null
      * terminator
      */
     static inline size_t findcstr(const std::vector<char>& bin_data,
                                   size_t offset, std::string& result) {
          /* Search for a null terminator */
          size_t end = offset;
          while (end < bin_data.size() && bin_data[end] != 0) ++end;

          if (end >= bin_data.size())  // No null-terminator found
               throw std::invalid_argument("Invalid payload");

          result.assign(bin_data.begin() + offset, bin_data.begin() + end);
          return end + 1;  // Position of the next character after the null
                           // terminator
     }

     /* === Getters and Setters === */

     /**
      * @brief Returns the two-byte opcode of the packet.
      * @return TFTPOpcode - packet opcode
      */
     TFTPOpcode get_opcode() const { return this->opcode; }

     /* === Debugging Methods === */

     /**
      * @brief Returns a hexdump string of the packet.
      * @return std::string - hexdump string
      */
     std::string hexdump() const {
          std::ostringstream stream;

          auto bin = to_binary();
          for (size_t i = 0; i < bin.size(); ++i) {
               stream << std::hex << std::setw(2) << std::setfill('0')
                      << static_cast<int>(bin[i]) << " ";
          }
          std::cout << stream.str() << std::endl;

          return stream.str();
     }

   protected:
     TFTPOpcode opcode; /**< Two-byte opcode of the packet. */
};

#endif
