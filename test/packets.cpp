#include "catch_amalgamated.hpp"
#include "packet/AcknowledgementPacket.hpp"
#include "packet/RequestPacket.hpp"

/* === RRQ Packet === */
// opcode: 01 02

TEST_CASE("Request Packet Functionality", "[packet_rrq]") {
     SECTION("Default constructor init") {
          RequestPacket rp;
          REQUIRE(rp.getOpcode() == TFTPOpcode::RRQ);
          REQUIRE(rp.getFilename() == "");
          REQUIRE(rp.getMode() == "");
     }

     SECTION("Parametrised constructor init") {
          RequestPacket rp_read(RequestPacketType::Read, "test.txt", "octet");
          REQUIRE(rp_read.getOpcode() == TFTPOpcode::RRQ);
          REQUIRE(rp_read.getFilename() == "test.txt");
          REQUIRE(rp_read.getMode() == "octet");

          RequestPacket rp_write(RequestPacketType::Write, "test.txt", "octet");
          REQUIRE(rp_write.getOpcode() == TFTPOpcode::WRQ);
          REQUIRE(rp_write.getFilename() == "test.txt");
          REQUIRE(rp_write.getMode() == "octet");

          RequestPacket rp_incomplete(RequestPacketType::Read, "test.txt", "");
          REQUIRE(rp_incomplete.getOpcode() == TFTPOpcode::RRQ);
          REQUIRE(rp_incomplete.getFilename() == "test.txt");
          REQUIRE(rp_incomplete.getMode() == "");
     }

     // SECTION("Invalid parameters") {
     //      RequestPacket rp;
     //      REQUIRE_THROWS(rp.setMode("magic"));
     //      REQUIRE_THROWS(RequestPacket(RequestPacketType::Read, "",
     //      "magic"));
     // }

     SECTION("Setters and getters") {
          RequestPacket rp;
          REQUIRE(rp.getFilename() == "");
          REQUIRE(rp.getMode() == "");

          rp.setType(RequestPacketType::Read);
          REQUIRE(rp.getOpcode() == TFTPOpcode::RRQ);
          rp.setType(RequestPacketType::Write);
          REQUIRE(rp.getOpcode() == TFTPOpcode::WRQ);

          rp.setFilename("test.txt");
          REQUIRE(rp.getFilename() == "test.txt");
          rp.setMode("octet");
          REQUIRE(rp.getMode() == "octet");
     }

     SECTION("Serialisation and deserialisation") {
          std::string filename = "example.txt";
          std::string mode = "octet";
          RequestPacket rp(RequestPacketType::Read, filename, mode);

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
          REQUIRE(rp2.getMode() == mode);
     }
}