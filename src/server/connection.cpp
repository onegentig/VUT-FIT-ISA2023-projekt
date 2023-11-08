/**
 * @file connection.cpp
 * @author Onegen Something (xkrame00@vutbr,cz)
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

     /* Set timeout */
     struct timeval timeout {
          TFTP_TIMEO, 0
     };

     if (setsockopt(this->fd, SOL_SOCKET, SO_RCVTIMEO,
                    reinterpret_cast<char*>(&timeout), sizeof(timeout))
         < 0) {
          this->state = ConnectionState::DENIED;
          return;
     }

     /* Fill out remaining attributes */
     filePath = rootDir + reqPacket.getFilename();
     type = reqPacket.getType();
     format = reqPacket.getMode();
}

TFTPServerConnection::~TFTPServerConnection() {
     /*if (this->fd >= 0) {
          ::close(this->fd);
          this->fd = -1;
     }*/
}

/* === Connection Flow === */

void TFTPServerConnection::run() {
     // TODO: implement
     ErrorPacket res
         = ErrorPacket(TFTPErrorCode::Unknown, "Response not implemented");
     auto payload = res.toBinary();

     if (sendto(this->fd, payload.data(), payload.size(), 0,
                reinterpret_cast<const sockaddr*>(&this->addr),
                sizeof(this->addr))
         < 0) {
          std::cerr << "!ERR! Failed to send response!" << std::endl;
     }
}
