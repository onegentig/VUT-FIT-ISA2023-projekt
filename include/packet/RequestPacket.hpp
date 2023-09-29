/**
 * @file RequestPacket.hpp
 * @author Filip J. Kramec <xkrame00@vutbr.cz>
 * @brief TFTP request packet.
 * @date 2023-09-28
 */

#pragma once
#ifndef REQUEST_PACKET_HPP
#     define REQUEST_PACKET_HPP
#     include "packet/BasePacket.hpp"

/**
 * @brief TFTP request packet type.
 */
enum RequestPacketType {
     Read = 0, /**< Read request */
     Write = 1 /**< Write request */
};

/**
 * @brief TFTP request packet class.
 */
class RequestPacket : public BasePacket {
   public:
     /**
      * @brief Constructs a new Request packet object.
      * @return RequestPacket
      */
     RequestPacket();

     /**
      * @brief Constructs a new Request packet object.
      * @param RequestPacketType - type
      * @param std::string - filename
      * @param std::string - mode
      * @return RequestPacket
      */
     explicit RequestPacket(RequestPacketType type, std::string filename,
                            std::string mode);

     /* === Core Methods === */

     /**
      * @brief Returns the binary representation of the packet.
      * @return std::vector<char> - packet in binary
      */
     std::vector<char> toBinary() const override;

     /**
      * @brief Creates a new packet from a binary representation.
      * @param std::vector<char> - packet in binary
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
      * @param std::string - filename
      * @return void
      */
     void setFilename(std::string filename) { this->filename = filename; }

     /**
      * @brief Returns the mode.
      * @return std::string - mode
      */
     std::string getMode() const { return mode; }

     /**
      * @brief Sets the mode.
      * @param std::string - mode
      * @return void
      */
     void setMode(std::string mode) { this->mode = mode; }

     /**
      * @brief Sets the type
      * @param RequestPacketType - type
      * @return void
      */
     void setType(RequestPacketType type) {
          this->opcode = type == RequestPacketType::Read ? TFTPOpcode::RRQ
                                                         : TFTPOpcode::WRQ;
     }

   private:
     std::string filename; /**< Filename */
     std::string mode;     /**< Mode */
};

#endif