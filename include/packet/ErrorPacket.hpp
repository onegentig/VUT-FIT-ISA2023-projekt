/**
 * @file ErrorPacket.hpp
 * @author Filip J. Kramec <xkrame00@vutbr.cz>
 * @brief TFTP error packet.
 * @date 2023-10-16
 */

#pragma once
#ifndef ERROR_PACKET_HPP
#     define ERROR_PACKET_HPP
#     include "packet/BasePacket.hpp"

/**
 * @brief TFTP error packet class.
 * Represents the ERROR (opcode 5) packet, which signals an error. it
 * can be used as a response to any other packet. ERROR packets
 * end the communication without any acknowledgements.
 * @see https://datatracker.ietf.org/doc/html/rfc1350#autoid-4
 */
class ErrorPacket : public BasePacket {
   public:
     /**
      * @brief Constructs a new empty ERROR packet object.
      * @return ErrorPacket
      */
     ErrorPacket();

     /**
      * @brief Constructs a new ERROR packet object with set error code.
      * @param TFTPErrorCode errcode
      * @return ErrorPacket
      */
     explicit ErrorPacket(TFTPErrorCode errcode);

     /**
      * @brief Constructs a new ERROR packet object with set error code and
      * message.
      * @param TFTPErrorCode errcode
      * @param std::string msg
      * @return ErrorPacket
      */
     ErrorPacket(TFTPErrorCode errcode, std::string msg);

     /**
      * @brief Checks equality of two ERROR packets.
      * @param other ERROR packet to compare with
      * @return true when equal,
      * @return false when not equal
      */
     bool operator==(const ErrorPacket& other) const {
          return this->opcode == other.opcode && this->errcode == other.errcode
                 && this->msg == other.msg;
     }

     /* === Core Methods === */

     /**
      * @brief Returns binary representation of the packet.
      * Creates binary vector of the packet. ERROR packets start with a
      * 2B opcode followed by a 2B error errcode and an optional NetASCII
      * error message (terminated by a 0 byte).
      *
      * @return std::vector<char> - packet in binary
      */
     std::vector<char> toBinary() const override;

     /**
      * @brief Creates an ERROR packet from binary representation.
      * @throws std::invalid_argument when vector is not a proper ERROR packet
      *
      * @param std::vector<char> packet in binary
      * @return void
      */
     void fromBinary(const std::vector<char>& binaryData) override;

     /* === Getters and Setters === */

     /**
      * @brief Returns the error code.
      * @return TFTPErrorCode
      */
     TFTPErrorCode getErrcode() const { return this->errcode; }

     /**
      * @brief Sets the error code.
      * @param TFTPErrorCode errcode
      * @return void
      */
     void setErrcode(TFTPErrorCode errcode) { this->errcode = errcode; }

     /**
      * @brief Returns the error message.
      * @return std::optional<std::string>
      */
     std::optional<std::string> getMessage() const { return this->msg; }

     /**
      * @brief Sets the error message.
      * @param std::string msg
      * @return void
      */
     void setMessage(const std::string& msg) { this->msg = msg; }

     /**
      * @brief Removes the error message.
      * @return void
      */
     void removeMessage() { this->msg = std::nullopt; }

   private:
     TFTPErrorCode errcode;          /**< Error code */
     std::optional<std::string> msg; /**< Error message */
};

#endif
