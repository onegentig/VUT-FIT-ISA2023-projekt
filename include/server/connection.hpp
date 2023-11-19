/**
 * @file connection.hpp
 * @author Filip J. Kramec (xkrame00@vutbr,cz)
 * @brief TFTP connection implementation.
 * @date 2023-11-08
 */

#pragma once
#ifndef TFTP_CONNECTION_HPP
#     define TFTP_CONNECTION_HPP

#     include "util/connection.hpp"

/**
 * @brief Class for TFTPserver connections
 */
class TFTPServerConnection : public TFTPConnectionBase {
   public:
     /**
      * @brief Constructs a new TFTPServerConnection object
      * @param clt_addr Client address
      * @param req_packet Request packet (RRQ / WRQ)
      * @param root_dir Server root directory
      * @param shutd_flag Shared shutdown flag
      */
     TFTPServerConnection(const sockaddr_in& clt_addr,
                          const RequestPacket& req_packet,
                          const std::string& root_dir,
                          const std::shared_ptr<std::atomic<bool>>& shutd_flag);

     TFTPServerConnection& operator=(TFTPServerConnection&& other) = delete;
     TFTPServerConnection& operator=(const TFTPServerConnection&) = delete;
     TFTPServerConnection(TFTPServerConnection&& other) = delete;
     TFTPServerConnection(const TFTPServerConnection&) = delete;

     /** @note `exec()` is made public to allow for `poll()` handling */
     using TFTPConnectionBase::exec;

   protected:
     /* === Overrides === */

     bool is_upload() const override {
          return this->get_type() == TFTPRequestType::Read;
     }

     bool is_download() const override {
          return this->get_type() == TFTPRequestType::Write;
     }

     void handle_request_upload() override;
     void handle_request_download() override;
     bool should_shutd() override;
     std::vector<char> next_data() override;

     /* === Variables === */
     std::atomic<bool>& shutd_flag; /**< Flag to signal shutdown */
};

#endif