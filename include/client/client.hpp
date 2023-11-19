/**
 * @file client.hpp
 * @author Onegen Something (xkrame00@vutbr.cz)
 * @brief TFTP client implementation
 * @date 2023-11-17
 */

#pragma once
#ifndef TFTP_CLIENT_HPP
#     define TFTP_CLIENT_HPP
#     include <netdb.h>

#     include <csignal>

#     include "util/connection.hpp"

/**
 * @brief Class for TFTP client
 */
class TFTPClient : public TFTPConnectionBase {
   public:
     /**
      * @brief Constructs a new TFTP client object with set hostname,
      *        port and destination path.
      * @param string hostname
      * @param [string] filepath
      * @param string destpath
      * @param [uint16_t] port
      * @return TFTPClient
      */
     TFTPClient(const std::string& hostname, int port,
                const std::string& destpath,
                const std::optional<std::string>& filepath);

     TFTPClient& operator=(TFTPClient&& other) = delete;
     TFTPClient& operator=(const TFTPClient&) = delete;
     TFTPClient(TFTPClient&& other) = delete;
     TFTPClient(const TFTPClient&) = delete;

   protected:
     /* === Overrides === */

     bool is_upload() const override {
          return this->get_type() == TFTPRequestType::Write;
     }

     bool is_download() const override {
          return this->get_type() == TFTPRequestType::Read;
     }

     void handle_request_upload() override;
     void handle_request_download() override;
     void handle_oack(const OptionAckPacket& oack) override;
     bool should_shutd() override;
     std::vector<char> next_data() override;

     /* === Variables === */

     /* == Connection params == */
     std::string hostname;                /**< Hostname to connect to */
     int port = TFTP_PORT;                /**< Port to connect to */
     std::string destpath;                /**< Destination path */
     std::optional<std::string> filepath; /**< Filepath to download */

     /* == Host entity == */
     struct hostent* host; /**< Host entity */

     /* == File buffer == */
     std::vector<char> file_buffer; /**< File buffer */
     ssize_t file_rx_len = 0;       /**< File buffer size */
};

#endif