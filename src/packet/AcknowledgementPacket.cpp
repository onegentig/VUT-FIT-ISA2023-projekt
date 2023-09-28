/**
 * @file AcknowledgementPacket.hpp
 * @author Filip J. Kramec <xkrame00@vutbr.cz>
 * @brief TFTP acknowledgement packet.
 * @date 2023-09-28
 */

#include "packet/AcknowledgementPacket.hpp"

/* === Constructors === */

AcknowledgementPacket::AcknowledgementPacket() : blockN(0) {
     opcode = TFTPOpcode::ACK;
}

AcknowledgementPacket::AcknowledgementPacket(uint16_t blockN) : blockN(blockN) {
     opcode = TFTPOpcode::ACK;
}

/* === Core Methods === */

std::vector<char> AcknowledgementPacket::toBinary() const {
     std::vector<char> binaryData(4);

     /* Convert data to network byte order */
     uint16_t opcode = htons(static_cast<uint16_t>(this->opcode));
     uint16_t blockN = htons(this->blockN);

     /* Copy binary data to vector */
     std::memcpy(binaryData.data(), &opcode, sizeof(opcode));
     std::memcpy(binaryData.data() + 2, &blockN, sizeof(blockN));

     return binaryData;
}

void AcknowledgementPacket::fromBinary(const std::vector<char>& binaryData) {
     if (binaryData.size() != 4)
          throw std::invalid_argument("Incorrect packet size");

     /* Copy binary data to variables */
     uint16_t opcode, blockN;
     std::memcpy(&opcode, binaryData.data(), sizeof(opcode));
     std::memcpy(&blockN, binaryData.data() + 2, sizeof(blockN));

     /* Convert data to host byte order */
     opcode = ntohs(opcode);
     blockN = ntohs(blockN);

     /* Validate opcode */
     if (opcode != static_cast<uint16_t>(TFTPOpcode::ACK))
          throw std::invalid_argument("Incorrect opcode");

     /* Set variables */
     this->opcode = static_cast<TFTPOpcode>(opcode);
     this->blockN = blockN;
}

/* === Debugging Methods === */

void AcknowledgementPacket::dprint() const {
     auto opcodeStr = dprint2BNum(opcode);
     auto blockStr = dprint2BNum(blockN);

     std::cout << " -------------------\n"
               << "|  " << opcodeStr << "  |  " << blockStr << " |\n"
               << " -------------------\n";
}
