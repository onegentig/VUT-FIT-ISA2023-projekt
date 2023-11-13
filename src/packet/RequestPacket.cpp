/**
 * @file RequestPacket.hpp
 * @author Onegen Something <xkrame00@vutbr.cz>
 * @brief TFTP request packet.
 * @date 2023-09-28
 */

#include "packet/RequestPacket.hpp"

#include <algorithm>

/* === Constructors === */

RequestPacket::RequestPacket() : mode(TFTPDataFormat::Octet) {
     opcode = TFTPOpcode::RRQ;
}

RequestPacket::RequestPacket(TFTPRequestType type, std::string filename,
                             TFTPDataFormat mode)
    : filename(std::move(filename)), mode(mode) {
     opcode = type == TFTPRequestType::Read ? TFTPOpcode::RRQ : TFTPOpcode::WRQ;
}

/* === Core Methods === */

std::vector<char> RequestPacket::to_binary() const {
     /* If filename is not set, return an empty vector */
     if (filename.empty()) return std::vector<char>();

     std::string mode_str = this->get_mode_str();
     std::vector<char> filename_bin = BasePacket::to_netascii(
         std::vector<char>(filename.begin(), filename.end()));
     std::vector<char> mode_bin = BasePacket::to_netascii(
         std::vector<char>(mode_str.begin(), mode_str.end()));

     size_t length = 2 /* opcode */ + filename_bin.size() + mode_bin.size()
                     + 2 /* separators */;
     std::vector<char> bin_data(length);

     /* Convert and copy opcode in network byte order */
     uint16_t opcode = htons(static_cast<uint16_t>(this->opcode));
     std::memcpy(bin_data.data(), &opcode, sizeof(opcode));

     /* Insert filename and mode strings to vector */
     size_t offset = 2;  // after 2B opcode
     std::memcpy(bin_data.data() + offset, filename_bin.data(),
                 filename_bin.size());
     offset += filename.length() + 1;  // after filename + separator
     std::memcpy(bin_data.data() + offset, mode_bin.data(), mode_bin.size());

     return bin_data;
}

RequestPacket RequestPacket::from_binary(const std::vector<char>& bin_data) {
     if (bin_data.size() < 4)  // Min. size is 4B (2B opcode + 2B separator)
          throw std::invalid_argument("Incorrect packet size");

     /* Obtain and validate opcode */
     uint16_t opcode;
     std::memcpy(&opcode, bin_data.data(), sizeof(opcode));
     opcode = ntohs(opcode);

     if (opcode != static_cast<uint16_t>(TFTPOpcode::RRQ)
         && opcode != static_cast<uint16_t>(TFTPOpcode::WRQ))
          throw std::invalid_argument("Incorrect opcode");

     /* Search for the filename and mode strings */
     std::string mode_str;
     std::string filename;
     size_t offset = 2;
     offset = findcstr(bin_data, offset, filename);
     offset = findcstr(bin_data, offset, mode_str);

     /* Validate and parse mode (case insensitive) */
     TFTPDataFormat mode;
     std::transform(mode_str.begin(), mode_str.end(), mode_str.begin(),
                    ::tolower);
     if (mode_str == "octet") {
          mode = TFTPDataFormat::Octet;
     } else if (mode_str == "netascii") {
          mode = TFTPDataFormat::NetASCII;
     } else {
          throw std::invalid_argument("Incorrect mode");
     }

     return RequestPacket(opcode == static_cast<uint16_t>(TFTPOpcode::RRQ)
                              ? TFTPRequestType::Read
                              : TFTPRequestType::Write,
                          filename, mode);
}
