/**
 * @file connection.cpp
 * @author Filip J. Kramec (xkrame00@vutbr,cz)
 * @brief TFTP connection implementation.
 * @date 2023-11-08
 */

#include "server/connection.hpp"

/* === Constructors === */

TFTPServerConnection::TFTPServerConnection(
    int srv_fd, const sockaddr_in& clt_addr, const RequestPacket& reqPacket,
    const std::string& rootDir,
    const std::shared_ptr<std::atomic<bool>>& shutd_flag)
    : srv_fd(srv_fd), clt_addr(clt_addr), shutd_flag(*shutd_flag) {
     this->state = ConnectionState::Requested;
     file_path = rootDir + "/" + reqPacket.get_filename();
     type = reqPacket.get_type();
     format = reqPacket.get_mode();

     /* Create socket */
     this->conn_fd = socket(AF_INET, SOCK_DGRAM, 0);
     if (this->conn_fd == -1) {
          Logger::glob_err("Failed to create a socket for connection!");
          this->state = ConnectionState::Errored;
          return;
     }

     /* Set address */
     memset(&(this->conn_addr), 0, this->conn_addr_len);
     this->conn_addr.sin_family = AF_INET;
     this->conn_addr.sin_port = htons(0);  // Random port
     this->conn_addr.sin_addr.s_addr = htonl(INADDR_ANY);

     /* Set timeout */
     struct timeval timeout {
          TFTP_TIMEO, 0
     };

     if (setsockopt(this->conn_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                    sizeof(timeout))
         < 0) {
          Logger::glob_err("Failed to set socket timeout for connection!");
          this->state = ConnectionState::Errored;
          return;
     }

     /* Allow address:port reuse */
     int optval = 1;
     if (setsockopt(this->conn_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                    &optval, sizeof(optval))
         < 0) {
          Logger::glob_err("Failed to set socket reuse for connection!");
          this->state = ConnectionState::Errored;
          return;
     }

     /* Bind socket */
     if (bind(this->conn_fd,
              reinterpret_cast<struct sockaddr*>(&this->conn_addr),
              this->conn_addr_len)
         < 0) {
          Logger::glob_err("Failed to bind socket for connection!");
          this->state = ConnectionState::Errored;
          return;
     }

     /* Get OS-assigned random port */
     if (getsockname(this->conn_fd,
                     reinterpret_cast<struct sockaddr*>(&this->conn_addr),
                     &(this->conn_addr_len))
         < 0) {
          Logger::glob_err("Failed to get socket name for connection!");
          this->state = ConnectionState::Errored;
          return;
     }

     this->tid = ntohs(this->conn_addr.sin_port);
     Logger::glob_event("New connection [" + std::to_string(this->tid) + "]"
                        + " from "
                        + std::string(inet_ntoa(this->clt_addr.sin_addr)) + ":"
                        + std::to_string(ntohs(this->clt_addr.sin_port)));
}

TFTPServerConnection::~TFTPServerConnection() {
     /* Close connection socket */
     if (this->conn_fd >= 0) {
          ::close(this->conn_fd);
          this->conn_fd = -1;
     }

     /* If download (write) ended with error, remove file */
     if (this->is_download() && this->state != ConnectionState::Completed
         && this->block_n > 0) {
          if (access(this->file_path.c_str(), F_OK) == 0) {
               remove(this->file_path.c_str());
          }
     }

     /* Close file */
     if (this->file_fd >= 0) {
          ::close(this->file_fd);
          this->file_fd = -1;
     }

     Logger::glob_event("Closed connection [" + std::to_string(this->tid)
                        + "]");
}

/* === Connection Flow === */

void TFTPServerConnection::run() {
     while (this->is_running()) {
          /* Check shutdown flag */
          if (this->shutd_flag.load()) {
               log_info("Shutting down (shutd_flag detected)");
               return send_error(TFTPErrorCode::Unknown, "Terminated by user");
          }

          /* Handle state */
          switch (this->state) {
               case ConnectionState::Requested:
                    this->type == TFTPRequestType::Read ? this->handle_rrq()
                                                        : this->handle_wrq();
                    break;
               case ConnectionState::Uploading:
                    this->handle_upload();
                    break;
               case ConnectionState::Downloading:
                    this->handle_download();
                    break;
               case ConnectionState::Awaiting:
                    this->type == TFTPRequestType::Read
                        ? this->handle_await_upload()
                        : this->handle_await_download();
                    break;
               default:
                    log_error("`run` called in invalid state");
                    this->state = ConnectionState::Errored;
                    break;
          }

          // TODO: This could be reworked to `select` or `poll` for better perf
          std::this_thread::sleep_for(
              std::chrono::microseconds(TFTP_THREAD_DELAY));
     }
}

