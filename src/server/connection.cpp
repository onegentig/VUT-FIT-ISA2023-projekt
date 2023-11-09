/**
 * @file connection.cpp
 * @author Filip J. Kramec (xkrame00@vutbr,cz)
 * @brief TFTP connection implementation.
 * @date 2023-11-08
 */

#include "server/connection.hpp"

/* === Constructors === */

TFTPServerConnection::TFTPServerConnection(int fd, const sockaddr_in& addr,
                                           const RequestPacket& reqPacket,
                                           const std::string& rootDir)
    : fd(fd), addr(addr) {
     this->state = ConnectionState::REQUESTED;
     file_path = rootDir + reqPacket.get_filename();
     type = reqPacket.get_type();
     format = reqPacket.get_mode();
}

TFTPServerConnection::~TFTPServerConnection() {
     std::cout << "==> Connection closed" << std::endl;

     /*if (this->fd >= 0) {
          ::close(this->fd);
          this->fd = -1;
     }*/
}

/* === Connection Flow === */

void TFTPServerConnection::run() {
     // TODO: implement
     std::cout << "  Not implemented, sending ERROR" << std::endl;

     ErrorPacket res
         = ErrorPacket(TFTPErrorCode::Unknown, "Response not implemented");
     auto payload = res.to_binary();

     if (sendto(this->fd, payload.data(), payload.size(), 0,
                reinterpret_cast<const sockaddr*>(&this->addr),
                sizeof(this->addr))
         < 0) {
          std::cerr << "!ERR! Failed to send response!" << std::endl;
     }

     this->state = ConnectionState::DENIED;
}
