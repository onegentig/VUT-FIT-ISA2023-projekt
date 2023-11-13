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
     std::vector<char> bin_data(4);  // 2B opcode + 2B errcode

     /* Convert and copy opcode and error code in network byte order */
     uint16_t opcode = htons(static_cast<uint16_t>(this->opcode));
     uint16_t errcode = htons(static_cast<uint16_t>(this->errcode));
     std::memcpy(bin_data.data(), &opcode, sizeof(opcode));
     std::memcpy(bin_data.data() + 2, &errcode, sizeof(errcode));

     /* Insert error message to vector */
     if (msg.has_value()) {
          std::vector<char> msg_bin = BasePacket::to_netascii(
              std::vector<char>(msg.value().begin(), msg.value().end()));
          bin_data.insert(bin_data.end(), msg_bin.begin(), msg_bin.end());
     }

     bin_data.push_back('\0');

     return bin_data;
}

ErrorPacket ErrorPacket::from_binary(const std::vector<char>& bin_data) {
     if (bin_data.size() < 4) {  // Min. size is 4B (2B opcode + 2B errcode)
          throw std::invalid_argument("Incorrect packet size");
     }

     /* Obtain and validate opcode */
     uint16_t opcode;
     std::memcpy(&opcode, bin_data.data(), sizeof(opcode));
     opcode = ntohs(opcode);
     if (opcode != static_cast<uint16_t>(TFTPOpcode::ERROR)) {
          throw std::invalid_argument("Incorrect opcode");
     }

     /* Obtain and validate error code */
     uint16_t errcode;
     std::memcpy(&errcode, bin_data.data() + 2, sizeof(errcode));
     errcode = ntohs(errcode);
     if (errcode > 7) {
          throw std::invalid_argument("Incorrect error code");
     }

     /* Obtain and validate error message */
     if (bin_data.size() > 5) {
          std::string msgStr;
          std::vector<char> msg_bin(bin_data.begin() + 4, bin_data.end() - 1);
          msg_bin = BasePacket::from_netascii(msg_bin);
          msgStr = std::string(msg_bin.begin(), msg_bin.end());
          return ErrorPacket(static_cast<TFTPErrorCode>(errcode), msgStr);
     }

     return ErrorPacket(static_cast<TFTPErrorCode>(errcode));
}