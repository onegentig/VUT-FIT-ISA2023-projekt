/**
 * @file connection.hpp
 * @author Onegen Something (xkrame00@vutbr,cz)
 * @brief TFTP connection implementation.
 * @date 2023-11-08
 */

#pragma once
#ifndef TFTP_CONNECTION_HPP
#     define TFTP_CONNECTION_HPP

#     include <sys/stat.h>

#     include <thread>

#     include "common.hpp"
#     include "packet/PacketFactory.hpp"
#     include "util/logger.hpp"

/**
 * @brief Enumeration for all possible states of a connection
 */
enum class ConnectionState {
     Requested,   /**< Initial transitory state entered on WRQ/RRQ */
     Uploading,   /**< TRANSFERING state on reading */
     Downloading, /**< TRANSFERING state on writing */
     Awaiting,    /**< Awaiting ACK */
     Timedout,    /**< Lost packet state; attempts retransmit */
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
      * @param shutd_flag Shared shutdown flag
      */
     TFTPServerConnection(int srv_fd, const sockaddr_in& clt_addr,
                          const RequestPacket& req_packet,
                          const std::string& root_dir,
                          const std::shared_ptr<std::atomic<bool>>& shutd_flag);

     /**
      * @brief Destroys the connection
      */
     ~TFTPServerConnection();

     TFTPServerConnection& operator=(TFTPServerConnection&& other) = delete;
     TFTPServerConnection& operator=(const TFTPServerConnection&) = delete;
     TFTPServerConnection(TFTPServerConnection&& other) = delete;
     TFTPServerConnection(const TFTPServerConnection&) = delete;

     /* === Core Methods === */

     /**
      * @brief Runs the connection
      */
     void run();

     /**
      * @brief Handles a read request
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
      * @brief Handles download of a data packet
      */
     void handle_download();

     /**
      * @brief Handles upload awaiting state (wait for ACK)
      */
     void handle_await_upload();

     /**
      * @brief Handles download awaiting state (wait for DATA)
      */
     void handle_await_download();

     /* === Getters, setters and checkers === */

     /**
      * @brief Gets the block_n string in hexadecimal uppercase
      */
     std::string get_block_n_hex() const {
          std::stringstream sstream;
          sstream << std::hex << std::uppercase << this->block_n;
          return sstream.str();
     }

     /**
      * @brief Checks if the connection is running
      * @return true if running,
      * @return false otherwise
      */
     bool is_running() const {
          return this->state != TFTPConnectionState::Completed
                 && this->state != TFTPConnectionState::Errored;
     }

     /**
      * @brief Checks if the connection is transfering
      * @return true if transfering,
      * @return false otherwise
      */
     bool is_transfering() const {
          return this->state == TFTPConnectionState::Uploading
                 || this->state == TFTPConnectionState::Downloading;
     }

     /**
      * @brief Checks if the connection is an upload (read from server)
      * @return true if upload,
      * @return false otherwise
      */
     bool is_upload() const { return this->type == TFTPRequestType::Read; }

     /**
      * @brief Checks if the connection is a download (write to server)
      * @return true if download,
      * @return false otherwise
      */
     bool is_download() const { return this->type == TFTPRequestType::Write; }

     /**
      * @brief Checks if the sockaddr_in matches the client address
      * @param addr Address to check
      * @return true if matches,
      * @return false otherwise
      */
     bool is_client_addr(const sockaddr_in& addr) const {
          return this->clt_addr.sin_family == addr.sin_family
                 && this->clt_addr.sin_port == addr.sin_port
                 && this->clt_addr.sin_addr.s_addr == addr.sin_addr.s_addr;
     }

     /**
      * @brief Checks timeout of the connection
      * @return true if timeout,
      * @return false otherwise
      */
     bool is_timedout() {
          /** @see https://en.cppreference.com/w/cpp/chrono#Example */
          auto now = std::chrono::steady_clock::now();
          auto diff = std::chrono::duration_cast<std::chrono::seconds>(
                          now - this->last_packet_time)
                          .count();
          return diff > TFTP_PACKET_TIMEO;
     }

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
          Logger::conn_info(std::to_string(this->tid), msg);
     };

     /**
      * @brief Logs a connection ERROR message
      * to standard output.
      */
     void log_error(const std::string& msg) const {
          Logger::conn_err(std::to_string(this->tid), msg);
     };

     /**
      * @brief Receives, logs and checks origin of a TFTP packet
      * @return Unique pointer to received packet or nullopt
      */
     std::optional<std::unique_ptr<BasePacket>> recv_packet();

   private:
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
     TFTPConnectionState state
         = TFTPConnectionState::Idle; /**< State of the connection */
     std::atomic<bool>& shutd_flag;   /**< Flag to signal shutdown */
     bool last_data_cr = false;       /**< Flag if last DATA ended with CR */
     std::chrono::steady_clock::time_point
         last_packet_time; /**< Time of last packet */
     std::array<char, TFTP_MAX_PACKET> rx_buffer{
         0};             /**< Buffer for incoming packets */
     ssize_t rx_len = 0; /**< Length of the incoming packet */
     socklen_t clt_addr_len
         = sizeof(clt_addr); /**< Length of the client address */
     socklen_t conn_addr_len
         = sizeof(conn_addr); /**< Length of the connection address */
};

#endif