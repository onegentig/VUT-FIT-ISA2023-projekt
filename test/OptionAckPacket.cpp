/**
 * @file test/OptionAckPacket.cpp
 * @author Filip J. Kramec <xkrame00@vutbr.cz>
 * @brief OptionAckPacket (OACK) unit tests
 * @date 2023-11-19
 */

#include "packet/OptionAckPacket.hpp"

#include "catch_amalgamated.hpp"

TEST_CASE("OACK Packet Functionality", "[packet_oack]") {
     SECTION("Default constructor init") {
          OptionAckPacket oap;
          REQUIRE(oap.get_opcode() == TFTPOpcode::OACK);
          REQUIRE(oap.get_options_count() == 0);
          REQUIRE(oap.get_options().empty());
     }

     SECTION("Parametrised constructor init") {
          std::vector<std::pair<std::string, std::string>> opt_vec;
          opt_vec.push_back(std::make_pair("blksize", "1432"));
          opt_vec.push_back(std::make_pair("timeout", "5"));
          opt_vec.push_back(std::make_pair("tsize", "123456789"));
          opt_vec.push_back(std::make_pair("hakuna", "matata"));

          OptionAckPacket oap(opt_vec);
          REQUIRE(oap.get_opcode() == TFTPOpcode::OACK);
          REQUIRE(oap.get_options_count() == 4);
          REQUIRE(oap.get_option_value("blksize") == "1432");
          REQUIRE(oap.get_option_value("timeout") == "5");
          REQUIRE(oap.get_option_value("tsize") == "123456789");
          REQUIRE(oap.get_option_value("hakuna") == "matata");
          REQUIRE(oap.get_option_value("undefined") == "");
          REQUIRE(oap.get_option_str(0) == "blksize=1432");
          REQUIRE(oap.get_option_str(1) == "timeout=5");
          REQUIRE(oap.get_option_str(2) == "tsize=123456789");
          REQUIRE(oap.get_option_str(3) == "hakuna=matata");
          REQUIRE(oap.get_option_str(4) == "");
          REQUIRE(oap.get_options() == opt_vec);
     }

     SECTION("Setters and getters") {
          OptionAckPacket oap;
          REQUIRE(oap.get_options_count() == 0);

          oap.add_option("blksize", "1432");
          REQUIRE(oap.get_options_count() == 1);
          REQUIRE(oap.get_options()[0].first == "blksize");
          REQUIRE(oap.get_options()[0].second == "1432");
          REQUIRE(oap.get_option_value("blksize") == "1432");
          REQUIRE(oap.get_option_str(0) == "blksize=1432");

          oap.add_option("timeout", "5");
          REQUIRE(oap.get_options_count() == 2);
          REQUIRE(oap.get_options()[1].first == "timeout");
          REQUIRE(oap.get_options()[1].second == "5");
          REQUIRE(oap.get_option_value("timeout") == "5");
          REQUIRE(oap.get_option_str(1) == "timeout=5");

          oap.add_option("tsize", "123456789");
          REQUIRE(oap.get_options_count() == 3);
          REQUIRE(oap.get_options()[2].first == "tsize");
          REQUIRE(oap.get_options()[2].second == "123456789");
          REQUIRE(oap.get_option_value("tsize") == "123456789");
          REQUIRE(oap.get_option_str(2) == "tsize=123456789");

          oap.add_option("hakuna", "matata");
          REQUIRE(oap.get_options_count() == 4);
          REQUIRE(oap.get_options()[3].first == "hakuna");
          REQUIRE(oap.get_options()[3].second == "matata");
          REQUIRE(oap.get_option_value("hakuna") == "matata");
          REQUIRE(oap.get_option_str(3) == "hakuna=matata");

          /* Option overwrite */
          oap.set_option("hakuna", "tumainini");
          REQUIRE(oap.get_options_count() == 4);
          REQUIRE(oap.get_options()[3].first == "hakuna");
          REQUIRE(oap.get_options()[3].second == "tumainini");
          REQUIRE(oap.get_option_value("hakuna") == "tumainini");
          REQUIRE(oap.get_option_str(3) == "hakuna=tumainini");

          oap.clear_options();
          REQUIRE(oap.get_options_count() == 0);
          REQUIRE(oap.get_options().empty());
     }

     SECTION("Serialisation and deserialisation") {
          std::string opt_name, opt_val;
          std::vector<std::pair<std::string, std::string>> opt_vec;
          opt_vec.push_back(std::make_pair("blksize", "1432"));
          opt_vec.push_back(std::make_pair("timeout", "5"));
          OptionAckPacket oap(opt_vec);

          // Packet -> Binary
          std::vector<char> binary = oap.to_binary();
          REQUIRE(binary[0] == 0x00);  // Opcode (HI)
          REQUIRE(binary[1] == 0x06);  // Opcode (LO)
          int offset = 2;
          offset = OptionAckPacket::findcstr(binary, offset, opt_name);
          REQUIRE(opt_name == "blksize");
          REQUIRE(binary[offset - 1] == 0x00);  // Terminator
          offset = OptionAckPacket::findcstr(binary, offset, opt_val);
          REQUIRE(opt_val == "1432");
          REQUIRE(binary[offset - 1] == 0x00);  // Terminator
          offset = OptionAckPacket::findcstr(binary, offset, opt_name);
          REQUIRE(opt_name == "timeout");
          REQUIRE(binary[offset - 1] == 0x00);  // Terminator
          offset = OptionAckPacket::findcstr(binary, offset, opt_val);
          REQUIRE(opt_val == "5");
          REQUIRE(binary[offset - 1] == 0x00);  // Terminator

          // Binary -> Packet
          OptionAckPacket oap2 = OptionAckPacket::from_binary(binary);
          REQUIRE(oap2.get_opcode() == TFTPOpcode::OACK);
          REQUIRE(oap2.get_options_count() == oap.get_options_count());
          REQUIRE(oap2.get_options() == oap.get_options());
          REQUIRE(oap == oap2);
     }

     SECTION("Empty serialisation") {
          OptionAckPacket oap;
          std::vector<char> binary = oap.to_binary();
          REQUIRE(binary.size() == 0);
     }
}