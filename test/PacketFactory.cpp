/**
 * @file test/DataPacket.cpp
 * @author Onegen Something <xkrame00@vutbr.cz>
 * @brief Packet factory tests
 * @date 2023-11-08
 */

#include "packet/PacketFactory.hpp"

#include "catch_amalgamated.hpp"

TEST_CASE("Packet factory creation", "[packet_factory]") {
     SECTION("Create RRQ packet") {
          std::vector<char> binary = {0x00, 0x01};
          std::string filename = "test.txt";
          std::string mode = "octet";

          std::vector<char> filename_vec(filename.begin(), filename.end());
          std::vector<char> mode_vec(mode.begin(), mode.end());
          filename_vec = BasePacket::to_netascii(filename_vec);
          mode_vec = BasePacket::to_netascii(mode_vec);
          filename_vec.push_back('\0');
          mode_vec.push_back('\0');

          binary.insert(binary.end(), filename_vec.begin(), filename_vec.end());
          binary.insert(binary.end(), mode_vec.begin(), mode_vec.end());

          std::unique_ptr<BasePacket> packet = PacketFactory::create(binary);
          REQUIRE(packet->get_opcode() == TFTPOpcode::RRQ);
          std::cout << packet->to_binary().size() << std::endl;
          REQUIRE(packet->to_binary() == binary);
     }

     SECTION("Create WRQ packet") {
          std::vector<char> binary = {0x00, 0x02};
          std::string filename = "example.txt";
          std::string mode = "netascii";

          std::vector<char> filename_vec(filename.begin(), filename.end());
          std::vector<char> mode_vec(mode.begin(), mode.end());
          filename_vec = BasePacket::to_netascii(filename_vec);
          mode_vec = BasePacket::to_netascii(mode_vec);
          filename_vec.push_back('\0');
          mode_vec.push_back('\0');

          binary.insert(binary.end(), filename_vec.begin(), filename_vec.end());
          binary.insert(binary.end(), mode_vec.begin(), mode_vec.end());

          std::unique_ptr<BasePacket> packet = PacketFactory::create(binary);
          REQUIRE(packet->get_opcode() == TFTPOpcode::WRQ);
          REQUIRE(packet->to_binary() == binary);
     }

     SECTION("Create DATA packet") {
          std::vector<char> binary = {0x00, 0x03, 0x00, 0x01, 0x00, 0x00};
          std::unique_ptr<BasePacket> packet = PacketFactory::create(binary);
          REQUIRE(packet->get_opcode() == TFTPOpcode::DATA);
          REQUIRE(packet->to_binary() == binary);
     }

     SECTION("Create ACK packet") {
          std::vector<char> binary = {0x00, 0x04, 0x00, 0x01};
          std::unique_ptr<BasePacket> packet = PacketFactory::create(binary);
          REQUIRE(packet->get_opcode() == TFTPOpcode::ACK);
          REQUIRE(packet->to_binary() == binary);
     }

     SECTION("Create ERROR packet") {
          std::vector<char> binary = {0x00, 0x05, 0x00, 0x02, 0x00};
          std::unique_ptr<BasePacket> packet = PacketFactory::create(binary);
          REQUIRE(packet->get_opcode() == TFTPOpcode::ERROR);
          REQUIRE(packet->to_binary() == binary);
     }

     SECTION("Create ERROR packet with message") {
          std::vector<char> binary = {0x00, 0x05, 0x00, 0x01, 0x00, 0x00};
          std::string message = "Test message";
          std::vector<char> message_vec(message.begin(), message.end());
          message_vec = BasePacket::to_netascii(message_vec);
          message_vec.push_back('\0');
          binary.insert(binary.end(), message_vec.begin(), message_vec.end());

          std::unique_ptr<BasePacket> packet = PacketFactory::create(binary);
          REQUIRE(packet->get_opcode() == TFTPOpcode::ERROR);
          REQUIRE(packet->to_binary() == binary);
     }

     SECTION("Create invalid packet") {
          std::vector<char> binary = {0x00, 0x06, 0x00, 0x01, 0x00, 0x00};
          std::unique_ptr<BasePacket> packet = PacketFactory::create(binary);
          REQUIRE(packet == nullptr);
     }
}