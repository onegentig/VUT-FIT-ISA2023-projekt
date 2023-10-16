/**
 * @file test/RequestPacket.cpp
 * @author Filip J. Kramec <xkrame00@vutbr.cz>
 * @brief RequestPacket (RRQ/WQR) unit tests
 * @date 2023-10-07
 */

#include "packet/RequestPacket.hpp"

#include "catch_amalgamated.hpp"

TEST_CASE("Request Packet Functionality", "[packet_rrq]") {
     SECTION("Default constructor init") {
          RequestPacket rp;
          REQUIRE(rp.getOpcode() == TFTPOpcode::RRQ);
          REQUIRE(rp.getFilename() == "");
          REQUIRE(rp.getMode() == TFTPDataFormat::Octet);
     }

     SECTION("Parametrised constructor init") {
          RequestPacket rp_read(RequestPacketType::Read, "example.txt",
                                TFTPDataFormat::Octet);
          REQUIRE(rp_read.getOpcode() == TFTPOpcode::RRQ);
          REQUIRE(rp_read.getFilename() == "example.txt");
          REQUIRE(rp_read.getMode() == TFTPDataFormat::Octet);

          RequestPacket rp_write(RequestPacketType::Write, "example.txt",
                                 TFTPDataFormat::Octet);
          REQUIRE(rp_write.getOpcode() == TFTPOpcode::WRQ);
          REQUIRE(rp_write.getFilename() == "example.txt");
          REQUIRE(rp_write.getMode() == TFTPDataFormat::Octet);
          REQUIRE(rp_read != rp_write);

          RequestPacket rp_incomplete(RequestPacketType::Read, "example.txt",
                                      TFTPDataFormat::NetASCII);
          REQUIRE(rp_incomplete.getOpcode() == TFTPOpcode::RRQ);
          REQUIRE(rp_incomplete.getFilename() == "example.txt");
          REQUIRE(rp_incomplete.getMode() == TFTPDataFormat::NetASCII);
     }

     SECTION("Setters and getters") {
          RequestPacket rp;
          REQUIRE(rp.getFilename() == "");
          REQUIRE(rp.getMode() == TFTPDataFormat::Octet);

          rp.setType(RequestPacketType::Read);
          REQUIRE(rp.getOpcode() == TFTPOpcode::RRQ);
          rp.setType(RequestPacketType::Write);
          REQUIRE(rp.getOpcode() == TFTPOpcode::WRQ);

          rp.setFilename("test.txt");
          REQUIRE(rp.getFilename() == "test.txt");
          rp.setMode(TFTPDataFormat::NetASCII);
          REQUIRE(rp.getMode() == TFTPDataFormat::NetASCII);
     }

     SECTION("Serialisation and deserialisation") {
          std::string filename = "example.txt";
          std::string mode = "octet";
          RequestPacket rp(RequestPacketType::Read, filename,
                           TFTPDataFormat::Octet);

          // Packet -> Binary
          std::vector<char> binary = rp.toBinary();
          REQUIRE(binary[0] == 0x00);  // Opcode (HI)
          REQUIRE(binary[1] == 0x01);  // Opcode (LO)
          int offset = 2;
          std::string filename_bin(binary.begin() + offset,
                                   binary.begin() + offset + filename.length());
          REQUIRE(filename_bin == filename);  // Filename
          offset += filename.length();
          REQUIRE(binary[offset] == 0x00);  // Separator
          offset += 1;
          std::string mode_bin(binary.begin() + offset,
                               binary.begin() + offset + mode.length());
          REQUIRE(mode_bin == mode);  // Mode
          offset += mode.length();
          REQUIRE(binary[offset] == 0x00);  // Terminator

          // Binary -> Packet
          RequestPacket rp2;
          rp2.fromBinary(binary);
          REQUIRE(rp2.getOpcode() == TFTPOpcode::RRQ);
          REQUIRE(rp2.getFilename() == filename);
          REQUIRE(rp2.getMode() == TFTPDataFormat::Octet);
          REQUIRE(rp2.getModeStr() == mode);
          REQUIRE(rp == rp2);
     }

     SECTION("Empty serialisation") {
          RequestPacket rp;
          std::vector<char> binary = rp.toBinary();
          REQUIRE(binary.size() == 0);
     }
}