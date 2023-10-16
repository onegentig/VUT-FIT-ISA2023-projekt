/**
 * @file DataPacket.hpp
 * @author Filip J. Kramec <xkrame00@vutbr.cz>
 * @brief TFTP data packet.
 * @date 2023-10-07
 */

#pragma once
#ifndef DATA_PACKET_HPP
#     define DATA_PACKET_HPP
#     include <fcntl.h>
#     include <unistd.h>

#     include "packet/BasePacket.hpp"

/**
 * @brief TFTP data packet class.
 * Represents the DATA (opcode 3) packet that contains the binary data
 * along with the block number (staring from 1). The data field length
 * is 0 to 512 bytes, with length of <512 bytes indicating the end of
 * the transfer. The DATA packet is followed by ACK.
 * @note The binary data can be set in two ways – either by setting the
 *       file descriptor (or path) OR by setting the raw data. The raw
 *       data has higher priority if both are set. However, raw data
 *       will not have the encoding changed (NetASCII to octet) and you
 *       must handle that yourself.
 * @see https://datatracker.ietf.org/doc/html/rfc1350#autoid-5
 */
class DataPacket : public BasePacket {
   public:
     /* === Constructors === */

     /**
      * @brief Constructs a new DATA packet object.
      * @return DataPacket
      */
     DataPacket();

     /**
      * @brief Constructs a new DATA packet object (from raw data)
      * @param data - binary data to be send
      * @return DataPacket
      */
     explicit DataPacket(std::vector<char> data, uint16_t blockN);

     /**
      * @brief Constructs a new DATA packet object (from file descriptor)
      * @param fd - file descriptor
      * @param blockN - block number
      * @return DataPacket
      */
     explicit DataPacket(int fd, uint16_t blockN);

     /**
      * @brief Constructs a new DATA packet object (from file path)
      * @param path - file path
      * @param blockN - block number
      * @return DataPacket
      */
     explicit DataPacket(const std::string& path, uint16_t blockN);

     /**
      * @brief Compares and checks equality of two DATA packets.
      * @param other DATA packet to compare with
      * @return true when equal,
      * @return false when not equal
      */
     bool operator==(const DataPacket& other) const {
          return this->toBinary() == other.toBinary();
     }

     /* === Core Methods === */

     /**
      * @brief Reads data from the file descriptor and returns
      * its binary representation in set format.
      * @throws std::runtime_error when reading from file descriptor fails
      * @return std::vector<char> - packet in binary
      */
     std::vector<char> readFileData() const;

     /**
      * @brief Returns data for further processing, either returning a
      * cut part of the data vector (if `data` is set), reading data from
      * the file descriptor (if `fd` is set) or returning an empty vector.
      * @throws std::runtime_error when reading from file descriptor fails
      * @return std::vector<char> - packet in binary
      */
     std::vector<char> readData() const;

     /**
      * @brief Returns the binary representation of the packet.
      * Creates binary vector of the packet. DATA packets always start with a 2B
      * opcode (3) and 2B block number. The data is appended to the end of the
      * vector without any sconst eparators, sized 0 to 512 bytes.
      *
      * @return std::vector<char> - packet in binary
      */
     std::vector<char> toBinary() const override;

     /**
      * @brief Creates a new DATA packet from a binary representation.
      * Attempts to parse a binary vector into a DATA packet.
      * @throws std::invalid_argument when vector is not a proper DATA packet
      * @note Prefer to use `fromBinary()` with mode parameter.
      *
      * @param std::vector<char> packet in binary
      * @return void
      */
     void fromBinary(const std::vector<char>& binaryData) override;

     /**
      * @brief Creates a new DATA packet from a binary representation (with
      * mode). Attempts to parse a binary vector into a DATA packet.
      * @throws std::invalid_argument when vector is not a proper DATA packet
      *
      * @param std::vector<char> packet in binary
      * @param TFTPDataFormat mode
      * @return void
      */
     void fromBinary(const std::vector<char>& binaryData, TFTPDataFormat mode);

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

     /**
      * @brief Sets the mode.
      * @param TFTPDataFormat mode
      * @return void
      */
     void setMode(TFTPDataFormat mode) { this->mode = mode; }

     /**
      * @brief Returns the mode.
      * @return TFTPDataFormat - mode
      */
     TFTPDataFormat getMode() const { return mode; }

     /**
      * @brief Returns the mode (as string).
      * @return std::string - mode
      */
     std::string getModeStr() const {
          return mode == TFTPDataFormat::Octet ? "octet" : "netascii";
     }

     /**
      * @brief Returns the raw data.
      * @note This obtains the raw data when set, but does not cut to
      *       the block size nor does it read from the file descriptor
      *       if set. For that, use `readData()`!
      * @return std::vector<char> - data
      */
     std::vector<char> getData() const { return this->data; }

     /**
      * @brief Sets the raw data.
      * @param std::vector<char> data
      * @return void
      */
     void setData(std::vector<char> data) { this->data = std::move(data); }

     /**
      * @brief Sets the file descriptor.
      * @param int - file descriptor
      * @return void
      */
     void setFd(int fd) { this->fd = fd; }

     /**
      * @brief Returns the file descriptor.
      * @return int - file descriptor
      */
     int getFd() const { return fd; }

   private:
     int fd = -1;                         /**< File descriptor */
     uint16_t blockN;                     /**< Block number */
     std::vector<char> data;              /**< Binary data */
     TFTPDataFormat mode = TFTPDataFormat::Octet; /**< Transfer format mode */
};

#endif
