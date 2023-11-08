/**
 * @file test/AcknowledgementPacket.cpp
 * @author Filip J. Kramec <xkrame00@vutbr.cz>
 * @brief Acknowledgement (ACK) unit tests
 * @date 2023-10-07
 */

#include "packet/AcknowledgementPacket.hpp"

#include "catch_amalgamated.hpp"

TEST_CASE("Acknowledgement Packet Functionality", "[packet_rrq]") {
     SECTION("Default constructor init") {
          AcknowledgementPacket ap;
          REQUIRE(ap.get_opcode() == TFTPOpcode::ACK);
          REQUIRE(ap.get_block_number() == 0);
     }

     SECTION("Parametrised constructor init") {
          AcknowledgementPacket ap(1);
          REQUIRE(ap.get_opcode() == TFTPOpcode::ACK);
          REQUIRE(ap.get_block_number() == 0x0001);

          AcknowledgementPacket ap2(0xFFFF);
          REQUIRE(ap2.get_opcode() == TFTPOpcode::ACK);
          REQUIRE(ap2.get_block_number() == 0xFFFF);
     }

     SECTION("Setters and getters") {
          AcknowledgementPacket ap;
          AcknowledgementPacket ap2;
          REQUIRE(ap.get_block_number() == 0);
          REQUIRE(ap == ap2);

          ap.set_block_number(0x0001);
          REQUIRE(ap.get_block_number() == 0x0001);
          ap.set_block_number(0xFFFF);
          REQUIRE(ap.get_block_number() == 0xFFFF);
          REQUIRE(ap != ap2);
     }

     SECTION("Serialisation and deserialisation") {
          int block_n = 50;
          AcknowledgementPacket ap(block_n);

          // Packet -> Binary
          std::vector<char> binary = ap.to_binary();
          REQUIRE(binary[0] == 0x00);  // Opcode (HI)
          REQUIRE(binary[1] == 0x04);  // Opcode (LO)
          uint16_t blockN_bin = (binary[2] << 8) | binary[3];
          REQUIRE(blockN_bin == block_n);

          // Binary -> Packet
          AcknowledgementPacket ap2;
          ap2.from_binary(binary);
          REQUIRE(ap2.get_opcode() == TFTPOpcode::ACK);
          REQUIRE(ap2.get_block_number() == block_n);
          REQUIRE(ap == ap2);
     }

     SECTION("Empty serialisation") {
          AcknowledgementPacket ap;
          std::vector<char> binary = ap.to_binary();
          REQUIRE(binary.size() == 4);
          REQUIRE(binary[0] == 0x00);  // Opcode (HI)
          REQUIRE(binary[1] == 0x04);  // Opcode (LO)
          REQUIRE(binary[2] == 0x00);  // Block number (HI)
          REQUIRE(binary[3] == 0x00);  // Block number (LO)
     }
}