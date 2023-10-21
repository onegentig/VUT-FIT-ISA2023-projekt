/**
 * @file server.hpp
 * @author Filip J. Kramec (xkrame00@vutbr.cz)
 * @brief TFTP server implementation.
 * @date 2023-10-21
 */

#include "server.hpp"

#include <fcntl.h>

/* === Constructors === */

TFTPServer::TFTPServer() : port(DEFAULT_TFTP_PORT), rootdir("./") {
     this->running.store(false);
}

TFTPServer::TFTPServer(std::string rootdir)
    : port(DEFAULT_TFTP_PORT), rootdir(rootdir) {
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

bool TFTPServer::start() {
     /** @see
      * https://moodle.vut.cz/pluginfile.php/550189/mod_folder/content/0/IPK2022-23L-03-PROGRAMOVANI.pdf#page=21
      */

     /* Create socket */
     this->fd = socket(AF_INET, SOCK_DGRAM, 0);
     if (this->fd == -1) {
          std::cerr << "!ERR! Failed to create a socket!" << std::endl;
          return false;
     }

     /* Set up address */
     memset(&(this->addr), 0, this->addr_len);
     this->addr.sin_family = AF_INET;
     this->addr.sin_port = htons(port);
     this->addr.sin_addr.s_addr = htonl(INADDR_ANY);

     /* Set up timeout */
     struct timeval timeout {
          TIMEOUT, 0
     };

     if (setsockopt(this->fd, SOL_SOCKET, SO_RCVTIMEO,
                    reinterpret_cast<char*>(&timeout), sizeof(timeout))
         < 0) {
          std::cerr << "!ERR! Failed to set socket timeout!" << std::endl;
          return false;
     }

     /* Allow address:port reuse */
     int optval = 1;
     if (setsockopt(this->fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optval,
                    sizeof(optval))
         < 0) {
          std::cerr << "!ERR! Failed to set socket options!" << std::endl;
          return false;
     }

     /* Bind socket */
     if (bind(this->fd, reinterpret_cast<struct sockaddr*>(&this->addr),
              addr_len)
         < 0) {
          std::cerr << "!ERR! Failed to bind socket!" << std::endl;
          return false;
     }

     /* Make the listening socket non-blocking */
     int flags = fcntl(this->fd, F_GETFL, 0);
     if (flags < 0) {
          std::cerr << "!ERR! Failed to get socket flags!" << std::endl;
          return false;
     }
     flags |= O_NONBLOCK;
     if (fcntl(this->fd, F_SETFL, flags) < 0) {
          std::cerr << "!ERR! Failed to set socket flags!" << std::endl;
          return false;
     }

     /* Listen */
     connListen();
}

void TFTPServer::connListen() {
     this->running.store(true);
     std::cout << "TFTP server listening on port " << this->port << std::endl;

     /** @see
      * https://moodle.vut.cz/pluginfile.php/550189/mod_folder/content/0/IPK2022-23L-03-PROGRAMOVANI.pdf#page=23
      */
     while (this->running.load()) {
          // TODO: Implement
     }
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