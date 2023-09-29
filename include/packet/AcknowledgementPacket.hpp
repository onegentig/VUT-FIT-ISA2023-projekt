/**
 * @file AcknowledgementPacket.hpp
 * @author Filip J. Kramec <xkrame00@vutbr.cz>
 * @brief TFTP acknowledgement packet.
 * @date 2023-09-28
 */

#pragma once
#ifndef ACKNOWLEDGEMENT_PACKET_HPP
#     define ACKNOWLEDGEMENT_PACKET_HPP
#     include "packet/BasePacket.hpp"

/**
 * @brief TFTP acknowledgement packet class.
 */
class AcknowledgementPacket : public BasePacket {
   public:
     /* === Constructors === */

     /**
      * @brief Constructs a new Acknowledgement packet object.
      * @return AcknowledgementPacket
      */
     AcknowledgementPacket();

     /**
      * @brief Constructs a new Acknowledgement packet object.
      * @param uint16_t - block number
      * @return AcknowledgementPacket
      */
     explicit AcknowledgementPacket(uint16_t blockN);

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
      * @brief Returns the two-byte block number.
      * @return uint16_t - block number
      */
     uint16_t getBlockNumber() const { return blockN; }

     /**
      * @brief Sets the two-byte block number.
      * @param uint16_t - block number
      * @return void
      */
     void setBlockNumber(uint16_t blockN) { this->blockN = blockN; }

   private:
     uint16_t blockN; /**< Two-byte block number. */
};

#endif
