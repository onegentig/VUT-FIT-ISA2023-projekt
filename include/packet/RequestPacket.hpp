/**
 * @file RequestPacket.hpp
 * @author Filip J. Kramec <xkrame00@vutbr.cz>
 * @brief TFTP request packet.
 * @date 2023-09-28
 */

#pragma once
#ifndef TFTP_REQ_PACKET_HPP
#     define TFTP_REQ_PACKET_HPP
#     include "packet/BasePacket.hpp"

/**
 * @brief TFTP request packet class.
 * @details Represents the RRQ (opcode 1) and WRQ (opcode 2) packet that
 *          establishes the connection. These packets should be sent to
 *          port 69 (or alt. main server port) instead of the generated
 *          TID. Assuming no error happens, RRQ is followed by DATA and
 *          WRQ is followed by ACK.
 * @see https://datatracker.ietf.org/doc/html/rfc1350#autoid-4
 *
 * @details RFC 2347 adds support for options, that can be set by the
 *          client and OACK’d by the server. If any options are present
 *          in `options`, req. appends them at the end of the packet.
 */
class RequestPacket : public BasePacket {
   public:
     /**
      * @brief Constructs a new empty RQ packet object.
      * @return RequestPacket
      */
     RequestPacket();

     /**
      * @brief Constructs a new RQ packet object with set parameters.
      * @param TFTPRequestType type
      * @param std::string filename
      * @param TFTPDataFormat mode
      * @return RequestPacket
      */
     explicit RequestPacket(TFTPRequestType type, std::string filename,
                            TFTPDataFormat mode);

     /**
      * @brief Checks equality of two RQ packets.
      * @param other RQ packet to compare with
      * @return true when equal,
      * @return false when not equal
      */
     bool operator==(const RequestPacket& other) const {
          bool eq = this->opcode == other.opcode
                    && this->filename == other.filename
                    && this->mode == other.mode;
          if (!eq) return false;

          /* Compare options */
          if (this->opts.size() != other.opts.size()) return false;
          for (size_t i = 0; i < this->opts.size(); i++) {
               if (this->opts[i].first != other.opts[i].first
                   || this->opts[i].second != other.opts[i].second)
                    return false;
          }

          return true;
     }

     /* === Core Methods === */

     /**
      * @brief Returns binary representation of the packet.
      * @details Creates binary vector of the packet. {W,R}RQ packets always
      *          start with a 2B opcode followed by a NetASCII filename string
      *          (null-terminated) and a NetASCII mode ("octet" or "netascii")
      *          string (null-terminated).
      *
      * @return std::vector<char> - packet in binary
      */
     std::vector<char> to_binary() const override;

     /**
      * @brief Creates a RQ packet from binary representation
      * @details Attempts to parse a binary vector into a {W,R}RQ packet.
      * @throws std::invalid_argument when vector is not a proper RQ packet
      *
      * @param std::vector<char> packet in binary
      * @return RequestPacket
      */
     static RequestPacket from_binary(const std::vector<char>& bin_data);

     /* === Getters and Setters === */

     /**
      * @brief Returns the filename
      * @return std::string - filename
      */
     std::string get_filename() const { return filename; }

     /**
      * @brief Sets the filename
      * @param std::string filename
      * @return void
      */
     void set_filename(std::string filename) {
          this->filename = std::move(filename);
     }

     /**
      * @brief Returns the transfer format mode
      * @return TFTPDataFormat - mode
      */
     TFTPDataFormat get_mode() const { return mode; }

     /**
      * @brief Returns the mode (as string)
      * @return std::string - mode
      */
     std::string get_mode_str() const {
          return mode == TFTPDataFormat::Octet ? "octet" : "netascii";
     }

     /**
      * @brief Sets the mode
      * @param TFTPDataFormat mode
      * @return void
      */
     void set_mode(TFTPDataFormat mode) { this->mode = mode; }

     /**
      * @brief Sets the type (read || write)
      * @param TFTPRequestType type
      * @return void
      */
     void set_type(TFTPRequestType type) {
          this->opcode = type == TFTPRequestType::Read ? TFTPOpcode::RRQ
                                                       : TFTPOpcode::WRQ;
     }

     /**
      * @brief Returns the type (read || write)
      * @return TFTPRequestType - type
      */
     TFTPRequestType get_type() const {
          return opcode == TFTPOpcode::RRQ ? TFTPRequestType::Read
                                           : TFTPRequestType::Write;
     }

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
     std::string filename; /**< Filename (NetASCII string) */
     TFTPDataFormat mode;  /**< Mode (NetASCII string, "octet" or "netascii") */
     std::vector<std::pair<std::string, std::string>> opts; /**< Options */
};

#endif
