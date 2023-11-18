/**
 * @file client.hpp
 * @author Filip J. Kramec (xkrame00@vutbr.cz)
 * @brief TFTP client implementation
 * @date 2023-11-17
 */

#pragma once
#ifndef TFTP_CLIENT_HPP
#     define TFTP_CLIENT_HPP
#     include <fcntl.h>
#     include <netdb.h>
#     include <sys/stat.h>

#     include <csignal>
#     include <thread>

#     include "common.hpp"
#     include "util/logger.hpp"

/**
 * @brief Class for TFTP client
 */
class TFTPClient {
   public:
     /**
      * @brief Constructs a new TFTP client object with set hostname,
      *        port and destination path.
      * @param string hostname
      * @param [string] filepath
      * @param string destpath
      * @param [uint16_t] port
      * @return TFTPClient
      */
     explicit TFTPClient(const std::string& hostname, int port,
                         const std::string& destpath,
                         const std::optional<std::string>& filepath);

     /**
      * @brief Deconstructs the TFTP client object
      */
     ~TFTPClient();

     TFTPClient& operator=(TFTPClient&& other) = delete;
     TFTPClient& operator=(const TFTPClient&) = delete;
     TFTPClient(TFTPClient&& other) = delete;
     TFTPClient(const TFTPClient&) = delete;

     /* === Core Methods === */

     /**
      * @brief Starts the TFTP client
      */
     void start();

     /**
      * @brief Sends a download (read) request
      */
     void send_rrq();

     /**
      * @brief Sends a upload (write) request
      */
     void send_wrq();

     /**
      * @brief Handles download awaiting state (wait for DATA)
      */
     void handle_await_download();

     /**
      * @brief Handles upload awaiting state (wait for ACK)
      */
     void handle_await_upload();

     /**
      * @brief Handles download of a data packet
      */
     void handle_download();

     /**
      * @brief Handles upload of a data packet
      */
     void handle_upload();

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
      * @brief Returns the type (direction) of the connection
      * @return TFTPRequestType
      */
     TFTPRequestType get_type() const { return this->type; }

     /**
      * @brief Checks if the connection is an upload (write to server)
      * @return true if upload,
      * @return false otherwise
      */
     bool is_upload() const { return this->type == TFTPRequestType::Write; }

     /**
      * @brief Checks if the connection is a download (read from server)
      * @return true if download,
      * @return false otherwise
      */
     bool is_download() const { return this->type == TFTPRequestType::Read; }

     /**
      * @brief Checks if the sockaddr_in matches the client address
      * @param addr Address to check
      * @return true if matches,
      * @return false otherwise
      */
     bool is_host_addr(const sockaddr_in& addr) const {
          return this->host_addr.sin_family == addr.sin_family
                 && this->host_addr.sin_port == addr.sin_port
                 && this->host_addr.sin_addr.s_addr == addr.sin_addr.s_addr;
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
      * @param addr_overwrite Flag to overwrite remote address (use when remote
      * TID is expected)
      * @return Unique pointer to received packet or nullopt
      */
     std::optional<std::unique_ptr<BasePacket>> recv_packet(bool addr_overwrite
                                                            = false);

   private:
     int tid = -1;                        /**< Transfer identifier */
     int conn_fd = -1;                    /**< Socket file descriptor */
     int file_fd = -1;                    /**< Download file descriptor */
     int block_n = 0;                     /**< Current block number */
     int send_tries = 0;                  /**< Number of retransmi attempts */
     struct hostent* host;                /**< Host entity */
     std::string hostname;                /**< Hostname to connect to */
     int port = TFTP_PORT;                /**< Port to connect to */
     TFTPRequestType type;                /**< Request type (RRQ / WRQ) */
     std::string destpath;                /**< Destination path */
     std::optional<std::string> filepath; /**< Filepath to download */
     struct sockaddr_in conn_addr {};     /**< Address of this client */
     struct sockaddr_in host_addr {};     /**< Address of the host */
     bool is_last = false;                /**< Flag for last packet */
     bool last_data_cr = false; /**< Flag if last DATA ended with CR */
     bool file_created = false; /**< Flag if file was created */
     TFTPDataFormat format = TFTPDataFormat::Octet; /**< Transfer format */
     TFTPConnectionState state
         = TFTPConnectionState::Idle; /**< State of the connection */
     std::chrono::steady_clock::time_point
         last_packet_time; /**< Time of last packet */
     std::array<char, TFTP_MAX_PACKET> rx_buffer{
         0};                        /**< Buffer for incoming packets */
     std::vector<char> file_buffer; /**< Buffer for file upload */
     ssize_t rx_len = 0;            /**< Length of the incoming packet */
     ssize_t file_rx_len = 0;       /**< Length of the last stdin read */
     socklen_t conn_addr_len
         = sizeof(conn_addr); /**< Length of the connection address */
     socklen_t host_addr_len
         = sizeof(host_addr); /**< Length of the host address */

     /**
      * @brief Handling loop
      */
     void conn_exec();
};

#endif