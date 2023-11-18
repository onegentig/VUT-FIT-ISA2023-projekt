/**
 * @file client.cpp
 * @author Onegen Something (xkrame00@vutbr.cz)
 * @brief TFTP client implementation
 * @date 2023-11-17
 */

#include "client/client.hpp"

/**
 * @brief SIGINT flag
 * @details Atomic flag indicating whether SIGINT was recieved,
 * used to gracefully terminate the client (and send ERROR).
 */
std::atomic<bool> quit(false);

/**
 * @brief Sets the quit flag to true on SIGINT.
 * @param signal - signal number
 */
void signal_handler(int signal) {
     (void)signal;
     quit.store(true);
}

/* === Constructors === */

TFTPClient::TFTPClient(const std::string &hostname, int port,
                       const std::string &destpath,
                       const std::optional<std::string> &filepath)
    : hostname(hostname), port(port), destpath(destpath), filepath(filepath) {
     file_buffer.resize(TFTP_MAX_DATA);

     /* Verify hostname */
     if (hostname.empty()) throw std::runtime_error("Invalid hostname");

     /* Verify port number */
     if (port < 1 || port > 65535)
          throw std::runtime_error("Invalid port number");

     /* Verify destination path */
     if (destpath.empty()) throw std::runtime_error("Invalid destination path");

     /* Verify filepath */
     if (filepath.has_value()) {
          if (filepath.value().empty())
               throw std::runtime_error("Invalid filepath");

          /* Check if destination file exists */
          if (access(destpath.c_str(), F_OK) != -1)
               throw std::runtime_error("File " + destpath + " already exists");
     }

     /* Resolve hostname and get host */
     /** @see
      * https://moodle.vut.cz/pluginfile.php/550189/mod_folder/content/0/IPK2022-23L-03-PROGRAMOVANI.pdf#page=18
      */
     this->host = gethostbyname(this->hostname.c_str());
     if (this->host == NULL)
          throw std::runtime_error("Host not found: " + this->hostname);

     memset(reinterpret_cast<char *>(&(this->host_addr)), 0,
            this->host_addr_len);
     this->host_addr.sin_family = AF_INET;
     this->host_addr.sin_port = htons(this->port);
     memcpy(this->host->h_addr,
            reinterpret_cast<char *>(&(this->host_addr).sin_addr.s_addr),
            this->host->h_length);

     /* Convert hostname to IP address */
     /** @see https://beej.us/guide/bgnet/pdf/bgnet_a4_c_1.pdf#page=82 */
     int ip_vld = inet_pton(AF_INET, this->hostname.c_str(),
                            &(this->host_addr.sin_addr));
     if (ip_vld == 0) throw std::runtime_error("Hostname IP is not valid");
     if (ip_vld < 0)
          throw std::runtime_error("Failed to convert IP address: "
                                   + std::string(strerror(errno)));
}

TFTPClient::~TFTPClient() {
     /* Close connection socket */
     if (this->conn_fd >= 0) {
          ::close(this->conn_fd);
          conn_fd = -1;
     }

     /* If download (read) ended with error, remove file */
     if (this->is_download() && this->state == TFTPConnectionState::Errored
         && this->file_created) {
          remove(this->destpath.c_str());
     }

     /* Close file socket */
     if (this->file_fd >= 0) {
          ::close(this->file_fd);
          file_fd = -1;
     }

     Logger::glob_event("Client [" + std::to_string(this->tid) + "] closed");
}

/* === Client Flow === */

