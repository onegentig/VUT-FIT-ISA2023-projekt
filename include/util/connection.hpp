/**
 * @file connection.hpp
 * @author Filip J. Kramec <xkrame00@vutbr.cz>
 * @brief Abstract base class for TFTP connection handling
 * @date 2023-11-18
 */

#ifndef TFTP_CONNECTION_BASE_HPP
#define TFTP_CONNECTION_BASE_HPP

#include <sys/stat.h>

#include <thread>

#include "common.hpp"
#include "packet/PacketFactory.hpp"
#include "util/logger.hpp"

/**
 * @brief Abstract base class for TFTP connection handling
 */
class TFTPConnectionBase {
   public:
     /* === Constructors === */

     /**
      * @brief Constructs a new TFTP connection
      */
     TFTPConnectionBase() = default;

     /**
      * @brief Deconstructs the TFTP client object
      */
     ~TFTPConnectionBase();

     TFTPConnectionBase& operator=(TFTPConnectionBase&& other) = delete;
     TFTPConnectionBase& operator=(const TFTPConnectionBase&) = delete;
     TFTPConnectionBase(TFTPConnectionBase&& other) = delete;
     TFTPConnectionBase(const TFTPConnectionBase&) = delete;

     /* === Public methods === */

     /**
      * @brief Starts the TFTP connection, creating a socket
      *        for this connection and starting the main loop.
      */
     void run();

     /* === Public getters, setters and checkers === */

     /**
      * @brief Gets the block number as a hexadecimal string
      * @return std::string hex block_n
      */
     std::string get_block_n_hex() const {
          std::stringstream sstream;
          sstream << std::hex << std::uppercase << this->block_n;
          return sstream.str();
     }

     /**
      * @brief Makes remote address static (does not rewrite on first packet)
      */
     void set_addr_static() { this->addr_static = true; }

     /**
      * @brief Unsets remote address static flag (rewrites on first packet)
      */
     void unset_addr_static() { this->addr_static = false; }

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
      * @brief Checks if the connection has errored
      * @return true if errored,
      * @return false otherwise
      */
     bool is_errored() const {
          return this->state == TFTPConnectionState::Errored;
     }

     /**
      * @brief Checks if the connection is an upload (write to server)
      * @return true if upload,
      * @return false otherwise
      */
     virtual bool is_upload() const {
          return this->get_type() == TFTPRequestType::Write;
     }

     /**
      * @brief Checks if the connection is a download (read from server)
      * @return true if download,
      * @return false otherwise
      */
     virtual bool is_download() const {
          return this->get_type() == TFTPRequestType::Read;
     }

   protected:
     /* === Core private methods === */

     /**
      * @brief Handles the connection main loop
      */
     void exec();

     /**
      * @brief Handles an incoming or outgoing upload request
      * @note Made virtual due to stark differences between client-side
      *       RRQ sending and server-side RRQ/WRQ handling.
      */
     virtual void handle_request_upload() = 0;

     /**
      * @brief Handles an incoming or outgoing download request
      * @note Made virtual due to stark differences between client-side
      *       RRQ sending and server-side RRQ/WRQ handling.
      */
     virtual void handle_request_download() = 0;

     /**
      * @brief Handles upload of a DATA packet
      */
     void handle_upload();

     /**
      * @brief Handles download of a DATA packet
      *        ands its ACKnowledgement
      */
     void handle_download();

     /**
      * @brief Handles waiting for an ACK packet
      *       (upload awaiting state)
      */
     void handle_await_upload();

     /**
      * @brief Handles waiting for an DATA packet
      *       (download awaiting state)
      */
     void handle_await_download();

     /* === Utility methods === */

     /**
      * @brief Logs an error, sends a ERROR packet to the remote host
      *        (does not await ACK) and sets state to Errored
      * @param code TFTP error code
      * @param msg Error message
      */
     void send_error(TFTPErrorCode code, const std::string& msg);

     /**
      * @brief Receives a packet from the remote host with
      *        packet parsing. On error, method calls
      *        `send_error`. If packet does not match the
      *        stored `rem_addr`, it is ignored.
      * @param addr_overwrite Overwrite stored remote addr?
      *                       Use when remote TID is not decided.
      * @return unique_ptr<BasePacket>> Received packet,
      * @return nullptr if no packet (or invalid) received
      */
     std::optional<std::unique_ptr<BasePacket>> recv_packet(bool addr_overwrite
                                                            = false);

