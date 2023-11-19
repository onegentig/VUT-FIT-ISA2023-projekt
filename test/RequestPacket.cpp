/**
 * @file test/RequestPacket.cpp
 * @author Onegen Something <xkrame00@vutbr.cz>
 * @brief RequestPacket (RRQ/WQR) unit tests
 * @date 2023-10-07
 */

#include "packet/RequestPacket.hpp"

#include "catch_amalgamated.hpp"

TEST_CASE("Request Packet Functionality", "[packet_rrq]") {
     SECTION("Default constructor init") {
          RequestPacket rp;
          REQUIRE(rp.get_opcode() == TFTPOpcode::RRQ);
          REQUIRE(rp.get_filename() == "");
          REQUIRE(rp.get_mode() == TFTPDataFormat::Octet);
          REQUIRE(rp.get_options_count() == 0);
     }

     SECTION("Parametrised constructor init") {
          RequestPacket rp_read(TFTPRequestType::Read, "example.txt",
                                TFTPDataFormat::Octet);
          REQUIRE(rp_read.get_opcode() == TFTPOpcode::RRQ);
          REQUIRE(rp_read.get_filename() == "example.txt");
          REQUIRE(rp_read.get_mode() == TFTPDataFormat::Octet);
          REQUIRE(rp_read.get_options_count() == 0);

          RequestPacket rp_write(TFTPRequestType::Write, "example.txt",
                                 TFTPDataFormat::Octet);
          REQUIRE(rp_write.get_opcode() == TFTPOpcode::WRQ);
          REQUIRE(rp_write.get_filename() == "example.txt");
          REQUIRE(rp_write.get_mode() == TFTPDataFormat::Octet);
          REQUIRE(rp_write.get_options_count() == 0);
          REQUIRE(rp_read != rp_write);

          RequestPacket rp_incomplete(TFTPRequestType::Read, "example.txt",
                                      TFTPDataFormat::NetASCII);
          REQUIRE(rp_incomplete.get_opcode() == TFTPOpcode::RRQ);
          REQUIRE(rp_incomplete.get_filename() == "example.txt");
          REQUIRE(rp_incomplete.get_mode() == TFTPDataFormat::NetASCII);
          REQUIRE(rp_incomplete.get_options_count() == 0);
     }

     SECTION("Setters and getters") {
          RequestPacket rp;
          REQUIRE(rp.get_filename() == "");
          REQUIRE(rp.get_mode() == TFTPDataFormat::Octet);

          rp.set_type(TFTPRequestType::Read);
          REQUIRE(rp.get_opcode() == TFTPOpcode::RRQ);
          rp.set_type(TFTPRequestType::Write);
          REQUIRE(rp.get_opcode() == TFTPOpcode::WRQ);

          rp.set_filename("test.txt");
          REQUIRE(rp.get_filename() == "test.txt");
          rp.set_mode(TFTPDataFormat::NetASCII);
          REQUIRE(rp.get_mode() == TFTPDataFormat::NetASCII);
     }

     SECTION("Serialisation and deserialisation") {
          std::string filename = "example.txt";
          std::string mode = "octet";  // case-insensitive test
          RequestPacket rp(TFTPRequestType::Read, filename,
                           TFTPDataFormat::Octet);

          // Packet -> Binary
          std::vector<char> binary = rp.to_binary();
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
          RequestPacket rp2 = RequestPacket::from_binary(binary);
          REQUIRE(rp2.get_opcode() == TFTPOpcode::RRQ);
          REQUIRE(rp2.get_filename() == filename);
          REQUIRE(rp2.get_mode() == TFTPDataFormat::Octet);
          REQUIRE(rp2.get_mode_str() == mode);
          REQUIRE(rp == rp2);
     }

     SECTION("Empty serialisation") {
          RequestPacket rp;
          std::vector<char> binary = rp.to_binary();
          REQUIRE(binary.size() == 0);
     }

     SECTION("Serialisation and deserialisation /w options") {
          std::string filename = "example.txt";
          std::string mode = "netascii";
          std::string opt_name, opt_val;
          RequestPacket rp(TFTPRequestType::Write, filename,
                           TFTPDataFormat::NetASCII);

          /* Set options */
          rp.add_option("blksize", "1432");
          REQUIRE(rp.get_options_count() == 1);
          rp.add_option("timeout", "5");
          REQUIRE(rp.get_options_count() == 2);
          rp.add_option("tsize", "123456789");
          REQUIRE(rp.get_options_count() == 3);
          rp.add_option("hakuna", "matata");
          REQUIRE(rp.get_options_count() == 4);

          // Packet -> Binary
          std::vector<char> binary = rp.to_binary();
          REQUIRE(binary[0] == 0x00);  // Opcode (HI)
          REQUIRE(binary[1] == 0x02);  // Opcode (LO)
          int offset = 2;
          std::string filename_bin(binary.begin() + offset,
                                   binary.begin() + offset + filename.length());
          REQUIRE(filename_bin == filename);  // Filename
          offset += filename.length();
          REQUIRE(binary[offset] == 0x00);  // Separator
          offset++;
          std::string mode_bin(binary.begin() + offset,
                               binary.begin() + offset + mode.length());
          REQUIRE(mode_bin == mode);  // Mode
          offset += mode.length();
          REQUIRE(binary[offset] == 0x00);  // Separator

          /* Options */
          offset++;
          offset = RequestPacket::findcstr(binary, offset, opt_name);
          REQUIRE(opt_name == "blksize");
          offset = RequestPacket::findcstr(binary, offset, opt_val);
          REQUIRE(opt_val == "1432");

          offset = RequestPacket::findcstr(binary, offset, opt_name);
          REQUIRE(opt_name == "timeout");
          offset = RequestPacket::findcstr(binary, offset, opt_val);
          REQUIRE(opt_val == "5");

          offset = RequestPacket::findcstr(binary, offset, opt_name);
          REQUIRE(opt_name == "tsize");
          offset = RequestPacket::findcstr(binary, offset, opt_val);
          REQUIRE(opt_val == "123456789");

          offset = RequestPacket::findcstr(binary, offset, opt_name);
          REQUIRE(opt_name == "hakuna");
          offset = RequestPacket::findcstr(binary, offset, opt_val);
          REQUIRE(opt_val == "matata");

          REQUIRE(binary[offset] == 0x00);  // Terminator

          // Binary -> Packet
          RequestPacket rp2 = RequestPacket::from_binary(binary);
          REQUIRE(rp2.get_opcode() == TFTPOpcode::WRQ);
          REQUIRE(rp2.get_filename() == filename);
          REQUIRE(rp2.get_mode() == TFTPDataFormat::NetASCII);
          REQUIRE(rp2.get_mode_str() == mode);
          REQUIRE(rp2.get_options_count() == 4);
          REQUIRE(rp == rp2);
     }
}