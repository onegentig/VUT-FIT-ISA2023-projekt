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
#     include <poll.h>
#     include <sys/stat.h>

#     include <csignal>
#     include <memory>

#     include "common.hpp"
#     include "server/connection.hpp"
#     include "util/logger.hpp"

#     define POLL_TIMEO 1000

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
     explicit TFTPServer(std::string rootdir, int port);

     /**
      * @brief Deconstructs the TFTP server object.
      */
     ~TFTPServer() {
          if (this->fd > 0) this->stop();
     }

     TFTPServer& operator=(TFTPServer&& other) = delete;
     TFTPServer& operator=(const TFTPServer&) = delete;
     TFTPServer(TFTPServer&& other) = delete;
     TFTPServer(const TFTPServer&) = delete;

     /* === Core Methods === */

     /**
      * @brief Starts the TFTP server
      */
     void start();

     /**
      * @brief Stops the TFTP server
      */
     void stop();

   private:
     /* === Core methods === */

     /**
      * @brief Listens for incoming connections
      */
     void srv_poll();

     /**
      * @brief Handles a new incoming connection
      */
     void new_conn();

     /**
      * @brief Removes a connection from `connections` and `fds`
      */
     void conn_remove(int fd);

     /**
      * @brief Cleanup all finished connections
      */
     void conn_cleanup();

     /* === Helper methods === */

     /**
      * @brief Validates that the rootdir is a valid, readable and writable
      * directory
      * @return true when valid,
      * @return false otherwise
      */
     bool check_dir() const;

     /**
      * @brief Finds a TFTPServerConnection by its socket file descriptor
      * @return shared_ptr<TFTPServerConnection>* when found,
      * @return NULL otherwise
      */
     std::shared_ptr<TFTPServerConnection>* find_conn(int fd);

     /* === Variables === */

     /* == Server config and fd == */
     int port;            /**< Port to listen on */
     std::string rootdir; /**< Root directory of the server */

     /* == Poll == */
     std::vector<struct pollfd> fds; /**< Poll file descriptors */
     struct pollfd srv_fd {};        /**< Server file descriptor */

     /* == Server address and fd == */
     int fd = -1;                       /**< Socket file descriptor */
     struct sockaddr_in addr {};        /**< Socket address */
     socklen_t addr_len = sizeof(addr); /**< Socket address length */

     /* == Other == */
     std::vector<std::shared_ptr<TFTPServerConnection>>
         connections; /**< Connections vector */
     std::shared_ptr<std::atomic<bool>>
         shutd_flag; /**< Flag to signal shutdown */
};

#endif
