/**
 * @file AcknowledgementPacket.hpp
 * @author Onegen Something <xkrame00@vutbr.cz>
 * @brief TFTP acknowledgement packet.
 * @date 2023-09-28
 */

#pragma once
#ifndef ACKNOWLEDGEMENT_PACKET_HPP
#     define ACKNOWLEDGEMENT_PACKET_HPP
#     include "packet/BasePacket.hpp"

/**
 * @brief TFTP acknowledgement packet class.
 * Represents the ACK packet (opcode 4) in TFTP, which is the expected
 * response for WRQ and DATA packets (when not erroneous). This packet type only
 * contains the opcode and the number of the block it’s acknowledging (when not
 * applicable, ex. WRQ response, it’s 0).
 * @see https://datatracker.ietf.org/doc/html/rfc1350#autoid-5
 */
class AcknowledgementPacket : public BasePacket {
   public:
     /* === Constructors === */

     /**
      * @brief Constructs a new empty ACK packet object.
      * @return AcknowledgementPacket
      */
     AcknowledgementPacket();

     /**
      * @brief Constructs a new ACK packet object with set block number.
      * @param uint16_t block number
      * @return AcknowledgementPacket
      */
     explicit AcknowledgementPacket(uint16_t blockN);

     /**
      * @brief Checks equality of two ACK packets.
      * @param other ACK packet to compare with
      * @return true when equal,
      * @return false when not equal
      */
     bool operator==(const AcknowledgementPacket& other) const {
          return this->opcode == other.opcode && this->blockN == other.blockN;
     }

     /* === Core Methods === */

     /**
      * @brief Returns binary representation of the packet.
      * Creates binary vector of the packet. ACK packets always have 2B
      * opcode and 2B block number and nothing else. Block number is 0 when
      * acknowledging a WRQ, otherwise 1+ (as DATA blocks are indexed from 1).
      *
      * @return std::vector<char> - packet in binary
      */
     std::vector<char> toBinary() const override;

     /**
      * @brief Creates an ACK packet from a binary representation.
      * Attempts to parse a binary vector into an ACK packet.
      * @throws std::invalid_argument when vector is not a proper ACK packet
      *
      * @param std::vector<char> packet in binary
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
      * @param uint16_t block number
      * @return void
      */
     void setBlockNumber(uint16_t blockN) { this->blockN = blockN; }

   private:
     uint16_t blockN; /**< Two-byte block number. */
};

#endif
