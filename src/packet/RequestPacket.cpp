/**
 * @file RequestPacket.hpp
 * @author Filip J. Kramec <xkrame00@vutbr.cz>
 * @brief TFTP request packet.
 * @date 2023-09-28
 */

#include "packet/RequestPacket.hpp"

#include <algorithm>

/* === Constructors === */

RequestPacket::RequestPacket() : filename(""), mode(TFTPDataFormat::Octet) {
     opcode = TFTPOpcode::RRQ;
}

RequestPacket::RequestPacket(RequestPacketType type, std::string filename,
                             TFTPDataFormat mode)
    : filename(std::move(filename)), mode(mode) {
     opcode
         = type == RequestPacketType::Read ? TFTPOpcode::RRQ : TFTPOpcode::WRQ;
}

/* === Core Methods === */

std::vector<char> RequestPacket::toBinary() const {
     /* If filename is not set, return an empty vector */
     if (filename.empty()) return std::vector<char>();

     std::string modeStr = this->getModeStr();
     std::vector<char> filenameBin = BasePacket::toNetascii(
         std::vector<char>(filename.begin(), filename.end()));
     std::vector<char> modeBin = BasePacket::toNetascii(
         std::vector<char>(modeStr.begin(), modeStr.end()));

     size_t length = 2 /* opcode */ + filenameBin.size()
                     + 1 /* separator */ + modeBin.size() + 1 /* separator */;
     std::vector<char> binaryData(length);

     /* Convert and copy opcode in network byte order */
     uint16_t opcode = htons(static_cast<uint16_t>(this->opcode));
     std::memcpy(binaryData.data(), &opcode, sizeof(opcode));

     /* Insert filename and mode strings to vector */
     size_t offset = 2;  // after 2B opcode
     std::memcpy(binaryData.data() + offset, filenameBin.data(),
                 filenameBin.size());
     offset += filename.length() + 1;  // after filename + separator
     std::memcpy(binaryData.data() + offset, modeBin.data(), modeBin.size());

     return binaryData;
}

void RequestPacket::fromBinary(const std::vector<char>& binaryData) {
     if (binaryData.size() < 4)  // Min. size is 4B (2B opcode + 2B separator)
          throw std::invalid_argument("Incorrect packet size");

     /* Obtain and validate opcode */
     uint16_t opcode;
     std::memcpy(&opcode, binaryData.data(), sizeof(opcode));
     opcode = ntohs(opcode);

     if (opcode != static_cast<uint16_t>(TFTPOpcode::RRQ)
         && opcode != static_cast<uint16_t>(TFTPOpcode::WRQ))
          throw std::invalid_argument("Incorrect opcode");
     this->opcode = static_cast<TFTPOpcode>(opcode);

     /* Search for the filename and mode strings */
     std::string modeStr;
     size_t offset = 2;
     offset = findcstr(binaryData, offset, filename);
     offset = findcstr(binaryData, offset, modeStr);

     /* Validate and parse mode (case insensitive) */
     std::transform(modeStr.begin(), modeStr.end(), modeStr.begin(), ::tolower);
     if (modeStr == "octet") {
          mode = TFTPDataFormat::Octet;
     } else if (modeStr == "netascii") {
          mode = TFTPDataFormat::NetASCII;
     } else {
          throw std::invalid_argument("Incorrect mode");
     }
}
