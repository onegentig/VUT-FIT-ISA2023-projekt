/**
 * @file BasePacket.hpp
 * @author Onegen Something <xkrame00@vutbr.cz>
 * @brief Base class for all TFTP packets.
 * @date 2023-09-28
 */

#pragma once
#ifndef BASE_PACKET_HPP
#     define BASE_PACKET_HPP
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
     virtual std::vector<char> toBinary() const = 0;

     /**
      * @brief Creates a packet from a binary representation.
      * @param std::vector<char> - binary representation of the packet
      * @return void
      */
     virtual void fromBinary(const std::vector<char>&) = 0;

     /* === Helper Methods === */

     /**
      * @brief Searches for a null-terminated string in a binary data vector.
      * @param std::vector<char> - binary data vector
      * @param size_t - offset to start searching from
      * @param std::string& - string to store the result in
      * @return size_t - position of the next character after the null
      * terminator
      */
     static inline size_t findcstr(const std::vector<char>& binaryData,
                                   size_t offset, std::string& result) {
          /* Search for a null terminator */
          size_t end = offset;
          while (end < binaryData.size() && binaryData[end] != 0) ++end;

          if (end >= binaryData.size())  // No null-terminator found
               throw std::invalid_argument("Invalid payload");

          result.assign(binaryData.begin() + offset, binaryData.begin() + end);
          return end + 1;  // Position of the next character after the null
                           // terminator
     }

     /* === Getters and Setters === */

     /**
      * @brief Returns the two-byte opcode of the packet.
      * @return TFTPOpcode - packet opcode
      */
     TFTPOpcode getOpcode() const { return this->opcode; }

     /* === Debugging Methods === */

     /**
      * @brief Prints the packet as an inline table
      *        to the standard output. For debugging purposes.
      * @return void
      */
     virtual void dprint() const = 0;

     /**
      * @brief Creates a hexdump string of a 2B numerical segment.
      * @return std::string - hexdump string
      */
     static inline std::string dprint2BNum(uint16_t num) {
          std::ostringstream stream;
          stream << std::hex << std::setw(2) << std::setfill('0')
                 << static_cast<int>((num & 0xFF00) / 256) << " "
                 << std::setw(2) << (num & 0x00FF);
          return stream.str();
     }

     /**
      * @brief Creates a hexdump string of a string segment
      * @return std::string - hexdump string
      */
     static inline std::string dprintStr(std::string str) {
          std::ostringstream stream;

          if (str.size() <= MAX_DEBUG_STRING)
               for (char c : str)
                    stream << std::setw(2) << static_cast<int>(c) << " ";
          return stream.str();

          // TODO: Make this obey MAX_DEBUG_STRING
          stream << std::setw(2) << static_cast<int>(str[0]) << " "
                 << std::setw(2) << static_cast<int>(str[1]) << " .. "
                 << std::setw(2) << static_cast<int>(str[str.size() - 2]) << " "
                 << std::setw(2) << static_cast<int>(str[str.size() - 1]);

          return stream.str();
     }

   protected:
     TFTPOpcode opcode; /**< Two-byte opcode of the packet. */
};

#endif
