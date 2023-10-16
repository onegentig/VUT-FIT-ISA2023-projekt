/**
 * @file RequestPacket.hpp
 * @author Onegen Something <xkrame00@vutbr.cz>
 * @brief TFTP request packet.
 * @date 2023-09-28
 */

#pragma once
#ifndef REQUEST_PACKET_HPP
#     define REQUEST_PACKET_HPP
#     include "packet/BasePacket.hpp"

/**
 * @brief Enumeration of the request packet types.
 */
enum RequestPacketType {
     Read = 0, /**< Read request */
     Write = 1 /**< Write request */
};

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
      * @param RequestPacketType type
      * @param std::string filename
      * @param DataFormat mode
      * @return RequestPacket
      */
     explicit RequestPacket(RequestPacketType type, std::string filename,
                            DataFormat mode);

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
     std::vector<char> toBinary() const override;

     /**
      * @brief Creates a RQ packet from binary representation.
      * Attempts to parse a binary vector into a {W,R}RQ packet.
      * @throws std::invalid_argument when vector is not a proper RQ packet
      *
      * @param std::vector<char> packet in binary
      * @return void
      */
     void fromBinary(const std::vector<char>& binaryData) override;

     /* === Getters and Setters === */

     /**
      * @brief Returns the filename.
      * @return std::string - filename
      */
     std::string getFilename() const { return filename; }

     /**
      * @brief Sets the filename.
      * @param std::string filename
      * @return void
      */
     void setFilename(std::string filename) {
          this->filename = std::move(filename);
     }

     /**
      * @brief Returns the transfer format mode.
      * @return DataFormat - mode
      */
     DataFormat getMode() const { return mode; }

     /**
      * @brief Returns the mode (as string).
      * @return std::string - mode
      */
     std::string getModeStr() const {
          return mode == DataFormat::Octet ? "octet" : "netascii";
     }

     /**
      * @brief Sets the mode.
      * @param DataFormat mode
      * @return void
      */
     void setMode(DataFormat mode) { this->mode = mode; }

     /**
      * @brief Sets the type (read || write)
      * @param RequestPacketType type
      * @return void
      */
     void setType(RequestPacketType type) {
          this->opcode = type == RequestPacketType::Read ? TFTPOpcode::RRQ
                                                         : TFTPOpcode::WRQ;
     }

   private:
     std::string filename; /**< Filename (NetASCII string) */
     DataFormat mode;      /**< Mode (NetASCII string, "octet" or "netascii") */
};

#endif