     /**
      * @brief Change `state` to a new state, pushing the old
      *        state to `pstate`.
      * @param new_state New state to set
      * @return TFTPConnectionState – Previous state
      */
     TFTPConnectionState set_state(TFTPConnectionState new_state) {
          this->pstate = this->state;
          this->state = new_state;
          return this->pstate;
     }

     /**
      * @brief Stores current time in `last_packet_time`
      */
     void update_sent_time() {
          this->last_packet_time = std::chrono::steady_clock::now();
     }

     /**
      * @brief Logs a connection INFO message to the standard output
      */
     void log_info(const std::string& msg) const {
          Logger::conn_info(std::to_string(this->tid), msg);
     };

     /**
      * @brief Logs a connection ERROR message to the standard output
      */
     void log_error(const std::string& msg) const {
          Logger::conn_err(std::to_string(this->tid), msg);
     };

     /* == Internal getters, setters and checkers == */

     /**
      * @brief Checks if the sockaddr_in matches the remote address
      * @note Usable to check TID match.
      * @param addr Address to check
      * @return true if matches,
      * @return false otherwise
      */
     bool is_remote_addr(const sockaddr_in& addr) const {
          return this->rem_addr.sin_family == addr.sin_family
                 && this->rem_addr.sin_port == addr.sin_port
                 && this->rem_addr.sin_addr.s_addr == addr.sin_addr.s_addr;
     }

     /**
      * @brief Checks if the connection timeout has been reached
      * @return true if did timeout,
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

     /**
      * @brief Returns the type (direction) of the connection
      * @return TFTPRequestType
      */
     TFTPRequestType get_type() const { return this->type; }

     /* == Internal virtuals == */

     /**
      * @brief Checks the shutdown flag and returns whether
      *        the connection should terminate (checked on
      *        every `run` iteration.)
      * @return true if connection should terminate
      * @return false otherwise
      */
     virtual bool should_shutd() = 0;

     /**
      * @brief Obtains the next DataPacket payload to be sent
      * @return std::vector<char> Serialised DataPacket payload
      */
     virtual std::vector<char> next_data() = 0;

     /* === Variables === */

     /* == File descriptors ==*/
     int tid = -1;     /**< Transfer ID (local) */
     int conn_fd = -1; /**< Connection socket file descriptor */
     int file_fd = -1; /**< File socket file descriptor */

     /* == Counters == */
     int block_n = 0;    /**< Number of the currently transferred block */
     int send_tries = 0; /**< Number of packet retransmission attempts */

     /* == Flags == */
     bool is_last = false;      /**< Flag for last packet */
     bool cr_end = false;       /**< Flad if last DATA ended with CR */
     bool file_created = false; /**< Flag if the file `filename` was created */
     bool addr_static
         = false; /**< Flag if rem_addr should never be ovearwritten */

     /* == State-tracking enums == */
     TFTPConnectionState state
         = TFTPConnectionState::Idle; /**< Connection state */
     TFTPConnectionState pstate
         = TFTPConnectionState::Idle; /**< Previous connection state */
     TFTPRequestType type;            /**< Request type (download or upload?) */
     TFTPDataFormat format = TFTPDataFormat::Octet; /**< Transfer format */

     /* == Adresses == */
     struct sockaddr_in con_addr {}; /**< Address of this client */
     struct sockaddr_in rem_addr {}; /**< Address of the remote host */
     socklen_t con_addr_len
         = sizeof(con_addr); /**< Length of the connection address */
     socklen_t rem_addr_len
         = sizeof(rem_addr); /**< Length of the remote address */

     /* == Buffers == */
     std::array<char, TFTP_MAX_PACKET> rx_buffer{
         0};             /**< Buffer for incoming packets */
     ssize_t rx_len = 0; /**< Length of the incoming packet */

     /* == Other == */
     std::string file_name = ""; /**< Name of downloaded/uploaded file */
     std::chrono::steady_clock::time_point
         last_packet_time; /**< Time of last packet */
};

#endif