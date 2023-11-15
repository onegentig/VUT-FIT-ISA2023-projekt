/**
 * @file Logger.hpp
 * @author Filip J. Kramec <xkrame00@vutbr.cz>
 * @brief Logging utility class
 * @date 2023-11-15
 */

#pragma once
#ifndef TFTP_LOGGER_HPP

#     include <optional>

#     include "packet/PacketFactory.hpp"

/**
 * @brief Logger utility class
 * @note Style very loosely based on `yay` AUR helper logs.
 *       Why? Just because I like it. Styling is used just
 *       on stdout, stderr logs follow the assignment specs.
 */
class Logger {
   public:
     /**
      * @brief Prints global operation info to stdout.
      * @param txt Text to print
      */
     static void glob_op(const std::string& txt) {
          std::cout << ":: " << txt << std::endl;
     }

     /**
      * @brief Prints global event info to stdout.
      * @param txt Text to print
      */
     static void glob_event(const std::string& txt) {
          std::cout << "==> " << txt << std::endl;
     }

     /**
      * @brief Prints a global information log to stdout.
      * @param txt Text to print
      */
     static void glob_info(const std::string& txt) {
          std::cout << "  " << txt << std::endl;
     }

     /**
      * @brief Prints a global error log to stderr.
      * @param txt Text to print
      */
     static void glob_err(const std::string& txt) {
          std::cerr << "!ERR! " << txt << std::endl;
     }

     /**
      * @brief Prints connection info to stdout.
      * @param id Connection ID
      * @param txt Text to print
      */
     static void conn_info(const std::string& id, const std::string& txt) {
          std::cout << "  [" << id << "] - INFO  - " << txt << std::endl;
     }

     /**
      * @brief Prints connection error to stdout.
      * @param id Connection ID
      * @param txt Text to print
      */
     static void conn_err(const std::string& id, const std::string& txt) {
          std::cout << "  [" << id << "] - ERROR - " << txt << std::endl;
     }

     /**
      * @brief Prints a packet to stderr
      * @param BasePacket Packet to print
      * @param sockaddr_in Address of the sender (src)
      * @param optional<sockaddr_in> Address of the receiver (dst)
      */
     static void packet(const BasePacket& packet, const sockaddr_in& src,
                        const std::optional<sockaddr_in>& dst = std::nullopt) {
          std::string msg;

          /* Opcode */
          switch (packet.get_opcode()) {
               case TFTPOpcode::RRQ:
                    msg = "RRQ ";
                    break;
               case TFTPOpcode::WRQ:
                    msg = "WRQ ";
                    break;
               case TFTPOpcode::ACK:
                    msg = "ACK ";
                    break;
               case TFTPOpcode::DATA:
                    msg = "DATA ";
                    break;
               case TFTPOpcode::ERROR:
                    msg = "ERROR ";
                    break;
               default:
                    return;
          }

          /* SRC_IP:SRC_PORT */
          msg += std::string(inet_ntoa(src.sin_addr)) + ":"
                 + std::to_string(ntohs(src.sin_port));

          /* DST_PORT (only for DATA and ERROR + if defined) */
          if (dst.has_value()
              && (packet.get_opcode() == TFTPOpcode::DATA
                  || packet.get_opcode() == TFTPOpcode::ERROR)) {
               msg += ":" + std::to_string(ntohs(dst.value().sin_port));
          }

          /* Type-specific */
          switch (packet.get_opcode()) {
               case TFTPOpcode::RRQ:
               case TFTPOpcode::WRQ: {
                    auto rq_packet = dynamic_cast<const RequestPacket&>(packet);
                    msg += " \"" + rq_packet.get_filename() + "\"";
                    msg += " " + rq_packet.get_mode_str();
                    break;
               }

               case TFTPOpcode::ACK: {
                    auto ack_packet
                        = dynamic_cast<const AcknowledgementPacket&>(packet);
                    msg += " " + std::to_string(ack_packet.get_block_number());
                    break;
               }

               case TFTPOpcode::DATA: {
                    auto data_packet = dynamic_cast<const DataPacket&>(packet);
                    msg += " " + std::to_string(data_packet.get_block_number());
                    break;
               }

               case TFTPOpcode::ERROR: {
                    auto err_packet = dynamic_cast<const ErrorPacket&>(packet);
                    msg += " " + std::to_string(err_packet.get_errcode());
                    if (err_packet.get_message().has_value()) {
                         msg += " \"" + err_packet.get_message().value() + "\"";
                    }
                    break;
               }

               default:
                    break;
          }

          /* Print */
          std::cerr << msg << std::endl;
     }
};

#endif