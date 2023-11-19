/**
 * @file OptionAckPacket.hpp
 * @author Onegen Something <xkrame00@vutbr.cz>
 * @brief TFTP option acknowledgement packet
 * @date 2023-11-19
 */

#include "packet/OptionAckPacket.hpp"

#include <algorithm>

/* === Constructors === */

OptionAckPacket::OptionAckPacket() { opcode = TFTPOpcode::OACK; }

OptionAckPacket::OptionAckPacket(
    std::vector<std::pair<std::string, std::string>> opt_vec)
    : opts(std::move(opt_vec)) {
     opcode = TFTPOpcode::OACK;
}

/* === Core Methods === */

std::vector<char> OptionAckPacket::to_binary() const {
     /* If there are no options to ack, return empty vector */
     if (opts.empty()) return std::vector<char>();
     std::vector<char> bin_data(sizeof(opcode));

     /* Copy opcode */
     uint16_t opcode = htons(static_cast<uint16_t>(this->opcode));
     std::memcpy(bin_data.data(), &opcode, sizeof(opcode));

     /* Serialise options one-by-one */
     for (const auto& opt : opts) {
          /* Name */
          std::vector<char> opt_bin = NetASCII::str_to_na(opt.first);
          bin_data.insert(bin_data.end(), opt_bin.begin(), opt_bin.end());
          bin_data.push_back('\0');

          /* Value */
          opt_bin = NetASCII::str_to_na(opt.second);
          bin_data.insert(bin_data.end(), opt_bin.begin(), opt_bin.end());
          bin_data.push_back('\0');
     }

     /* Ensure that the length doesn’t exceed 512B */
     if (bin_data.size() > 512)  // Should never happen, but just in case
          throw std::invalid_argument("Packet size exceeds 512B");

     return bin_data;
}

OptionAckPacket OptionAckPacket::from_binary(
    const std::vector<char>& bin_data) {
     if (bin_data.size() < 4)  // Min. size is 4B (2B opcode + 2 terminators)
          throw std::invalid_argument("Incorrect packet size");
     if (bin_data.size() > 512)  // Max. size is 512B
          throw std::invalid_argument("Packet too large");

     /* Obtain and validate opcode */
     uint16_t opcode;
     std::memcpy(&opcode, bin_data.data(), sizeof(opcode));
     opcode = ntohs(opcode);
     size_t offset = 2;  // after 2B opcode

     if (opcode != static_cast<uint16_t>(TFTPOpcode::OACK))
          throw std::invalid_argument("Incorrect opcode");

     /* Create packet */
     auto packet = OptionAckPacket();

     /* Parse options */
     while (offset < bin_data.size()) {
          std::string opt_name, opt_val;

          offset = findcstr(bin_data, offset, opt_name);
          if (offset >= bin_data.size())
               throw std::invalid_argument("Incomplete option key");

          offset = findcstr(bin_data, offset, opt_val);
          if (offset > bin_data.size())
               throw std::invalid_argument("Incomplete option value");

          packet.add_option(opt_name, opt_val);
     }

     return packet;
}