/* == Uploading (RRQ) flow == */

void TFTPServerConnection::handle_rrq() {
     log_info("Requesting read of file " + this->file_path);

     /* Check if file exists */
     if (access(this->file_path.c_str(), F_OK) != 0) {
          log_error("File does not exist");
          return this->send_error(TFTPErrorCode::FileNotFound,
                                  "File does not exist");
     }

     /* Open file for reading */
     this->file_fd = open(this->file_path.c_str(), O_RDONLY);
     if (this->file_fd < 0) {
          TFTPErrorCode errcode;
          std::string errmsg;

          if (errno == ENOENT) {
               errcode = TFTPErrorCode::FileNotFound;
               errmsg = "File not found";
          } else if (errno == EACCES) {
               errcode = TFTPErrorCode::AccessViolation;
               errmsg = "Access violation";
          } else {
               errcode = TFTPErrorCode::Unknown;
               errmsg = "Failed to open file";
          }

          log_error(errmsg);
          return this->send_error(errcode, errmsg);
     }

     /* Check if file doesn’t exceed max allowed size */
     /** @see https://stackoverflow.com/a/6039648 */
     struct stat st;
     if (fstat(this->file_fd, &st) != 0
         || st.st_size > static_cast<off_t>(TFTP_MAX_DATA * TFTP_MAX_FILE_BLOCKS
                                            - 1)) {
          close(this->file_fd);
          log_error("File too big");
          return this->send_error(TFTPErrorCode::Unknown, "File too big");
     }

     /* Things are ready for transfer */
     log_info("File ready, starting upload");
     this->state = ConnectionState::Uploading;
}

void TFTPServerConnection::handle_upload() {
     if (this->block_n == 0) this->block_n = 1;
     this->last_packet_time = std::chrono::steady_clock::now();

     /* Create data payload */
     DataPacket packet = DataPacket(this->file_fd, this->block_n);
     packet.set_mode(this->format);
     auto payload = packet.to_binary();
     if (payload.size() < TFTP_MAX_DATA + 4) {
          this->is_last.store(true);
     } else {
          this->is_last.store(false);
     }

     log_info("Sending DATA block " + this->get_block_n_hex() + " ("
              + std::to_string(payload.size()) + " bytes)");

     /* Send data */
     sendto(this->conn_fd, payload.data(), payload.size(), 0,
            reinterpret_cast<const sockaddr*>(&this->clt_addr),
            sizeof(this->clt_addr));

     /* And now, await acknowledgement */
     log_info("Awaiting ACK for block " + this->get_block_n_hex());
     this->state = ConnectionState::Awaiting;
}

