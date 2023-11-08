/**
 * @file connection.hpp
 * @author Filip J. Kramec (xkrame00@vutbr,cz)
 * @brief TFTP connection implementation.
 * @date 2023-11-08
 */

#pragma once
#ifndef TFTErrorCTION_HPP
#     define TFTP_CONNECTION_HPP

#     include "common.hpp"
#     include "packet/ErrorPacket.hpp"
#     include "packet/RequestPacket.hpp"

/**
 * @brief Enumeration to represent possible states of a connection
 */
enum class ConnectionState {
     REQUESTED, /**< Initial transitory state entered on WRQ/RRQ */
     READING,   /**< TRANSFERING state on reading */
     WRITING,   /**< TRANSFERING state on writing */
     TIMEDOUT,  /**< Lost packet state; attempts retransmit */
     DENIED,    /**< Request not fulfillable; terminal state*/
     ERRORED,   /**< Mid-transfer error; terminal state */
     COMPLETED  /**< Transfer completed; terminal state */
};

/**
 * @brief Base class for TFTP server connections
 */
class TFTPServerConnection {
   public:
     /**
      * @brief Constructs a new TFTPServerConnection object
      * @param fd Client socket file descriptor
      * @param addr Client address
      * @param reqPacket Request packet (RRQ / WRQ)
      * @param rootDir Server root directory
      */
     TFTPServerConnection(int fd, const sockaddr_in& addr,
                          const RequestPacket& reqPacket,
                          const std::string& rootDir);

     /**
      * @brief Destroys the TFTPServerConnection object
      */
     ~TFTPServerConnection();

     TFTPServerConnection& operator=(TFTPServerConnection&& other) = default;
     TFTPServerConnection& operator=(const TFTPServerConnection&) = default;
     TFTPServerConnection(TFTPServerConnection&& other) = default;
     TFTPServerConnection(const TFTPServerConnection&) = default;

     /* === Core Methods === */

     /**
      * @brief Runs the connection
      */
     void run();

     /**
      * @brief Checks if the connection is running
      * @return true if running
      * @return false otherwise
      */
     bool isRunning() const {
          return this->state != ConnectionState::COMPLETED
                 && this->state != ConnectionState::DENIED
                 && this->state != ConnectionState::ERRORED;
     }

     /**
      * @brief Checks if the connection is transfering
      * @return true if transfering
      * @return false otherwise
      */
     bool isTransfering() const {
          return this->state == ConnectionState::READING
                 || this->state == ConnectionState::WRITING;
     }

     /**
      * @brief Checks if the connection is an upload (read from server)
      * @return true if upload
      * @return false otherwise
      */
     bool isUpload() const { return this->type == TFTPRequestType::Read; }

     /**
      * @brief Checks if the connection is a download (write to server)
      * @return true if download
      * @return false otherwise
      */
     bool isDownload() const { return this->type == TFTPRequestType::Write; }

   protected:
     int fd;                            /**< Client socket file descriptor */
     struct sockaddr_in addr {};        /**< Address of the client */
     std::string filePath;              /**< Path to the file */
     TFTPRequestType type;              /**< Request type (RRQ / WRQ) */
     TFTPDataFormat format;             /**< Transfer format */
     ConnectionState state;             /**< State of the connection */
     socklen_t addr_len = sizeof(addr); /**< Length of the address */
};

#endif