void TFTPClient::start() {
     Logger::glob_op("Starting client...");
     this->type = this->filepath.has_value() ? TFTPRequestType::Read
                                             : TFTPRequestType::Write;

     /** @see
      * https://moodle.vut.cz/pluginfile.php/550189/mod_folder/content/0/IPK2022-23L-03-PROGRAMOVANI.pdf#page=16
      */

     /* Create socket */
     this->conn_fd = socket(AF_INET, SOCK_DGRAM, 0);
     if (this->conn_fd == -1)
          throw std::runtime_error("Failed to create socket");

     Logger::glob_info("socket created with FD "
                       + std::to_string(this->conn_fd));

     /* Set address */
     memset(&(this->conn_addr), 0, this->conn_addr_len);
     this->conn_addr.sin_family = AF_INET;
     this->conn_addr.sin_port = htons(0);  // => random port
     this->conn_addr.sin_addr.s_addr = htonl(INADDR_ANY);

     /* Set timeout */
     struct timeval timeout {
          TFTP_TIMEO, 0
     };

     if (setsockopt(this->conn_fd, SOL_SOCKET, SO_RCVTIMEO,
                    reinterpret_cast<char *>(&timeout), sizeof(timeout))
             < 0
         || setsockopt(this->conn_fd, SOL_SOCKET, SO_SNDTIMEO,
                       reinterpret_cast<char *>(&timeout), sizeof(timeout))
                < 0)
          throw std::runtime_error("Failed to set socket timeout");

     /* Allow address:port reuse */
     int optval = 1;
     if (setsockopt(this->conn_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                    &optval, sizeof(optval))
         < 0)
          throw std::runtime_error("Failed to set socket options");

     /* Bind socket */
     if (bind(this->conn_fd,
              reinterpret_cast<struct sockaddr *>(&this->conn_addr),
              this->conn_addr_len)
         < 0)
          throw std::runtime_error("Failed to bind socket : "
                                   + std::string(strerror(errno)));

     /* Get OS-assigned random port */
     if (getsockname(this->conn_fd,
                     reinterpret_cast<struct sockaddr *>(&this->conn_addr),
                     &this->conn_addr_len)
         < 0)
          throw std::runtime_error("Failed to get socket name : "
                                   + std::string(strerror(errno)));
     this->tid = ntohs(this->conn_addr.sin_port);

     Logger::glob_info("socket bound to "
                       + std::string(inet_ntoa(this->conn_addr.sin_addr)) + ":"
                       + std::to_string(ntohs(this->conn_addr.sin_port)));

     /* Set up signal handler */
     /** @see https://gist.github.com/aspyct/3462238 */
     struct sigaction sig_act {};
     memset(&sig_act, 0, sizeof(sig_act));
     sig_act.sa_handler = signal_handler;
     sigfillset(&sig_act.sa_mask);
     sigaction(SIGINT, &sig_act, NULL);

     /* Start connection */
     Logger::glob_event("Setup [" + std::to_string(this->tid) + "]"
                        + " complete, starting connection...");
     this->state = TFTPConnectionState::Requesting;
     this->conn_exec();
}

void TFTPClient::conn_exec() {
     while (this->is_running()) {
          /* Check shutdown flag */
          if (quit.load()) {
               Logger::glob_event("Shutdown flag set, stopping connection...");
               return send_error(TFTPErrorCode::Unknown, "Terminated by user");
          }

          switch (this->state) {
               case TFTPConnectionState::Requesting:
                    this->is_upload() ? this->send_wrq() : this->send_rrq();
                    break;
               case TFTPConnectionState::Uploading:
                    this->handle_upload();
                    break;
               case TFTPConnectionState::Downloading:
                    this->handle_download();
                    break;
               case TFTPConnectionState::Awaiting:
                    this->is_upload() ? this->handle_await_upload()
                                      : this->handle_await_download();
                    break;
               default:
                    log_error("`run` called in invalid state");
                    return send_error(TFTPErrorCode::Unknown,
                                      "Bad internal state");
          }

          std::this_thread::sleep_for(
              std::chrono::microseconds(TFTP_THREAD_DELAY));
     }
}

/* == Uploading (WRQ) flow == */

