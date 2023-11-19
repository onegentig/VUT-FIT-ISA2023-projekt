/**
 * @file RequestPacket.hpp
 * @author Onegen Something <xkrame00@vutbr.cz>
 * @brief TFTP option acknowledgement packet
 * @date 2023-11-19
 */

#pragma once
#ifndef TFTP_OACK_PACKET_HPP
#     define TFTP_OACK_PACKET_HPP
#     include "packet/BasePacket.hpp"

/**
 * @brief TFTP option acknowledgement packet class.
 * @details Represents the RFC 2347-added OACK (opcode 6) packet that
 *          acknowledges and approves of the options contained in the
 *          connection-establishing RRQ/WRQ packet.
 * @see https://datatracker.ietf.org/doc/html/rfc2347
 */
class OptionAckPacket : public BasePacket {
   public:
     /**
      * @brief Constructs a new empty OACK packet object
      * @return OptionAckPacket
      */
     OptionAckPacket();

     /**
      * @brief Constructs a new OACK packet object with given
      *        options vector
      * @param vector<pair<string, string>> options
      * @return OptionAckPacket
      */
     explicit OptionAckPacket(
         std::vector<std::pair<std::string, std::string>> opt_vec);

     /**
      * @brief Checks equality of two OACK packets.
      * @param other RQ packet to compare with
      * @return true when equal,
      * @return false when not equal
      */
     bool operator==(const OptionAckPacket& other) const {
          if (this->opcode != other.opcode) return false;
          if (this->opts.size() != other.opts.size()) return false;

          /* Compare options */
          for (size_t i = 0; i < this->opts.size(); i++) {
               if (this->opts[i].first != other.opts[i].first
                   || this->opts[i].second != other.opts[i].second)
                    return false;
          }

          return true;
     }

     /* === Core Methods === */

     /**
      * @brief Returns binary representation of the packet
      * @details Creaates binary vector of the packet. OACK packets always
      *          starts with a 2B optcode and then a list of n options,
      *          each with a name and a value in NetASCII, both
      *          null-terminated.
      *
      * @return std::vector<char>  - packet in binary
      */
     std::vector<char> to_binary() const override;

     /**
      * @brief Creates an OACK packet from binary representation
      * @details Attempts to parse a binary vector into a OACK packet
      * @throws std::invalid_argument when vector is not a proper OACK packet
      *
      * @param std::vector<char> packet in binary
      * @return OptionACkPacket
      */
     static OptionAckPacket from_binary(const std::vector<char>& bin_data);

     /* === Getters and Setters === */

     /**
      * @brief Sets an option, adding a new one if it doesn't exist and
      *        overwriting if it does
      * @param std::string name Option name
      * @param std::string value Option value
      */
     void set_option(std::string name, std::string value) {
          /* Overwrite if exists */
          for (auto& opt : opts) {
               if (opt.first == name) {
                    opt.second = std::move(value);
                    return;
               }
          }

          /* Else add new option */
          opts.emplace_back(std::move(name), std::move(value));
     }

     /**
      * @brief Adds a new option to the end of the packet
      * @throws std::invalid_argument when option already exists
      * @param std::string name Option name
      * @param std::string value Option value
      */
     void add_option(std::string name, std::string value) {
          /* Check if exists */
          for (const auto& opt : opts)
               if (opt.first == name)
                    throw std::invalid_argument("Option already exists");

          /* Add new option */
          opts.emplace_back(std::move(name), std::move(value));
     }

     /**
      * @brief Returns the options
      * @return std::vector<std::pair<std::string, std::string>> - options
      */
     std::vector<std::pair<std::string, std::string>> get_options() const {
          return opts;
     }

     /**
      * @brief Gets option value by name
      * @param std::string name Option name
      * @return std::string Option value
      */
     std::string get_option_value(const std::string& name) const {
          for (const auto& opt : opts)
               if (opt.first == name) return opt.second;

          return "";
     }

     /**
      * @brief Gets an option string "name=value" at given index
      * @param size_t index Index of the option
      * @return std::string Option string
      */
     std::string get_option_str(size_t index) const {
          if (index >= opts.size()) return "";

          return opts[index].first + "=" + opts[index].second;
     }

     /**
      * @brief Returns number of set options
      * @return size_t - number of options
      */
     size_t get_options_count() const { return opts.size(); }

     /**
      * @brief Clears all options
      */
     void clear_options() { opts.clear(); }

   private:
     std::vector<std::pair<std::string, std::string>> opts; /**< Options */
};

#endif