void TFTPServerConnection::handle_await_upload() {
     /* Timeout check */
     // @see https://en.cppreference.com/w/cpp/chrono#Example
     auto now = std::chrono::steady_clock::now();
     if (std::chrono::duration_cast<std::chrono::seconds>(
             now - this->last_packet_time)
             .count()
         > TFTP_PACKET_TIMEO) {
          /* Check if TFTP_MAX_RETRIES was reached */
          if (this->send_tries + 1 >= TFTP_MAX_RETRIES) {
               log_error("Maximum number of retries reached");
               send_error(TFTPErrorCode::Unknown, "Retransmission timeout");
               return;
          }

          this->send_tries++;
          this->state = is_upload() ? ConnectionState::Uploading
                                    : ConnectionState::Downloading;
          log_info("Retransmitting block " + this->get_block_n_hex()
                   + " (attempt " + std::to_string(this->send_tries + 1) + ")");
          return;
     }

     /* Receive ACK */
     this->rx_len
         = recvfrom(this->conn_fd, this->rx_buffer.data(), TFTP_MAX_PACKET, 0,
                    reinterpret_cast<struct sockaddr*>(&this->clt_addr),
                    &this->clt_addr_len);

     /* Handle errors */
     if (this->rx_len < 0) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
               /* Nothing to receive, loop in state */
               return;
          }

          log_error("Failed to receive ACK: " + std::string(strerror(errno)));
          send_error(TFTPErrorCode::Unknown, std::string(strerror(errno)));
          return;
     }

     /* Parse incoming packet */
     auto packet_ptr = PacketFactory::create(this->rx_buffer, this->rx_len);
     if (!packet_ptr) {
          log_error("Received an unparsable packet");
          send_error(TFTPErrorCode::IllegalOperation,
                     "Received an invalid packet");
          return;
     }

     Logger::packet(*packet_ptr, this->clt_addr, this->conn_addr);

     if (packet_ptr->get_opcode() != TFTPOpcode::ACK) {
          log_error("Received a non-ACK packet");
          send_error(TFTPErrorCode::IllegalOperation,
                     "Received a non-ACK packet");
          return;
     }

     auto* ack_packet_ptr
         = dynamic_cast<AcknowledgementPacket*>(packet_ptr.get());

     log_info("Received ACK for block " + this->get_block_n_hex() + " ("
              + std::to_string(this->rx_len) + " bytes)");

     /* Check if ACK block number is as expected */
     if (ack_packet_ptr->get_block_number() < this->block_n) {
          return;  // Stray past ACK, ignore.
     } else if (ack_packet_ptr->get_block_number() > this->block_n) {
          log_error("Received an ACK with a future block number");
          send_error(TFTPErrorCode::IllegalOperation,
                     "Received an ACK with a future block number");
          return;
     }

     /* Check if this is the last packet */
     if (this->is_last.load()) {
          log_info("Upload completed!");
          this->state = ConnectionState::Completed;
          return;
     }

     /* Increment block number */
     this->block_n++;
     this->send_tries = 0;

     /* Continue transferring */
     this->state = ConnectionState::Uploading;
}

/* == Downloading (WRQ) flow == */