void TFTPClient::send_wrq() {
     log_info("Requesting write to file " + this->destpath);

     /* Create request payload */
     RequestPacket packet
         = RequestPacket(TFTPRequestType::Write, this->destpath, this->format);
     auto payload = packet.to_binary();

     /* Send request */
     this->last_packet_time = std::chrono::steady_clock::now();
     sendto(this->conn_fd, payload.data(), payload.size(), 0,
            reinterpret_cast<const sockaddr *>(&this->host_addr),
            sizeof(this->host_addr));

     /* Await ACK */
     this->state = TFTPConnectionState::Awaiting;
}

void TFTPClient::handle_await_upload() {
     /* Timeout check */
     if (this->is_timedout()) {
          /* Check if TFTP_MAX_RETRIES was reached */
          if (this->send_tries + 1 >= TFTP_MAX_RETRIES) {
               log_error("Maximum number of retries reached");
               send_error(TFTPErrorCode::Unknown, "Retransmission timeout");
               return;
          }

          this->send_tries++;
          this->state = (this->block_n == 0) ? TFTPConnectionState::Requesting
                        : this->is_upload()  ? TFTPConnectionState::Uploading
                                             : TFTPConnectionState::Downloading;
          log_info("Retransmitting block " + this->get_block_n_hex()
                   + " (attempt " + std::to_string(this->send_tries + 1) + ")");
          return;
     }

     /* Receive and check packet */
     auto packet_res = this->recv_packet((this->block_n == 0));
     if (!packet_res.has_value()) return;  // Loop in state
     std::unique_ptr<BasePacket> packet_ptr = std::move(packet_res.value());

     if (packet_ptr->get_opcode() != TFTPOpcode::ACK)
          return send_error(TFTPErrorCode::IllegalOperation,
                            "Received a non-ACK packet");

     auto *ack_packet_ptr
         = dynamic_cast<AcknowledgementPacket *>(packet_ptr.get());

     log_info("Received ACK for block " + this->get_block_n_hex() + " ("
              + std::to_string(this->rx_len) + " bytes)");

     /* Check if ACK block number is as expected */
     if (ack_packet_ptr->get_block_number() < this->block_n) {
          return;  // Stray past ACK, ignore.
     } else if (ack_packet_ptr->get_block_number() > this->block_n) {
          return send_error(TFTPErrorCode::IllegalOperation,
                            "Received an ACK with a future block number");
     }

     /* Check if this is the last packet */
     if (this->is_last) {
          log_info("Upload completed!");
          this->state = TFTPConnectionState::Completed;
          return;
     }

     /* Increment block number */
     if (this->block_n == TFTP_MAX_FILE_BLOCKS)
          return send_error(TFTPErrorCode::Unknown,
                            "Block overflow (file too big)");
     this->block_n++;
     this->send_tries = 0;

     /* Continue transferring */
     this->state = TFTPConnectionState::Uploading;
}

void TFTPClient::handle_upload() {
     if (this->block_n == 0) this->block_n = 1;
     this->last_packet_time = std::chrono::steady_clock::now();

     /* Load chunk from stdin */
     std::fill(file_buffer.begin(), file_buffer.end(), 0);
     std::cin.read(file_buffer.data(), TFTP_MAX_DATA);
     this->file_rx_len = std::cin.gcount();

     /* Create data payload */
     DataPacket packet
         = DataPacket(std::vector<char>(file_buffer.begin(),
                                        file_buffer.begin() + file_rx_len),
                      this->block_n);
     packet.set_no_seek(true);
     packet.set_mode(this->format);
     auto payload = packet.to_binary();
     if (payload.size() < TFTP_MAX_DATA + 4) {
          this->is_last = true;
     } else {
          this->is_last = false;
     }

     log_info("Sending DATA block " + this->get_block_n_hex() + " ("
              + std::to_string(payload.size()) + " bytes)");

     /* Send data */
     sendto(this->conn_fd, payload.data(), payload.size(), 0,
            reinterpret_cast<const sockaddr *>(&this->host_addr),
            sizeof(this->host_addr));

     /* And now, await acknowledgement */
     log_info("Awaiting ACK for block " + this->get_block_n_hex());
     this->state = TFTPConnectionState::Awaiting;
}

