/**
 * @file test/DataPacket.cpp
 * @author Filip J. Kramec <xkrame00@vutbr.cz>
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
          REQUIRE(buf.size() == 3);
          REQUIRE(std::string(buf.begin(), buf.end()) == "abc");
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
          REQUIRE(buf.size() == 8);  // TODO
          REQUIRE(std::string(buf.begin(), buf.end()) == "\r\n\r\n\r\n\r\n");
          close(fd_newlines);

          /* Long text with newlines */
          // int fd_long = open("test/files/long.txt", O_RDONLY); // TODO
     }

     /*SECTION("Serialisation and deserialisation") {
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
          REQUIRE(rp == rp2);
     }

     SECTION("Empty serialisation") {
          RequestPacket rp;
          std::vector<char> binary = rp.toBinary();
          REQUIRE(binary.size() == 0);
     }*/
}