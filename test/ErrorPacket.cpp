/**
 * @file test/ErrorPacket.cpp
 * @author Onegen Something <xkrame00@vutbr.cz>
 * @brief ErrorPacket (ERROR) unit tests
 * @date 2023-10-07
 */

#include "packet/ErrorPacket.hpp"

#include "catch_amalgamated.hpp"

TEST_CASE("Error Packet Functionality", "[packet_rrq]") {
     SECTION("Default constructor init") {
          ErrorPacket ep;
          REQUIRE(ep.getOpcode() == TFTPOpcode::ERROR);
          REQUIRE(ep.getErrcode() == TFTPErrorCode::Unknown);
          REQUIRE(ep.getMessage() == std::nullopt);
     }

     SECTION("Parametrised constructor init") {
          ErrorPacket ep(TFTPErrorCode::FileNotFound, "File not found");
          REQUIRE(ep.getOpcode() == TFTPOpcode::ERROR);
          REQUIRE(ep.getErrcode() == TFTPErrorCode::FileNotFound);
          REQUIRE(ep.getMessage() == "File not found");

          ErrorPacket ep2(TFTPErrorCode::IllegalOperation);
          REQUIRE(ep2.getOpcode() == TFTPOpcode::ERROR);
          REQUIRE(ep2.getErrcode() == TFTPErrorCode::IllegalOperation);
          REQUIRE(ep2.getMessage() == std::nullopt);
     }

     SECTION("Setters and getters") {
          ErrorPacket ep;
          ep.setErrcode(TFTPErrorCode::NoSuchUser);
          REQUIRE(ep.getErrcode() == TFTPErrorCode::NoSuchUser);
          ep.setMessage("You don’t exist");
          REQUIRE(ep.getMessage() == "You don’t exist");

          ep.setErrcode(TFTPErrorCode::Unknown);
          REQUIRE(ep.getErrcode() == TFTPErrorCode::Unknown);
          ep.removeMessage();
          REQUIRE(ep.getMessage() == std::nullopt);
     }

     SECTION("Serialisation and deserialisation") {
          ErrorPacket ep(TFTPErrorCode::DiskFull, "I can't take it anymore");

          // Packet -> Binary
          std::vector<char> binary = ep.toBinary();
          REQUIRE(binary[0] == 0x00);  // Opcode (HI)
          REQUIRE(binary[1] == 0x05);  // Opcode (LO)
          REQUIRE(binary[2] == 0x00);  // Error code (HI)
          REQUIRE(binary[3] == 0x03);  // Error code (LO)
          int offset = 4;
          std::string msgBin(binary.begin() + offset,
                             binary.begin() + offset + 23);
          REQUIRE(msgBin == "I can't take it anymore");  // Message
          ep.hexdump();

          // Binary -> Packet
          ErrorPacket ep2;
          ep2.fromBinary(binary);
          REQUIRE(ep2.getOpcode() == TFTPOpcode::ERROR);
          REQUIRE(ep2.getErrcode() == TFTPErrorCode::DiskFull);
          REQUIRE(ep2.getMessage().value() == "I can't take it anymore");
          REQUIRE(ep == ep2);
     }

     SECTION("Empty serialisation") {
          ErrorPacket ep;
          std::vector<char> binary = ep.toBinary();
          REQUIRE(binary.size() == 5);  // OP OP ERR ERR 00
     }
}