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
          REQUIRE(ep.get_opcode() == TFTPOpcode::ERROR);
          REQUIRE(ep.get_errcode() == TFTPErrorCode::Unknown);
          REQUIRE(ep.get_message() == std::nullopt);
     }

     SECTION("Parametrised constructor init") {
          ErrorPacket ep(TFTPErrorCode::FileNotFound, "File not found");
          REQUIRE(ep.get_opcode() == TFTPOpcode::ERROR);
          REQUIRE(ep.get_errcode() == TFTPErrorCode::FileNotFound);
          REQUIRE(ep.get_message() == "File not found");

          ErrorPacket ep2(TFTPErrorCode::IllegalOperation);
          REQUIRE(ep2.get_opcode() == TFTPOpcode::ERROR);
          REQUIRE(ep2.get_errcode() == TFTPErrorCode::IllegalOperation);
          REQUIRE(ep2.get_message() == std::nullopt);
     }

     SECTION("Setters and getters") {
          ErrorPacket ep;
          ep.set_errcode(TFTPErrorCode::NoSuchUser);
          REQUIRE(ep.get_errcode() == TFTPErrorCode::NoSuchUser);
          ep.set_message("You don’t exist");
          REQUIRE(ep.get_message() == "You don’t exist");

          ep.set_errcode(TFTPErrorCode::Unknown);
          REQUIRE(ep.get_errcode() == TFTPErrorCode::Unknown);
          ep.remove_message();
          REQUIRE(ep.get_message() == std::nullopt);
     }

     SECTION("Serialisation and deserialisation") {
          ErrorPacket ep(TFTPErrorCode::DiskFull, "I can't take it anymore");

          // Packet -> Binary
          std::vector<char> binary = ep.to_binary();
          REQUIRE(binary[0] == 0x00);  // Opcode (HI)
          REQUIRE(binary[1] == 0x05);  // Opcode (LO)
          REQUIRE(binary[2] == 0x00);  // Error code (HI)
          REQUIRE(binary[3] == 0x03);  // Error code (LO)
          int offset = 4;
          std::string msgBin(binary.begin() + offset,
                             binary.begin() + offset + 23);
          REQUIRE(msgBin == "I can't take it anymore");  // Message

          // Binary -> Packet
          ErrorPacket ep2 = ErrorPacket::from_binary(binary);
          REQUIRE(ep2.get_opcode() == TFTPOpcode::ERROR);
          REQUIRE(ep2.get_errcode() == TFTPErrorCode::DiskFull);
          REQUIRE(ep2.get_message() == "I can't take it anymore");
          REQUIRE(ep == ep2);
     }

     SECTION("Empty serialisation") {
          ErrorPacket ep;
          std::vector<char> binary = ep.to_binary();
          REQUIRE(binary.size() == 5);  // OP OP ERR ERR 00
     }
}