/**
 * @file ErrorPacket.cpp
 * @author Filip J. Kramec <xkrame00@vutbr.cz>
 * @brief TFTP error packet.
 * @date 2023-10-16
 */

#include "packet/ErrorPacket.hpp"

/* === Constructors === */

ErrorPacket::ErrorPacket() : errcode(TFTPErrorCode::Unknown) {
     opcode = TFTPOpcode::ERROR;
}

ErrorPacket::ErrorPacket(TFTPErrorCode errcode) : errcode(errcode) {
     opcode = TFTPOpcode::ERROR;
}

ErrorPacket::ErrorPacket(TFTPErrorCode errcode, std::string msg)
    : errcode(errcode), msg(msg) {
     opcode = TFTPOpcode::ERROR;
}

/* === Core Methods === */

std::vector<char> ErrorPacket::to_binary() const {
     std::vector<char> binaryData(4);  // 2B opcode + 2B errcode

     /* Convert and copy opcode and error code in network byte order */
     uint16_t opcode = htons(static_cast<uint16_t>(this->opcode));
     uint16_t errcode = htons(static_cast<uint16_t>(this->errcode));
     std::memcpy(binaryData.data(), &opcode, sizeof(opcode));
     std::memcpy(binaryData.data() + 2, &errcode, sizeof(errcode));

     /* Insert error message to vector */
     if (msg.has_value()) {
          std::vector<char> msgBin = BasePacket::to_netascii(
              std::vector<char>(msg.value().begin(), msg.value().end()));
          binaryData.insert(binaryData.end(), msgBin.begin(), msgBin.end());
     } else {
          binaryData.push_back('\0');
     }

     return binaryData;
}

void ErrorPacket::from_binary(const std::vector<char>& binaryData) {
     if (binaryData.size() < 4)  // Min. size is 4B (2B opcode + 2B errcode)
          throw std::invalid_argument("Incorrect packet size");

     /* Obtain and validate opcode */
     uint16_t opcode;
     std::memcpy(&opcode, binaryData.data(), sizeof(opcode));
     opcode = ntohs(opcode);
     if (opcode != static_cast<uint16_t>(TFTPOpcode::ERROR))
          throw std::invalid_argument("Incorrect opcode");
     this->opcode = TFTPOpcode::ERROR;

     /* Obtain and validate error code */
     uint16_t errcode;
     std::memcpy(&errcode, binaryData.data() + 2, sizeof(errcode));
     errcode = ntohs(errcode);
     if (errcode > 7) throw std::invalid_argument("Incorrect error code");
     this->errcode = static_cast<TFTPErrorCode>(errcode);

     /* Obtain and validate error message */
     if (binaryData.size() > 5) {
          std::string msgStr;
          std::vector<char> msgBin(binaryData.begin() + 4, binaryData.end());
          msgBin = BasePacket::from_netascii(msgBin);
          msgStr = std::string(msgBin.begin(), msgBin.end());
          this->msg = msgStr;
     }
}