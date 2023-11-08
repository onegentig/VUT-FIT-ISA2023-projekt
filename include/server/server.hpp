/**
 * @file server.hpp
 * @author Filip J. Kramec (xkrame00@vutbr.cz)
 * @brief TFTP server implementation.
 * @date 2023-10-21
 */

#pragma once
#ifndef TFTP_SERVER_HPP
#     define TFTP_SERVER_HPP
#     include <sys/stat.h>

#     include <csignal>
#     include <memory>
#     include <thread>

#     include "common.hpp"
#     include "server/connection.hpp"

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
          if (this->running.load()) this->stop();
     }

     TFTPServer& operator=(TFTPServer&& other) = default;
     TFTPServer& operator=(const TFTPServer&) = default;
     TFTPServer(TFTPServer&& other) = default;
     TFTPServer(const TFTPServer&) = default;

     /* === Core Methods === */

     /**
      * @brief Starts the TFTP server.
      * @return true when started successfully,
      * @return false otherwise
      */
     void start();

     /**
      * @brief Stops the TFTP server.
      */
     void stop();

   private:
     int fd;                            /**< Socket file descriptor */
     int port;                          /**< Port to listen on */
     std::string rootdir;               /**< Root directory of the server */
     std::atomic<bool> running;         /**< Server running flag */
     struct sockaddr_in addr {};        /**< Socket address */
     socklen_t addr_len = sizeof(addr); /**< Socket address length */
     std::vector<std::shared_ptr<TFTPServerConnection>>
         connections; /**< Connection thread pool */

     /**
      * @brief Listens for incoming connections.
      */
     void connListen();

     /**
      * @brief Validates the rootdir (must be a readable
      * and writable directory)
      * @return true when valid,
      * @return false otherwise
      */
     bool validateDir() const;
};

#endif
