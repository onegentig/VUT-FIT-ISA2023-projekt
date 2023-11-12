/**
 * @file connection.hpp
 * @author Onegen Something (xkrame00@vutbr,cz)
 * @brief TFTP connection implementation.
 * @date 2023-11-08
 */

#pragma once
#ifndef TFTP_CONNECTION_HPP
#     define TFTP_CONNECTION_HPP

#     include <thread>

#     include "common.hpp"
#     include "packet/PacketFactory.hpp"

/**
 * @brief Enumeration to represent possible states of a connection
 */
enum class ConnectionState {
     Requested,   /**< Initial transitory state entered on WRQ/RRQ */
     Uploading,   /**< TRANSFERING state on reading */
     Downloading, /**< TRANSFERING state on writing */
     Awaiting,    /**< Awaiting ACK */
     Timedout,    /**< Lost packet state; attempts retransmit */
     Denied,      /**< Request not fulfillable; terminal state*/
     Errored,     /**< Mid-transfer error; terminal state */
     Completed    /**< Transfer completed; terminal state */
};

/**
 * @brief Base class for TFTP server connections
 */
class TFTPServerConnection {
   public:
     /**
      * @brief Constructs a new TFTPServerConnection object
      * @param srv_fd Client socket file descriptor
      * @param clt_addr Client address
      * @param req_packet Request packet (RRQ / WRQ)
      * @param root_dir Server root directory
      */
     TFTPServerConnection(int srv_fd, const sockaddr_in& clt_addr,
                          const RequestPacket& req_packet,
                          const std::string& root_dir);

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
      * @brief Handles a read request.
      */
     void handle_rrq();

     /**
      * @brief Handles a write request
      */
     void handle_wrq();

     /**
      * @brief Handles upload of a data packet
      */
     void handle_upload();

     /**
      * @brief Handles awaiting state (wait for ACK)
      */
     void handle_await();

     /* === Getters, setters and checkers === */

     /**
      * @brief Checks if the connection is running
      * @return true if running
      * @return false otherwise
      */
     bool is_running() const {
          return this->state != ConnectionState::Completed
                 && this->state != ConnectionState::Denied
                 && this->state != ConnectionState::Errored;
     }

     /**
      * @brief Checks if the connection is transfering
      * @return true if transfering
      * @return false otherwise
      */
     bool is_transfering() const {
          return this->state == ConnectionState::Uploading
                 || this->state == ConnectionState::Downloading;
     }

     /**
      * @brief Checks if the connection is an upload (read from server)
      * @return true if upload
      * @return false otherwise
      */
     bool is_upload() const { return this->type == TFTPRequestType::Read; }

     /**
      * @brief Checks if the connection is a download (write to server)
      * @return true if download
      * @return false otherwise
      */
     bool is_download() const { return this->type == TFTPRequestType::Write; }

     /* === Helper Methods === */

     /**
      * @brief Sends an error packet and sets
      * the connection state to Errored
      */
     void send_error(TFTPErrorCode code, const std::string& msg);

     /**
      * @brief Logs a connection INFO message
      * to standard output.
      */
     void log_info(const std::string& msg) const {
          std::cout << "  [" << this->tid << "] - INFO  - " << msg << std::endl;
     };

     /**
      * @brief Logs a connection ERROR message
      * to standard output.
      */
     void log_error(const std::string& msg) const {
          std::cout << "  [" << this->tid << "] - ERROR - " << msg << std::endl;
     };

   protected:
     int tid = -1;                    /**< Transfer identifier */
     int srv_fd;                      /**< Server socket file descriptor */
     int conn_fd = -1;                /**< Connection socket file descriptor */
     int file_fd = -1;                /**< File descriptor of the file */
     int block_n = 0;                 /**< Current block number */
     int send_tries = 0;              /**< Number of retransmi attempts */
     std::atomic<bool> is_last;       /**< Flag for last packet */
     struct sockaddr_in clt_addr {};  /**< Address of the client */
     struct sockaddr_in conn_addr {}; /**< Address of the connection */
     std::string file_path;           /**< Path to the file */
     TFTPRequestType type;            /**< Request type (RRQ / WRQ) */
     TFTPDataFormat format;           /**< Transfer format */
     ConnectionState state;           /**< State of the connection */
     std::chrono::steady_clock::time_point
         last_packet_time; /**< Time of last packet */
     std::array<char, TFTP_MAX_PACKET> buffer{
         0}; /**< Buffer for incoming packets */
     socklen_t clt_addr_len
         = sizeof(clt_addr); /**< Length of the client address */
     socklen_t conn_addr_len
         = sizeof(conn_addr); /**< Length of the connection address */
};

#endif