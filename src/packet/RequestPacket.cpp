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

     /* Serialise strings */
     std::vector<char> filename_bin = NetASCII::str_to_na(filename);
     std::vector<char> mode_bin = NetASCII::str_to_na(mode_str);

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
     if (bin_data.size() > 512)
          throw std::invalid_argument("Packet size exceeds 512B");

     return bin_data;
}

RequestPacket RequestPacket::from_binary(const std::vector<char>& bin_data) {
     if (bin_data.size() < 4)  // Min. size is 4B (2B opcode + 2B separator)
          throw std::invalid_argument("Incorrect packet size");
     if (bin_data.size() > 512)  // Max. size is 512B (as defined by RFC 2347)
          throw std::invalid_argument("Packet too large");

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

     /* Create packet */
     auto packet
         = RequestPacket(opcode == static_cast<uint16_t>(TFTPOpcode::RRQ)
                             ? TFTPRequestType::Read
                             : TFTPRequestType::Write,
                         filename, mode);

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
