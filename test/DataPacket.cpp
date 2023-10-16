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
          REQUIRE(dp.getOpcode() == TFTPOpcode::DATA);
          REQUIRE(dp.getFd() == -1);
          REQUIRE(dp.getBlockNumber() == 0);
          REQUIRE(dp.getData().size() == 0);
          REQUIRE(dp.getMode() == DataFormat::Octet);
     }

     SECTION("Parametrised constructor init") {
          /* Raw data */
          DataPacket dp_raw(std::vector<char>(1023, 0x01), 1);
          REQUIRE(dp_raw.getOpcode() == TFTPOpcode::DATA);
          REQUIRE(dp_raw.getFd() == -1);
          REQUIRE(dp_raw.getBlockNumber() == 1);
          REQUIRE(dp_raw.getData().size() == 1023);
          REQUIRE(dp_raw.readData().size() == MAX_DATA_SIZE);
          REQUIRE(dp_raw.getMode() == DataFormat::Octet);

          /* File descriptor */
          int fd = open("test/files/abc.txt", O_RDONLY);
          REQUIRE(fd != -1);
          DataPacket dp_fd(fd, 1);
          REQUIRE(dp_fd.getOpcode() == TFTPOpcode::DATA);
          REQUIRE(dp_fd.getFd() == fd);
          REQUIRE(dp_fd.getBlockNumber() == 1);
          REQUIRE(dp_fd.getData().size() == 0);
          REQUIRE(dp_fd.readData().size() == 3);
          REQUIRE(dp_fd.getMode() == DataFormat::Octet);
          close(fd);

          /* Path */
          DataPacket dp_path("test/files/abc.txt", 1);
          REQUIRE(dp_path.getOpcode() == TFTPOpcode::DATA);
          REQUIRE(dp_path.getFd() != -1);
          REQUIRE(dp_path.getBlockNumber() == 1);
          REQUIRE(dp_path.getData().size() == 0);
          REQUIRE(dp_path.readData().size() == 3);
          REQUIRE(dp_path.getMode() == DataFormat::Octet);
          close(dp_path.getFd());
     }

     SECTION("Setters and getters") {
          DataPacket dp;
          REQUIRE(dp.getBlockNumber() == 0);
          REQUIRE(dp.getData().size() == 0);

          dp.setBlockNumber(100);
          REQUIRE(dp.getBlockNumber() == 100);
          dp.setBlockNumber(0);
          REQUIRE(dp.getBlockNumber() == 0);
          dp.setBlockNumber(1);
          REQUIRE(dp.getBlockNumber() == 1);

          dp.setData(std::vector<char>(1023, 0x01));
          REQUIRE(dp.getData().size() == 1023);
          REQUIRE(dp.readData().size() == MAX_DATA_SIZE);
          dp.setData(std::vector<char>());
          REQUIRE(dp.getData().empty());
          REQUIRE(dp.getData().size() == 0);
          REQUIRE(dp.readData().size() == 0);
          int fd = open("test/files/abc.txt", O_RDONLY);
          REQUIRE(fd != -1);
          dp.setFd(fd);
          REQUIRE(dp.getFd() == fd);
          REQUIRE(dp.readData().size() == 3);
          REQUIRE(dp.getData().size() == 0);
          close(fd);
     }

     SECTION("Encoding") {
          DataPacket dp;
          std::vector<char> buf;

          /* "abc" string */
          int fd_abc = open("test/files/abc.txt", O_RDONLY);
          REQUIRE(fd_abc != -1);
          dp.setFd(fd_abc);
          dp.setBlockNumber(1);
          dp.setMode(DataFormat::Octet);
          buf = dp.readData();
          REQUIRE(buf.size() == 3);
          REQUIRE(std::string(buf.begin(), buf.end()) == "abc");
          dp.setMode(DataFormat::NetASCII);
          buf = dp.readData();
          REQUIRE(buf.size() == 4);  // 3 + 1 for null terminator
          REQUIRE(buf == std::vector<char>{'a', 'b', 'c', '\0'});
          close(fd_abc);

          /* 4 newlines */
          int fd_newlines = open("test/files/newlines.txt", O_RDONLY);
          REQUIRE(fd_newlines != -1);
          dp.setFd(fd_newlines);
          dp.setBlockNumber(1);
          dp.setMode(DataFormat::Octet);
          buf = dp.readData();
          REQUIRE(buf.size() == 4);
          REQUIRE(std::string(buf.begin(), buf.end()) == "\n\n\n\n");
          dp.setMode(DataFormat::NetASCII);
          buf = dp.readData();
          REQUIRE(buf.size() == 9);  // 4 * 2 (LF->CRLF) + 1 (null terminator)
          REQUIRE(buf
                  == std::vector<char>{'\r', '\n', '\r', '\n', '\r', '\n', '\r',
                                       '\n', '\0'});
          close(fd_newlines);
     }

     SECTION("Serialisation and deserialisation") {
          std::string filename = "test/files/abc.txt";
          DataPacket dp(filename, 1);
          dp.setMode(DataFormat::NetASCII);

          // Packet -> Binary
          std::vector<char> binary = dp.toBinary();
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
          dp2.fromBinary(binary, DataFormat::NetASCII);
          REQUIRE(dp2.getOpcode() == TFTPOpcode::DATA);
          REQUIRE(dp2.getBlockNumber() == 1);
          REQUIRE(dp2.getData().size() == 4);
          REQUIRE(dp2.readData().size() == 4);
          REQUIRE(dp == dp2);
     }

     SECTION("Empty serialisation") {
          DataPacket dp;
          std::vector<char> binary = dp.toBinary();
          REQUIRE(binary.size() == 0);
     }
}