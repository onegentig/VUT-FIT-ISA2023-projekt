/**
 * @file RequestPacket.hpp
 * @author Filip J. Kramec <xkrame00@vutbr.cz>
 * @brief TFTP request packet.
 * @date 2023-09-28
 */

#include "packet/RequestPacket.hpp"

/* === Constructors === */

RequestPacket::RequestPacket() : filename(""), mode("") {
     opcode = TFTPOpcode::RRQ;
}

RequestPacket::RequestPacket(RequestPacketType type, std::string filename,
                             std::string mode)
    : filename(filename), mode(mode) {
     opcode
         = type == RequestPacketType::Read ? TFTPOpcode::RRQ : TFTPOpcode::WRQ;
}

/* === Core Methods === */

std::vector<char> RequestPacket::toBinary() const {
     size_t length = 2 /* opcode */ + filename.length()
                     + 1 /* separator */ + mode.length() + 1 /* separator */;
     std::vector<char> binaryData(length);

     /* Convert and copy opcode in network byte order */
     uint16_t opcode = htons(static_cast<uint16_t>(this->opcode));
     std::memcpy(binaryData.data(), &opcode, sizeof(opcode));

     /* Insert filename and mode strings to vector */
     size_t offset = 2;  // after 2B opcode
     std::memcpy(binaryData.data() + offset, filename.c_str(),
                 filename.length());
     offset += filename.length() + 1;  // after filename + 1B separator
     std::memcpy(binaryData.data() + offset, mode.c_str(), mode.length());

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
     size_t offset = 2;
     offset = findcstr(binaryData, offset, filename);
     offset = findcstr(binaryData, offset, mode);
}