void TFTPServerConnection::handle_wrq() {
     log_info("Requesting write of file " + this->file_path);

     /* Check if file exists */
     /** @see https://stackoverflow.com/a/12774387 */
     if (access(this->file_path.c_str(), F_OK) == 0) {
          log_error("File already exists");
          return this->send_error(TFTPErrorCode::FileAlreadyExists,
                                  "File already exists");
     }

     /* Create a part file */
     this->file_fd = open(this->file_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC,
                          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

     /* Check if file was created properly */
     if (this->file_fd < 0) {
          log_error("Failed to create file");
          return this->send_error(TFTPErrorCode::AccessViolation,
                                  "Failed to create file");
     }

     /* Things are ready for transfer */
     log_info("File ready, starting download");
     this->state = ConnectionState::Downloading;
}

void TFTPServerConnection::handle_download() {
     AcknowledgementPacket ack = AcknowledgementPacket(this->block_n);
     this->last_packet_time = std::chrono::steady_clock::now();

     /* If `block_n` is 0 or buffer is empty (=> retransmit), just send ACK */
     if (this->block_n == 0 || this->rx_len <= 0) {
          log_info("Sending ACK for block " + this->get_block_n_hex());

          auto payload = ack.to_binary();
          if (sendto(this->conn_fd, payload.data(), payload.size(), 0,
                     reinterpret_cast<const sockaddr*>(&this->clt_addr),
                     sizeof(this->clt_addr))
              < 0) {
               log_error("Failed to send ACK for block 0");
               return this->send_error(TFTPErrorCode::Unknown,
                                       "Failed to send ACK for block 0");
          }

          /* Await next data block */
          this->state = ConnectionState::Awaiting;
          return;
     }

     /* Parse data from buffer */
     DataPacket packet = DataPacket::from_binary(std::vector<char>(
         this->rx_buffer.begin(), this->rx_buffer.begin() + this->rx_len));
     auto data = packet.get_data();
     if (this->format == TFTPDataFormat::NetASCII && !data.empty()) {
          /* Adjustment for [... CR] | [LF/NUL ...] block split*/
          if (this->last_data_cr && data[0] == '\n') {
               /* CR | LF -> LF */
               off_t file_size = lseek(this->file_fd, 0, SEEK_END);
               if (file_size > 0) ftruncate(this->file_fd, file_size - 1);
               lseek(this->file_fd, 0, SEEK_END);
          } else if (this->last_data_cr && data[0] == '\0') {
               /* CR | NUL -> CR */
               data.erase(data.begin());
          }

          data = NetASCII::from_netascii(data);
     }

     log_info("Received block " + this->get_block_n_hex() + " ("
              + std::to_string(data.size()) + " bytes)");

     /* Write data to file */
     if (write(this->file_fd, data.data(), data.size()) < 0) {
          log_error("Failed to write to file");
          return this->send_error(TFTPErrorCode::AccessViolation,
                                  "Failed to write to file");
     }

     this->last_data_cr = (!data.empty() && data.back() == '\r');

     /* Clear buffer */
     this->rx_buffer.fill(0);
     this->rx_len = 0;

     /* Send acknowledgement */
     log_info("Sending ACK for block " + this->get_block_n_hex());
     auto payload = ack.to_binary();
     if (sendto(this->conn_fd, payload.data(), payload.size(), 0,
                reinterpret_cast<const sockaddr*>(&this->clt_addr),
                sizeof(this->clt_addr))
         < 0) {
          log_error("Failed to send ACK for block " + this->get_block_n_hex());
          return this->send_error(
              TFTPErrorCode::Unknown,
              "Failed to send ACK for block " + this->get_block_n_hex());
     }

     /* Check if this was the last data block */
     if (packet.get_data().size() < TFTP_MAX_DATA) {
          log_info("Download completed!");
          this->state = ConnectionState::Completed;
          return;
     }

     /* Await next data block */
     this->state = ConnectionState::Awaiting;
}

void TFTPServerConnection::handle_await_download() {
     /* Timeout check */
     auto now = std::chrono::steady_clock::now();
     if (std::chrono::duration_cast<std::chrono::seconds>(
             now - this->last_packet_time)
             .count()
         > TFTP_PACKET_TIMEO) {
          /* Check if TFTP_MAX_RETRIES was reached */
          if (this->send_tries + 1 >= TFTP_MAX_RETRIES) {
               log_error("Maximum number of retries reached");
               send_error(TFTPErrorCode::Unknown, "Retransmission timeout");
               return;
          }

          log_info("Retransmitting ACK for block " + this->get_block_n_hex()
                   + " (attempt " + std::to_string(this->send_tries + 1) + ")");

          this->send_tries++;
          this->state = ConnectionState::Downloading;
          return;
     }

     /* Receive data */
     this->rx_len
         = recvfrom(this->conn_fd, this->rx_buffer.data(), TFTP_MAX_PACKET, 0,
                    reinterpret_cast<struct sockaddr*>(&this->clt_addr),
                    &this->clt_addr_len);

     /* Handle errors */
     if (this->rx_len < 0) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
               /* Nothing to receive, loop in state */
               return;
          }

          log_error("Failed to receive data: " + std::string(strerror(errno)));
          send_error(TFTPErrorCode::Unknown, std::string(strerror(errno)));
          return;
     }

     /* Parse incoming packet */
     auto packet_ptr = PacketFactory::create(this->rx_buffer, this->rx_len);
     if (!packet_ptr) {
          log_error("Received an unparsable packet");
          send_error(TFTPErrorCode::IllegalOperation,
                     "Received an invalid packet");
          return;
     }

     Logger::packet(*packet_ptr, this->clt_addr, this->conn_addr);

     if (packet_ptr->get_opcode() != TFTPOpcode::DATA) {
          log_error("Received a non-DATA packet");
          send_error(TFTPErrorCode::IllegalOperation,
                     "Received a non-DATA packet");
          return;
     }

     /* Increment block number */
     this->block_n++;
     this->send_tries = 0;

     /* Go to writing state */
     this->state = ConnectionState::Downloading;
}

/* === Helper Methods === */

void TFTPServerConnection::send_error(TFTPErrorCode code,
                                      const std::string& message) {
     ErrorPacket res = ErrorPacket(code, message);
     auto payload = res.to_binary();

     sendto(this->conn_fd, payload.data(), payload.size(), 0,
            reinterpret_cast<const sockaddr*>(&this->clt_addr),
            sizeof(this->clt_addr));

     this->state = ConnectionState::Errored;
}
