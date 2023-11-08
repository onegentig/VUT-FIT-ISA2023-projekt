/**
 * @file test/DataPacket.cpp
 * @author Onegen Something <xkrame00@vutbr.cz>
 * @brief DataPacket (DATA) unit tests
 * @date 2023-10-15
 */

#include "packet/DataPacket.hpp"

#include "catch_amalgamated.hpp"

TEST_CASE("Data Packet Functionality", "[packet_data]") {
     SECTION("Default constructor init") {
          DataPacket dp;
          REQUIRE(dp.get_opcode() == TFTPOpcode::DATA);
          REQUIRE(dp.get_fd() == -1);
          REQUIRE(dp.get_block_number() == 0);
          REQUIRE(dp.get_data().size() == 0);
          REQUIRE(dp.get_mode() == TFTPDataFormat::Octet);
     }

     SECTION("Parametrised constructor init") {
          /* Raw data */
          DataPacket dp_raw(std::vector<char>(1023, 0x01), 1);
          REQUIRE(dp_raw.get_opcode() == TFTPOpcode::DATA);
          REQUIRE(dp_raw.get_fd() == -1);
          REQUIRE(dp_raw.get_block_number() == 1);
          REQUIRE(dp_raw.get_data().size() == 1023);
          REQUIRE(dp_raw.read_data().size() == TFTP_MAX_DATA);
          REQUIRE(dp_raw.get_mode() == TFTPDataFormat::Octet);

          /* File descriptor */
          int fd = open("test/files/abc.txt", O_RDONLY);
          REQUIRE(fd != -1);
          DataPacket dp_fd(fd, 1);
          REQUIRE(dp_fd.get_opcode() == TFTPOpcode::DATA);
          REQUIRE(dp_fd.get_fd() == fd);
          REQUIRE(dp_fd.get_block_number() == 1);
          REQUIRE(dp_fd.get_data().size() == 0);
          REQUIRE(dp_fd.read_data().size() == 3);
          REQUIRE(dp_fd.get_mode() == TFTPDataFormat::Octet);
          close(fd);

          /* Path */
          DataPacket dp_path("test/files/abc.txt", 1);
          REQUIRE(dp_path.get_opcode() == TFTPOpcode::DATA);
          REQUIRE(dp_path.get_fd() != -1);
          REQUIRE(dp_path.get_block_number() == 1);
          REQUIRE(dp_path.get_data().size() == 0);
          REQUIRE(dp_path.read_data().size() == 3);
          REQUIRE(dp_path.get_mode() == TFTPDataFormat::Octet);
          close(dp_path.get_fd());
     }

     SECTION("Setters and getters") {
          DataPacket dp;
          REQUIRE(dp.get_block_number() == 0);
          REQUIRE(dp.get_data().size() == 0);

          dp.set_block_number(100);
          REQUIRE(dp.get_block_number() == 100);
          dp.set_block_number(0);
          REQUIRE(dp.get_block_number() == 0);
          dp.set_block_number(1);
          REQUIRE(dp.get_block_number() == 1);

          dp.set_data(std::vector<char>(1023, 0x01));
          REQUIRE(dp.get_data().size() == 1023);
          REQUIRE(dp.read_data().size() == TFTP_MAX_DATA);
          dp.set_data(std::vector<char>());
          REQUIRE(dp.get_data().empty());
          REQUIRE(dp.get_data().size() == 0);
          REQUIRE(dp.read_data().size() == 0);
          int fd = open("test/files/abc.txt", O_RDONLY);
          REQUIRE(fd != -1);
          dp.set_fd(fd);
          REQUIRE(dp.get_fd() == fd);
          REQUIRE(dp.read_data().size() == 3);
          REQUIRE(dp.get_data().size() == 0);
          close(fd);
     }

     SECTION("Encoding") {
          DataPacket dp;
          std::vector<char> buf;

          /* "abc" string */
          int fd_abc = open("test/files/abc.txt", O_RDONLY);
          REQUIRE(fd_abc != -1);
          dp.set_fd(fd_abc);
          dp.set_block_number(1);
          dp.set_mode(TFTPDataFormat::Octet);
          buf = dp.read_data();
          REQUIRE(buf.size() == 3);
          REQUIRE(std::string(buf.begin(), buf.end()) == "abc");
          dp.set_mode(TFTPDataFormat::NetASCII);
          buf = dp.read_data();
          REQUIRE(buf.size() == 4);  // 3 + 1 for null terminator
          REQUIRE(buf == std::vector<char>{'a', 'b', 'c', '\0'});
          close(fd_abc);

          /* 4 newlines */
          int fd_newlines = open("test/files/newlines.txt", O_RDONLY);
          REQUIRE(fd_newlines != -1);
          dp.set_fd(fd_newlines);
          dp.set_block_number(1);
          dp.set_mode(TFTPDataFormat::Octet);
          buf = dp.read_data();
          REQUIRE(buf.size() == 4);
          REQUIRE(std::string(buf.begin(), buf.end()) == "\n\n\n\n");
          dp.set_mode(TFTPDataFormat::NetASCII);
          buf = dp.read_data();
          REQUIRE(buf.size() == 9);  // 4 * 2 (LF->CRLF) + 1 (null terminator)
          REQUIRE(buf
                  == std::vector<char>{'\r', '\n', '\r', '\n', '\r', '\n', '\r',
                                       '\n', '\0'});
          close(fd_newlines);
     }

     SECTION("Serialisation and deserialisation") {
          std::string filename = "test/files/abc.txt";
          DataPacket dp(filename, 1);
          dp.set_mode(TFTPDataFormat::NetASCII);

          // Packet -> Binary
          std::vector<char> binary = dp.to_binary();
          REQUIRE(binary[0] == 0x00);  // Opcode (HI)
          REQUIRE(binary[1] == 0x03);  // Opcode (LO)
          int offset = 2;
          REQUIRE(binary[offset] == 0x00);      // Block number (HI)
          REQUIRE(binary[offset + 1] == 0x01);  // Block number (LO)
          offset += 2;
          std::string data_bin(binary.begin() + offset,
                               binary.begin() + offset + 3);
          REQUIRE(data_bin == "abc");  // Data
          REQUIRE(binary[offset + 3]
                  == '\0');  // Null terminator (because NetASCII)
          REQUIRE(binary.size()
                  == 8);  // 2 (op) + 2 (block) + 4 (data incl. \0)

          // Binary -> Packet
          DataPacket dp2;
          dp2.from_binary(binary, TFTPDataFormat::NetASCII);
          REQUIRE(dp2.get_opcode() == TFTPOpcode::DATA);
          REQUIRE(dp2.get_block_number() == 1);
          REQUIRE(dp2.get_data().size() == 4);
          REQUIRE(dp2.read_data().size() == 4);
          REQUIRE(dp == dp2);
     }

     SECTION("Empty serialisation") {
          DataPacket dp;
          std::vector<char> binary = dp.to_binary();
          REQUIRE(binary.size() == 0);
     }
}