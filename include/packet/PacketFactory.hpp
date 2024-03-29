/**
 * @file PacketFactory.hpp
 * @author Onegen Something <xkrame00@vutbr.cz>
 * @brief TFTP packet factory.
 * @date 2023-11-08
 */

#pragma once
#ifndef TFTP_PACKET_FACTORY_HPP
#     define TFTP_PACKET_FACTORY_HPP
#     include <memory>

#     include "packet/AcknowledgementPacket.hpp"
#     include "packet/BasePacket.hpp"
#     include "packet/DataPacket.hpp"
#     include "packet/ErrorPacket.hpp"
#     include "packet/OptionAckPacket.hpp"
#     include "packet/RequestPacket.hpp"

/**
 * @brief TFTP packet factory class.
 * This class is used to create TFTP packets from raw data.
 * @see https://refactoring.guru/design-patterns/factory-method/cpp/example
 */
class PacketFactory {
   public:
     /**
      * @brief Create TFTP packet from a binary vector.
      * @param bin_data Binary vector to be parsed.
      * @return std::unique_ptr<BasePacket> Pointer to created packet.
      */
     static std::unique_ptr<BasePacket> create(
         const std::vector<char>& bin_data) {
          if (bin_data.empty()) {
               return nullptr;
          }

          uint16_t opcode
              = ntohs(*reinterpret_cast<const uint16_t*>(bin_data.data()));

          switch (opcode) {
               case TFTPOpcode::RRQ:
               case TFTPOpcode::WRQ:
                    return std::make_unique<RequestPacket>(
                        RequestPacket::from_binary(bin_data));
               case TFTPOpcode::DATA:
                    return std::make_unique<DataPacket>(
                        DataPacket::from_binary(bin_data));
               case TFTPOpcode::ACK:
                    return std::make_unique<AcknowledgementPacket>(
                        AcknowledgementPacket::from_binary(bin_data));
               case TFTPOpcode::ERROR:
                    return std::make_unique<ErrorPacket>(
                        ErrorPacket::from_binary(bin_data));
               case TFTPOpcode::OACK:
                    return std::make_unique<OptionAckPacket>(
                        OptionAckPacket::from_binary(bin_data));
               default:
                    return nullptr;
          }
     }

     /**
      * @brief Create TFTP packet from a binary array.
      * @note This is used only in new connections for RRQs and WRQs, so
      *       `TFTP_DFLT_MAXSIZE` is fine. The connections with their
      *       `blksize`s use vectors.
      * @param bin_data Binary data to be parsed.
      * @return std::unique_ptr<BasePacket> Pointer to created packet.
      */
     static std::unique_ptr<BasePacket> create(
         std::array<char, TFTP_DFLT_MAXSIZE>& bin_data, size_t size) {
          return create(
              std::vector<char>(bin_data.begin(), bin_data.begin() + size));
     }
};

#endif