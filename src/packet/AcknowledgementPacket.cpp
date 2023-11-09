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

AcknowledgementPacket::AcknowledgementPacket(uint16_t block_n)
    : block_n(block_n) {
     opcode = TFTPOpcode::ACK;
}

/* === Core Methods === */

std::vector<char> AcknowledgementPacket::to_binary() const {
     std::vector<char> bin_data(4);

     /* Convert data to network byte order */
     uint16_t opcode = htons(static_cast<uint16_t>(this->opcode));
     uint16_t block_n = htons(this->block_n);

     /* Copy binary data to vector */
     std::memcpy(bin_data.data(), &opcode, sizeof(opcode));
     std::memcpy(bin_data.data() + 2, &block_n, sizeof(block_n));

     return bin_data;
}

AcknowledgementPacket AcknowledgementPacket::from_binary(
    const std::vector<char>& bin_data) {
     if (bin_data.size() != 4)
          throw std::invalid_argument("Incorrect packet size");

     /* Copy binary data to variables */
     uint16_t opcode;
     uint16_t block_n;
     std::memcpy(&opcode, bin_data.data(), sizeof(opcode));
     std::memcpy(&block_n, bin_data.data() + 2, sizeof(block_n));

     /* Convert data to host byte order */
     opcode = ntohs(opcode);
     block_n = ntohs(block_n);

     /* Validate opcode */
     if (opcode != static_cast<uint16_t>(TFTPOpcode::ACK))
          throw std::invalid_argument("Incorrect opcode");

     return AcknowledgementPacket(block_n);
}
