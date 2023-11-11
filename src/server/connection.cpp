/**
 * @file connection.cpp
 * @author Filip J. Kramec (xkrame00@vutbr,cz)
 * @brief TFTP connection implementation.
 * @date 2023-11-08
 */

#include "server/connection.hpp"

/* === Constructors === */

TFTPServerConnection::TFTPServerConnection(int srv_fd,
                                           const sockaddr_in& clt_addr,
                                           const RequestPacket& reqPacket,
                                           const std::string& rootDir)
    : srv_fd(srv_fd), clt_addr(clt_addr) {
     this->state = ConnectionState::REQUESTED;
     file_path = rootDir + "/" + reqPacket.get_filename();
     type = reqPacket.get_type();
     format = reqPacket.get_mode();

     /* Create socket */
     this->conn_fd = socket(AF_INET, SOCK_DGRAM, 0);
     if (this->conn_fd == -1) {
          std::cerr << "!ERR! Failed to create a socket!" << std::endl;
          this->state = ConnectionState::ERRORED;
          return;
     }

     /* Set up address */
     memset(&(this->conn_addr), 0, this->conn_addr_len);
     this->conn_addr.sin_family = AF_INET;
     this->conn_addr.sin_port = htons(0);  // Random port
     this->conn_addr.sin_addr.s_addr = htonl(INADDR_ANY);

     /* Set up timeout */
     struct timeval timeout {
          TFTP_TIMEO, 0
     };

     if (setsockopt(this->conn_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                    sizeof(timeout))
         < 0) {
          std::cerr << "!ERR! Failed to set timeout!" << std::endl;
          this->state = ConnectionState::ERRORED;
          return;
     }

     /* Allow address:port reuse */
     int optval = 1;
     if (setsockopt(this->conn_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                    &optval, sizeof(optval))
         < 0) {
          std::cerr << "!ERR! Failed to set socket options!" << std::endl;
          this->state = ConnectionState::ERRORED;
          return;
     }

     /* Bind socket */
     if (bind(this->conn_fd,
              reinterpret_cast<struct sockaddr*>(&this->conn_addr),
              this->conn_addr_len)
         < 0) {
          std::cerr << "!ERR! Failed to bind socket!"
                    << " : " << strerror(errno) << std::endl;
          this->state = ConnectionState::ERRORED;
          return;
     }

     /* Get OS-assigned random port */
     if (getsockname(this->conn_fd,
                     reinterpret_cast<struct sockaddr*>(&this->conn_addr),
                     &(this->conn_addr_len))
         < 0) {
          std::cerr << "!ERR! Failed to get socket name!"
                    << " : " << strerror(errno) << std::endl;
          this->state = ConnectionState::ERRORED;
          return;
     }

     this->tid = ntohs(this->conn_addr.sin_port);
     std::cout << "==> New connection [" << this->tid << "]"
               << " from " << inet_ntoa(this->clt_addr.sin_addr) << ":"
               << ntohs(this->clt_addr.sin_port) << std::endl;
}

TFTPServerConnection::~TFTPServerConnection() {
     if (this->conn_fd >= 0) {
          ::close(this->conn_fd);
          this->conn_fd = -1;
     }

     if (this->file_fd >= 0) {
          ::close(this->file_fd);
          this->file_fd = -1;
     }

     std::cout << "==> Closing connection [" << this->tid << "]" << std::endl;
}

/* === Connection Flow === */

void TFTPServerConnection::run() {
     while (this->is_running()) {
          switch (this->state) {
               case ConnectionState::REQUESTED:
                    this->type == TFTPRequestType::Read ? this->handle_rrq()
                                                        : this->handle_wrq();
                    break;
               case ConnectionState::UPLOADING:
                    this->handle_upload();
                    break;
               case ConnectionState::AWAITING:
                    this->handle_await();
                    break;
               default:
                    // TODO: implement everything
                    log_error("Reached a non-implemented state");
                    this->send_error(TFTPErrorCode::Unknown, "Not implemented");
                    break;
          }

          // TODO: This could be reworked to `select` or `poll` for better perf
          std::this_thread::sleep_for(
              std::chrono::microseconds(TFTP_THREAD_DELAY));
     }
}

