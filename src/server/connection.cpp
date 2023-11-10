/**
 * @file connection.cpp
 * @author Onegen Something (xkrame00@vutbr,cz)
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
               default:
                    // TODO: implement everything
                    log_error("Reached a non-implemented state");
                    this->send_error(TFTPErrorCode::Unknown, "Not implemented");
                    break;
          }
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