/* == Downloading (RRQ) flow == */

void TFTPClient::send_rrq() {
     log_info("Requesting read from file " + this->filepath.value());

     /* Create a part file */
     this->file_fd = open(this->destpath.c_str(), O_WRONLY | O_CREAT | O_TRUNC,
                          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

     /* Check if file was created properly */
     if (this->file_fd < 0)
          return send_error(TFTPErrorCode::AccessViolation,
                            "Failed to create file");
     this->file_created = true;

     /* Create request payload */
     RequestPacket packet = RequestPacket(TFTPRequestType::Read,
                                          this->filepath.value(), this->format);
     auto payload = packet.to_binary();

     /* Send request */
     this->last_packet_time = std::chrono::steady_clock::now();
     sendto(this->conn_fd, payload.data(), payload.size(), 0,
            reinterpret_cast<const sockaddr *>(&this->host_addr),
            sizeof(this->host_addr));

     /* Await DATA */
     this->state = TFTPConnectionState::Awaiting;
}

void TFTPClient::handle_await_download() {
     /* Timeout check */
     if (this->is_timedout()) {
          /* Check if TFTP_MAX_RETRIES was reached */
          if (this->send_tries + 1 >= TFTP_MAX_RETRIES) {
               log_error("Maximum number of retries reached");
               send_error(TFTPErrorCode::Unknown, "Retransmission timeout");
               return;
          }

          log_info("Retransmitting block " + this->get_block_n_hex()
                   + " (attempt " + std::to_string(this->send_tries + 1) + ")");

          this->send_tries++;
          this->state = (this->block_n == 0) ? TFTPConnectionState::Requesting
                        : this->is_upload()  ? TFTPConnectionState::Uploading
                                             : TFTPConnectionState::Downloading;
          return;
     }

     /* Receive and check packet */
     auto packet_res = this->recv_packet((this->block_n == 0));
     if (!packet_res.has_value()) return;  // Loop in state
     std::unique_ptr<BasePacket> packet_ptr = std::move(packet_res.value());

     if (packet_ptr->get_opcode() != TFTPOpcode::DATA)
          return send_error(TFTPErrorCode::IllegalOperation,
                            "Received a non-DATA packet");

     /* Increment block number */
     if (this->block_n == TFTP_MAX_FILE_BLOCKS)
          return send_error(TFTPErrorCode::IllegalOperation,
                            "Block overflow (file too big)");
     this->block_n++;
     this->send_tries = 0;

     /* Go to writing state */
     this->state = TFTPConnectionState::Downloading;
}

void TFTPClient::handle_download() {
     AcknowledgementPacket ack = AcknowledgementPacket(this->block_n);
     this->last_packet_time = std::chrono::steady_clock::now();

     /* If `block_n` is 0 or buffer is empty (=> retransmit), just send ACK */
     if (this->block_n == 0 || this->rx_len <= 0) {
          log_info("Sending ACK for block " + this->get_block_n_hex());

          auto payload = ack.to_binary();
          if (sendto(this->conn_fd, payload.data(), payload.size(), 0,
                     reinterpret_cast<const sockaddr *>(&this->host_addr),
                     sizeof(this->host_addr))
              < 0)
               return this->send_error(TFTPErrorCode::Unknown,
                                       "Failed to send ACK for block 0");

          /* Await next data block */
          this->state = TFTPConnectionState::Awaiting;
          return;
     }

     /* Parse data from buffer */
     DataPacket packet = DataPacket::from_binary(std::vector<char>(
         this->rx_buffer.begin(), this->rx_buffer.begin() + this->rx_len));
     auto data = packet.get_data();

     /* Convert to NetASCII if needed */
     if (this->format == TFTPDataFormat::NetASCII && !data.empty()) {
          /* Adjustment for [... CR] | [LF/NUL ...] block split*/
          if (this->last_data_cr && data[0] == '\n') {
               /* CR | LF -> LF */
               off_t file_size = lseek(this->file_fd, 0, SEEK_END);
               if (file_size > 0 && ftruncate(this->file_fd, file_size - 1))
                    return this->send_error(TFTPErrorCode::AccessViolation,
                                            "Failed to truncate file");
               lseek(this->file_fd, 0, SEEK_END);
          } else if (this->last_data_cr && data[0] == '\0') {
               /* CR | NUL -> CR */
               data.erase(data.begin());
          }

          data = NetASCII::na_to_vec(data);
     }

     log_info("Received block " + this->get_block_n_hex() + " ("
              + std::to_string(data.size()) + " bytes)");

     /* Write data to file */
     if (write(this->file_fd, data.data(), data.size()) < 0)
          return this->send_error(TFTPErrorCode::AccessViolation,
                                  "Failed to write to file");

     this->last_data_cr = (!data.empty() && data.back() == '\r');

     /* Clear buffer */
     this->rx_buffer.fill(0);
     this->rx_len = 0;

     /* Send acknowledgement */
     log_info("Sending ACK for block " + this->get_block_n_hex());
     auto payload = ack.to_binary();
     if (sendto(this->conn_fd, payload.data(), payload.size(), 0,
                reinterpret_cast<const sockaddr *>(&this->host_addr),
                sizeof(this->host_addr))
         < 0)
          return this->send_error(
              TFTPErrorCode::Unknown,
              "Failed to send ACK for block " + this->get_block_n_hex());

     /* Check if this was the last data block */
     if (packet.get_data().size() < TFTP_MAX_DATA) {
          log_info("Download completed!");
          this->state = TFTPConnectionState::Completed;
          return;
     }

     /* Await next data block */
     this->state = TFTPConnectionState::Awaiting;
}

/* === Helper Methods === */

std::optional<std::unique_ptr<BasePacket>> TFTPClient::recv_packet(
    bool addr_overwrite) {
     struct sockaddr_in origin_addr {};
     socklen_t origin_addr_len = sizeof(origin_addr);

     /* Receive packet */
     this->rx_len = recvfrom(
         this->conn_fd, this->rx_buffer.data(), TFTP_MAX_PACKET, 0,
         reinterpret_cast<struct sockaddr *>(&origin_addr), &origin_addr_len);

     /* Handle errors */
     if (rx_len < 0) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
               /* Nothing to receive, loop in state */
               return std::nullopt;
          }

          log_error("Failed to receive packet:");
          send_error(TFTPErrorCode::Unknown, std::string(strerror(errno)));
          return std::nullopt;
     }

     /* Parse incoming packet */
     auto packet_ptr = PacketFactory::create(this->rx_buffer, rx_len);
     if (!packet_ptr) {
          send_error(TFTPErrorCode::IllegalOperation,
                     "Received an invalid packet");
          return std::nullopt;
     }

     Logger::packet(*packet_ptr, origin_addr, this->conn_addr);

     /* Check TID/origin-client match */
     if (!addr_overwrite && !is_host_addr(origin_addr)) {
          log_info("Received packet from unexpected origin");
          /* Packet from unexpected origin – ignore, clear buffer */
          this->rx_buffer.fill(0);
          this->rx_len = 0;
          return std::nullopt;
     }

     /* Overwrite host_addr if applicable */
     if (addr_overwrite) {
          this->host_addr = origin_addr;
          this->host_addr_len = origin_addr_len;
     }

     return packet_ptr;
}

void TFTPClient::send_error(TFTPErrorCode code, const std::string &message) {
     log_error(message);

     ErrorPacket res = ErrorPacket(code, message);
     auto payload = res.to_binary();

     sendto(this->conn_fd, payload.data(), payload.size(), 0,
            reinterpret_cast<const sockaddr *>(&this->host_addr),
            sizeof(this->host_addr));

     this->state = TFTPConnectionState::Errored;
}
