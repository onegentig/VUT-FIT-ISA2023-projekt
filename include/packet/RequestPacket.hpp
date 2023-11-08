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
 * Represents the RRQ (opcode 1) and WRQ (opcode 2) packet that establishes the
 * connection. These packets should be sent to port 69 instead of the generated
 * TID. Assuming no error happens, RRQ is followed by DATA and WRQ is followed
 * by ACK.
 * @see https://datatracker.ietf.org/doc/html/rfc1350#autoid-4
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
          return this->opcode == other.opcode
                 && this->filename == other.filename
                 && this->mode == other.mode;
     }

     /* === Core Methods === */

     /**
      * @brief Returns binary representation of the packet.
      * Creates binary vector of the packet. {W,R}RQ packets always start with a
      * 2B opcode followed by a NetASCII filename string (null-terminated) and a
      * NetASCII mode ("octet" or "netascii") string (null-terminated).
      *
      * @return std::vector<char> - packet in binary
      */
     std::vector<char> to_binary() const override;

     /**
      * @brief Creates a RQ packet from binary representation.
      * Attempts to parse a binary vector into a {W,R}RQ packet.
      * @throws std::invalid_argument when vector is not a proper RQ packet
      *
      * @param std::vector<char> packet in binary
      * @return void
      */
     void from_binary(const std::vector<char>& bin_data) override;

     /* === Getters and Setters === */

     /**
      * @brief Returns the filename.
      * @return std::string - filename
      */
     std::string get_filename() const { return filename; }

     /**
      * @brief Sets the filename.
      * @param std::string filename
      * @return void
      */
     void set_filename(std::string filename) {
          this->filename = std::move(filename);
     }

     /**
      * @brief Returns the transfer format mode.
      * @return TFTPDataFormat - mode
      */
     TFTPDataFormat get_mode() const { return mode; }

     /**
      * @brief Returns the mode (as string).
      * @return std::string - mode
      */
     std::string get_mode_str() const {
          return mode == TFTPDataFormat::Octet ? "octet" : "netascii";
     }

     /**
      * @brief Sets the mode.
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

   private:
     std::string filename; /**< Filename (NetASCII string) */
     TFTPDataFormat mode;  /**< Mode (NetASCII string, "octet" or "netascii") */
};

#endif