void TFTPServerConnection::handle_rrq() {
     log_info("Requesting file " + this->file_path);

     /* Open file for reading */
     this->file_fd = open(this->file_path.c_str(), O_RDONLY);
     if (this->file_fd < 0) {
          // TODO: Separate FileNotFound and AccessDenied cases
          log_error("Failed to open file");
          return this->send_error(TFTPErrorCode::FileNotFound,
                                  "Failed to open file");
     }

     /* Things are ready for transfer */
     log_info("File ready, starting upload");
     this->state = ConnectionState::UPLOADING;
}

void TFTPServerConnection::handle_wrq() {
     // TODO: implement
     log_error("WRQ handling not implemented");
     this->send_error(TFTPErrorCode::Unknown, "Not implemented");
}

void TFTPServerConnection::handle_upload() {
     if (this->block_n == 0) this->block_n = 1;

     /* Create data payload */
     DataPacket packet = DataPacket(this->file_fd, this->block_n);
     packet.set_mode(this->format);
     auto payload = packet.to_binary();
     if (payload.size() < TFTP_MAX_DATA + 4) {
          this->is_last.store(true);
     } else {
          this->is_last.store(false);
     }

     log_info("Sending block " + std::to_string(this->block_n) + " ("
              + std::to_string(payload.size()) + " bytes, is_last is "
              + std::to_string(this->is_last.load()) + ")");

     /* Send data */
     if (sendto(this->srv_fd, payload.data(), payload.size(), 0,
                reinterpret_cast<const sockaddr*>(&this->clt_addr),
                sizeof(this->clt_addr))
         < 0) {
          this->state
              = ConnectionState::TIMEDOUT;  // TIMEDOUT state will attempt to
                                            // resend the packet
          return;
     }

     /* And now, await acknowledgement */
     log_info("Awaiting ACK for block " + std::to_string(this->block_n));
     this->state = ConnectionState::AWAITING;
}

void TFTPServerConnection::handle_await() {
     /* Receive ACK */
     ssize_t bytes_rx
         = recvfrom(this->srv_fd, this->buffer.data(), TFTP_MAX_PACKET, 0,
                    reinterpret_cast<struct sockaddr*>(&this->clt_addr),
                    &this->clt_addr_len);

     /* Handle errors */
     if (bytes_rx < 0) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
               /* Nothing to receive, loop in state */
               return;
          }

          log_error("Failed to receive ACK: " + std::string(strerror(errno)));
          send_error(TFTPErrorCode::Unknown, std::string(strerror(errno)));
          return;
     }

     /* Parse incoming packet */
     auto packet_ptr = PacketFactory::create(this->buffer, bytes_rx);
     if (!packet_ptr) {
          log_error("Received an unparsable packet");
          send_error(TFTPErrorCode::IllegalOperation,
                     "Received an invalid packet");
          return;
     }

     if (packet_ptr->get_opcode() != TFTPOpcode::ACK) {
          log_error("Received a non-ACK packet");
          send_error(TFTPErrorCode::IllegalOperation,
                     "Received a non-ACK packet");
          return;
     }

     auto* ack_packet_ptr
         = dynamic_cast<AcknowledgementPacket*>(packet_ptr.get());

     log_info("Received ACK for block " + std::to_string(this->block_n) + " ("
              + std::to_string(bytes_rx) + " bytes, is_last is "
              + std::to_string(this->is_last.load()) + ")");

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
          this->state = ConnectionState::COMPLETED;
          return;
     }

     /* Increment block number */
     this->block_n++;

     /* Continue transferring */
     this->state = is_upload() ? ConnectionState::UPLOADING
                               : ConnectionState::DOWNLOADING;
}

/* === Helper Methods === */

void TFTPServerConnection::send_error(TFTPErrorCode code,
                                      const std::string& message) {
     ErrorPacket res = ErrorPacket(code, message);
     auto payload = res.to_binary();

     sendto(this->srv_fd, payload.data(), payload.size(), 0,
            reinterpret_cast<const sockaddr*>(&this->clt_addr),
            sizeof(this->clt_addr));

     this->state = ConnectionState::ERRORED;
}
