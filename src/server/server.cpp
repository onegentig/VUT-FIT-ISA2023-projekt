/**
 * @file server.hpp
 * @author Filip J. Kramec (xkrame00@vutbr.cz)
 * @brief TFTP server implementation.
 * @date 2023-10-21
 */

#include "server.hpp"

#include <fcntl.h>

/* === Constructors === */

TFTPServer::TFTPServer() : port(TFTP_PORT), rootdir("./") {
     this->running.store(false);
}

TFTPServer::TFTPServer(std::string rootdir)
    : port(TFTP_PORT), rootdir(rootdir) {
     /* Verify root directory */
     if (!this->validateDir())
          throw std::runtime_error("Invalid root directory");

     this->running.store(false);
}

TFTPServer::TFTPServer(std::string rootdir, int port)
    : port(port), rootdir(rootdir) {
     /* Verify port number */
     if (port < 1 || port > 65535)
          throw std::runtime_error("Invalid port number");

     /* Verify root directory */
     if (!this->validateDir())
          throw std::runtime_error("Invalid root directory");

     this->running.store(false);
}

/* === Server Flow === */

void TFTPServer::start() {
     /** @see
      * https://moodle.vut.cz/pluginfile.php/550189/mod_folder/content/0/IPK2022-23L-03-PROGRAMOVANI.pdf#page=21
      */

     /* Create socket */
     this->fd = socket(AF_INET, SOCK_DGRAM, 0);
     if (this->fd == -1) {
          std::cerr << "!ERR! Failed to create a socket!" << std::endl;
          return;
     }

     /* Set up address */
     memset(&(this->addr), 0, this->addr_len);
     this->addr.sin_family = AF_INET;
     this->addr.sin_port = htons(port);
     this->addr.sin_addr.s_addr = htonl(INADDR_ANY);

     /* Set up timeout */
     struct timeval timeout {
          TFTP_TIMEO, 0
     };

     if (setsockopt(this->fd, SOL_SOCKET, SO_RCVTIMEO,
                    reinterpret_cast<char*>(&timeout), sizeof(timeout))
         < 0) {
          std::cerr << "!ERR! Failed to set socket timeout!" << std::endl;
          return;
     }

     /* Allow address:port reuse */
     int optval = 1;
     if (setsockopt(this->fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optval,
                    sizeof(optval))
         < 0) {
          std::cerr << "!ERR! Failed to set socket options!" << std::endl;
          return;
     }

     /* Bind socket */
     if (bind(this->fd, reinterpret_cast<struct sockaddr*>(&this->addr),
              addr_len)
         < 0) {
          std::cerr << "!ERR! Failed to bind socket!" << std::endl;
          return;
     }

     /* Make the listening socket non-blocking */
     int flags = fcntl(this->fd, F_GETFL, 0);
     if (flags < 0) {
          std::cerr << "!ERR! Failed to get socket flags!" << std::endl;
          return;
     }
     flags |= O_NONBLOCK;
     if (fcntl(this->fd, F_SETFL, flags) < 0) {
          std::cerr << "!ERR! Failed to set socket flags!" << std::endl;
          return;
     }

     /* Listen */
     connListen();
}

void TFTPServer::connListen() {
     this->running.store(true);
     std::cout << "TFTP server listening on port " << this->port << std::endl;

     // TODO: This logic should be moved to the loop below
     struct sockaddr_in c_addr {};
     socklen_t c_addr_len = sizeof(c_addr);
     std::array<uint8_t, TFTP_MAX_PACKET> buffer{0};

     /** @see
      * https://moodle.vut.cz/pluginfile.php/550189/mod_folder/content/0/IPK2022-23L-03-PROGRAMOVANI.pdf#page=23
      */
     while (this->running.load()) {
          // TODO: All this is just a "for now" thing for testing and all that
          memset(buffer.data(), 0, buffer.size());
          memset(&c_addr, 0, c_addr_len);

          ssize_t read_size = recvfrom(
              this->fd, buffer.data(), TFTP_MAX_PACKET, 0,
              reinterpret_cast<struct sockaddr*>(&c_addr), &c_addr_len);

          if (read_size < 0) {
               if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    /* No new connections */
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(TFTP_THREAD_DELAY));
                    continue;
               }

               std::cerr << "Failed to receive data: " << strerror(errno)
                         << std::endl;
               continue;
          }

          std::cout << "Received " << read_size << " bytes from "
                    << inet_ntoa(c_addr.sin_addr) << ":"
                    << ntohs(c_addr.sin_port) << std::endl;

          /* Short sleep (makes the loop a bit less CPU-heavy) */
          std::this_thread::sleep_for(
              std::chrono::milliseconds(TFTP_THREAD_DELAY));
     }
}

void TFTPServer::stop() {
     this->running.store(false);
     std::cout << "Stopping server..." << std::endl;

     shutdown(this->fd, SHUT_RDWR);
     close(this->fd);
     this->fd = -1;
}

/* === Helper Methods === */

bool TFTPServer::validateDir() const {
     struct stat sb;

     /** @see
      * https://www.geeksforgeeks.org/how-to-check-a-file-or-directory-exists-in-cpp/
      */
     if (stat(this->rootdir.c_str(), &sb) != 0) {
          std::cerr << "!ERR! '" << this->rootdir << "' does not seem exist!"
                    << std::endl;
          return false;
     } else if (!(sb.st_mode & S_IFDIR)) {
          std::cerr << "!ERR! '" << this->rootdir << "' is not a directory!"
                    << std::endl;
          return false;
     } else if (access(this->rootdir.c_str(), R_OK) != 0) {
          std::cerr << "!ERR! '" << this->rootdir << "' is not readable!"
                    << std::endl;
          return false;
     } else if (access(this->rootdir.c_str(), W_OK) != 0) {
          std::cerr << "!ERR! '" << this->rootdir << "' is not writable!"
                    << std::endl;
          return false;
     }

     return true;
}