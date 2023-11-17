/**
 * @file server.hpp
 * @author Onegen Something (xkrame00@vutbr.cz)
 * @brief TFTP server implementation.
 * @date 2023-10-21
 */

#pragma once
#ifndef TFTP_SERVER_HPP
#     define TFTP_SERVER_HPP
#     include <fcntl.h>
#     include <sys/stat.h>

#     include <csignal>
#     include <memory>

#     include "common.hpp"
#     include "server/connection.hpp"
#     include "util/logger.hpp"

/**
 * @brief Class for TFTP server.
 */
class TFTPServer {
   public:
     /**
      * @brief Constructs a new TFTP server object.
      * @return TFTPServer
      */
     TFTPServer();

     /**
      * @brief Constructs a new TFTP server object with set root directory.
      * @param std::string rootdir
      * @return TFTPServer
      */
     explicit TFTPServer(std::string rootdir);

     /**
      * @brief Constructs a new TFTP server object with set root directory and
      * port.
      * @param std::string rootdir
      * @param int port
      * @return TFTPServer
      */
     TFTPServer(std::string rootdir, int port);

     /**
      * @brief Deconstructs the TFTP server object.
      */
     ~TFTPServer() {
          if (this->fd > 0) this->stop();
     }

     TFTPServer& operator=(TFTPServer&& other) = default;
     TFTPServer& operator=(const TFTPServer&) = default;
     TFTPServer(TFTPServer&& other) = default;
     TFTPServer(const TFTPServer&) = default;

     /* === Core Methods === */

     /**
      * @brief Starts the TFTP server
      * @return true when started successfully,
      * @return false otherwise
      */
     void start();

     /**
      * @brief Stops the TFTP server
      */
     void stop();

   private:
     int fd;                            /**< Socket file descriptor */
     int port;                          /**< Port to listen on */
     std::string rootdir;               /**< Root directory of the server */
     struct sockaddr_in addr {};        /**< Socket address */
     socklen_t addr_len = sizeof(addr); /**< Socket address length */
     std::vector<std::shared_ptr<TFTPServerConnection>>
         connections; /**< Connection thread vector */
     std::shared_ptr<std::atomic<bool>>
         shutd_flag; /**< Flag to signal shutdown */

     /**
      * @brief Listens for incoming connections
      */
     void conn_listen();

     /**
      * @brief Validates that the rootdir is a valid, readable and writable
      * directory
      * @return true when valid,
      * @return false otherwise
      */
     bool check_dir() const;
};

#endif
