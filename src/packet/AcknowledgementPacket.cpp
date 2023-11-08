/**
 * @file AcknowledgementPacket.hpp
 * @author Filip J. Kramec <xkrame00@vutbr.cz>
 * @brief TFTP acknowledgement packet.
 * @date 2023-09-28
 */

#include "packet/AcknowledgementPacket.hpp"

/* === Constructors === */

AcknowledgementPacket::AcknowledgementPacket() : block_n(0) {
     opcode = TFTPOpcode::ACK;
}

AcknowledgementPacket::AcknowledgementPacket(uint16_t block_n) : block_n(block_n) {
     opcode = TFTPOpcode::ACK;
}

/* === Core Methods === */

std::vector<char> AcknowledgementPacket::to_binary() const {
     std::vector<char> binaryData(4);

     /* Convert data to network byte order */
     uint16_t opcode = htons(static_cast<uint16_t>(this->opcode));
     uint16_t block_n = htons(this->block_n);

     /* Copy binary data to vector */
     std::memcpy(binaryData.data(), &opcode, sizeof(opcode));
     std::memcpy(binaryData.data() + 2, &block_n, sizeof(block_n));

     return binaryData;
}

void AcknowledgementPacket::from_binary(const std::vector<char>& binaryData) {
     if (binaryData.size() != 4)
          throw std::invalid_argument("Incorrect packet size");

     /* Copy binary data to variables */
     uint16_t opcode, block_n;
     std::memcpy(&opcode, binaryData.data(), sizeof(opcode));
     std::memcpy(&block_n, binaryData.data() + 2, sizeof(block_n));

     /* Convert data to host byte order */
     opcode = ntohs(opcode);
     block_n = ntohs(block_n);

     /* Validate opcode */
     if (opcode != static_cast<uint16_t>(TFTPOpcode::ACK))
          throw std::invalid_argument("Incorrect opcode");

     /* Set variables */
     this->opcode = static_cast<TFTPOpcode>(opcode);
     this->block_n = block_n;
}
