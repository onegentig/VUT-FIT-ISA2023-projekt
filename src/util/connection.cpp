/**
 * @file connection.hpp
 * @author Filip J. Kramec <xkrame00@vutbr.cz>
 * @brief Abstract base class for TFTP connection handling
 * @date 2023-11-18
 */

#include "util/connection.hpp"

/* === Setup methods === */

/**
 * @note As a base class, most specifics should be sorted out by the
 *       derived classes. The constructor widely differed between
 *       `TFTPServerConnection` and `TFTPClient`, so it was not
 *       feasable to just cram it into the base class.
 */

/**
 * @details The destructor closes all file destriptors and logs its
 *          destruction via the `Logger` util class. If the connection
 *          was downloading a file, but it ended prematurely, destructor
 *          will remove this incomplete file.
 * @see https://moodle.vut.cz/mod/forum/discuss.php?d=2929#p4633
 */
TFTPConnectionBase::~TFTPConnectionBase() {
     /* Close the socket */
     if (this->conn_fd != -1) {
          close(this->conn_fd);
          this->conn_fd = -1;
     }

     /* Close the file */
     if (this->file_fd != -1) {
          close(this->file_fd);
          this->file_fd = -1;
     }

     /* Remove incomplete downloaded file */
     if (this->is_errored() && file_created) {
          if (access(this->file_name.c_str(), F_OK) != -1) {
               remove(this->file_name.c_str());
          }
     }

     if (this->tid > 0)
          Logger::glob_event("Closed connection [" + std::to_string(this->tid)
                             + "]");
}

/**
 * @details Simple socket initialising method.
 */
void TFTPConnectionBase::sock_init() {
     /* Create socket */
     this->conn_fd = socket(AF_INET, SOCK_DGRAM, 0);
     if (this->conn_fd == -1)
          throw std::runtime_error("Failed to create socket");

     Logger::glob_info("socket created with FD "
                       + std::to_string(this->conn_fd));

     /* Set address */
     memset(&(this->con_addr), 0, this->con_addr_len);
     this->con_addr.sin_family = AF_INET;
     this->con_addr.sin_port = htons(0);  // => random port
     this->con_addr.sin_addr.s_addr = htonl(INADDR_ANY);

     /* Set timeout */
     struct timeval timeout {
          TFTP_TIMEO, 0
     };

     if (setsockopt(this->conn_fd, SOL_SOCKET, SO_RCVTIMEO,
                    reinterpret_cast<char *>(&timeout), sizeof(timeout))
             < 0
         || setsockopt(this->conn_fd, SOL_SOCKET, SO_SNDTIMEO,
                       reinterpret_cast<char *>(&timeout), sizeof(timeout))
                < 0)
          throw std::runtime_error("Failed to set socket timeout");

     /* Allow address:port reuse */
     int optval = 1;
     if (setsockopt(this->conn_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                    &optval, sizeof(optval))
         < 0)
          throw std::runtime_error("Failed to set socket options");

     /* Bind socket */
     if (bind(this->conn_fd,
              reinterpret_cast<struct sockaddr *>(&this->con_addr),
              this->con_addr_len)
         < 0)
          throw std::runtime_error("Failed to bind socket : "
                                   + std::string(strerror(errno)));
     /* Get OS-assigned random port -> TID */
     if (getsockname(this->conn_fd,
                     reinterpret_cast<struct sockaddr *>(&this->con_addr),
                     &this->con_addr_len)
         < 0)
          throw std::runtime_error("Failed to get socket name : "
                                   + std::string(strerror(errno)));
     this->tid = ntohs(this->con_addr.sin_port);

     Logger::glob_info("socket bound to "
                       + std::string(inet_ntoa(this->con_addr.sin_addr)) + ":"
                       + std::to_string(ntohs(this->con_addr.sin_port)));
     this->set_state(TFTPConnectionState::Requesting);
}

/**
 * @details The `run` method is the main method of the connection.
 *          It will create a socket and set up the connection
 *          (incl. TID, binding and whatnot), after which it will
 *          call the internal `exec()` method that handles state
 *          logic. As such, `run()` is a blocking method.
 * @throws std::runtime_error if the socket creation screws up
 * @see
 * https://moodle.vut.cz/pluginfile.php/550189/mod_folder/content/0/IPK2022-23L-03-PROGRAMOVANI.pdf#page=16
 */
void TFTPConnectionBase::run() {
     Logger::glob_op("Starting connection...");

     /* Init socket */
     this->sock_init();

     /* Start connection */
     this->exec();  // ! Blocking until connection is done or errs
}

/* === Connection flow === */

/**
 * @details The `exec` method is the handling loop of the connection.
 *          It will call handler function for each state, which will
 *          handle the state logic and return the next state. The
 *          loop will run until the connection is done or errored.
 * @note Derived classes are expected to set up a virtual method
 *       `should_shutd()` using which they can terminate the
 *       connection (server sets it to a shared pointer atomic flag
 *       using which the server can terminate all its connections).
 * @note The `exec_unblock` flag was added later to allow `poll()`
 *       server handling (it was originally multi-threaded). The flag
 *       is set when conn transitions to the `Awaiting` state, which
 *       meants it, well, is awaitng a packet – so the server can
 *       `poll()` the connection and unblock it when a packet arrives.
 */
void TFTPConnectionBase::exec() {
     while (this->is_running()) {
          /* Check unblock flag */
          if (this->exec_unblock) {
               this->exec_unblock = false;
               break;
          }

          /* Check shutdown flag */
          if (this->should_shutd()) {
               log_info("Shutdown flag detected, stopping...");
               this->send_error(TFTPErrorCode::Unknown, "Terminated by user");
               break;
          }

          /* Handle state */
          switch (this->state) {
               case TFTPConnectionState::Requesting:
                    this->is_upload() ? this->handle_request_upload()
                                      : this->handle_request_download();
                    break;
               case TFTPConnectionState::Awaiting:
                    this->is_upload() ? this->handle_await_upload()
                                      : this->handle_await_download();
                    break;
               case TFTPConnectionState::Uploading:
                    this->handle_upload();
                    break;
               case TFTPConnectionState::Downloading:
                    this->handle_download();
                    break;
               default:
                    log_error("`run` called in invalid state");
                    return send_error(TFTPErrorCode::Unknown,
                                      "Bad internal state");
          }

          /* Short sleep (to prevent CPU-hogging) */
          std::this_thread::sleep_for(
              std::chrono::microseconds(TFTP_THREAD_DELAY));
     }
}

/**
 * @details `proc_opts` (process options) is a method that adjusts internal
 *          variables to suit given TFTP options, as defined by
 *          TFC 2347. On the server side, this method is called when
 *          RRQ/WRQ is received; on the client side when OACK is received
 *          (when we can be sure server is OK with out options).
 *          Method returns a vector of SUCCESSFULLY processed options
 *          (so that server can send back an OACK right away).
 * @note Currently the RFC 2347 is implemented, but no extending RFCs
 *       that uitilise options are. This method is essentally a stub.
 */
std::vector<std::pair<std::string, std::string>> TFTPConnectionBase::proc_opts(
    const std::vector<std::pair<std::string, std::string>> &new_opts) {
     std::vector<std::pair<std::string, std::string>>
         acc_opts; /**< Accepted opts */

     /* Iterate through options */
     for (const auto &opt : new_opts) {
          std::string opt_name = opt.first;
          std::transform(opt_name.begin(), opt_name.end(), opt_name.begin(),
                         static_cast<int (*)(int)>(std::tolower));

          /*if (opt_name == "blksize") {
               acc_opts.push_back(opt);
          } else {*/
               Logger::glob_info("ignoring unknown option '" + opt_name + "'");
          //}
     }

     return acc_opts;
}

/* == Uploading handlers == */

/**
 * @details The `handle_upload` method sends a DATA packet to the
 *          remote host. It is expected from derived classes to
 *          implement the `next_data` method to create the
 *          `DataPacket` to send (return the `.to_binary`) for
 *          flexibility.
 */
void TFTPConnectionBase::handle_upload() {
     /* OACK response */
     if (this->block_n == 0 && this->oack_init) {
          log_info("Sending OACK");

          OptionAckPacket oack = OptionAckPacket(this->opts);
          auto payload = oack.to_binary();

          this->update_sent_time();
          sendto(this->conn_fd, payload.data(), payload.size(), 0,
                 reinterpret_cast<const sockaddr *>(&this->rem_addr),
                 sizeof(this->rem_addr));

          this->set_state(TFTPConnectionState::Awaiting);
          return;
     }

     /* No OACK => no ACK 0 */
     if (this->block_n == 0) this->block_n = 1;

     /* Store sent time (for timeout checking) */
     this->update_sent_time();

     /* Get data to send */
     std::vector<char> payload = this->next_data();

     /* Remember if this packet will be the last */
     this->is_last = (payload.size() < TFTP_MAX_DATA + 4);

     log_info("Sending DATA block " + this->get_block_n_hex() + " ("
              + std::to_string(payload.size() - 4) + " bytes)");

     /* Send data */
     this->update_sent_time();
     sendto(this->conn_fd, payload.data(), payload.size(), 0,
            reinterpret_cast<const sockaddr *>(&this->rem_addr),
            sizeof(this->rem_addr));

     /* Await acknowledgement */
     log_info("Awaiting ACK for block " + this->get_block_n_hex());
     this->set_state(TFTPConnectionState::Awaiting);
}

/**
 * @details Awaits present block ACK packet from the remote host, incl.
 *          all the associated checks – timeo, packet validity
 *          block_n and whatnot. Transitions to `Uploading`
 *          state on success (block_n++), previous state on
 *          timeo, loops on `WOULDBLOCK`, `Errored` on err
 *          and `Completed` on last ACK.
 * @note After extending with RFC 2347, this handler also accepts
 *       OACKs if `oack_expect` is set to true (flag reset on handle).
 *       On OACK, it calls `handle_oack` "sub-handler". OACKs
 *       recieved when `oack_expect` is false are ignored (strays).
 */
void TFTPConnectionBase::handle_await_upload() {
     /* Timeout check */
     if (this->is_timedout()) {
          /* Check if max. no. of retries was reached */
          if (++this->send_tries > TFTP_MAX_RETRIES)
               return this->send_error(TFTPErrorCode::Unknown,
                                       "Retransmission timeout");

          /* if not, retransmit last packet */
          log_info("Retransmitting block " + this->get_block_n_hex()
                   + " (attempt " + std::to_string(this->send_tries) + ")");
          this->set_state(this->pstate);
          return;
     }

     /* Receive packet */
     auto packet_res = this->recv_packet((this->block_n == 0));
     if (!packet_res.has_value()) return;  // No packet => loop in state
     auto packet_ptr = std::move(packet_res.value());

     /* ERROR handling */
     if (packet_ptr->get_opcode() == TFTPOpcode::ERROR) {
          /* Cast */
          auto *packet = dynamic_cast<ErrorPacket *>(packet_ptr.get());
          auto errmsg = packet->get_message();

          /* Log */
          log_error("Host errored with code "
                    + std::to_string(packet->get_errcode()));
          if (errmsg.has_value()) log_error("'" + errmsg.value() + "'");

          /* Set state */
          this->set_state(TFTPConnectionState::Errored);
          return;
     }

     /* Check if packet type is as expected */
     if (packet_ptr->get_opcode() != TFTPOpcode::ACK
         && packet_ptr->get_opcode() != TFTPOpcode::OACK)
          return send_error(TFTPErrorCode::IllegalOperation,
                            "Received a non-(O)ACK packet");

     /* OACK handling */
     if (packet_ptr->get_opcode() == TFTPOpcode::OACK) {
          /* Ignore if `oack_expect` is unset */
          if (!this->oack_expect)
               return log_info(
                   "Received OACK but oack_expect is not set, ignoring");
          this->oack_expect = false;

          /* Cast and handle options */
          auto *packet = dynamic_cast<OptionAckPacket *>(packet_ptr.get());
          this->handle_oack(*packet);
     } else {
          /* ACK handling */
          auto *packet
              = dynamic_cast<AcknowledgementPacket *>(packet_ptr.get());

          /* Check ACK block number */
          if (packet->get_block_number() < this->block_n) {
               /* Stray old block ACK */
               log_info("Received ACK for block "
                        + std::to_string(packet->get_block_number())
                        + " (stray, ignoring)");
               return;  // As if nothing happened => loop in state
          }

          if (packet->get_block_number() > this->block_n) {
               /* Future block => error */
               return send_error(TFTPErrorCode::IllegalOperation,
                                 "Received ACK for future block");
          }
     }

     // (O)ACK handled, continue
     this->send_tries = 0;

     /* End transmission if this was the final block */
     if (this->is_last) {
          log_info("Upload complete!");
          this->set_state(TFTPConnectionState::Completed);
          return;
     }

     /* Increment block number */
     if (this->block_n++ == TFTP_MAX_FILE_BLOCKS)
          return send_error(TFTPErrorCode::Unknown,
                            "Block overflow (file too big)");

     /* Continue transferring */
     this->set_state(TFTPConnectionState::Uploading);
}

/* == Downloading handlers == */

/**
 * @details The `handle_download` method handles writing the received
 *          DATA packet data to the file and sending an ACK packet
 *          for the block. The packet is read from `rx_buffer`.
 *          Method also includes NetASCII conversion and adjustment
 *          for `[... CR ] | [ LF/NUL ...]` block split.
 */
void TFTPConnectionBase::handle_download() {
     /* OACK response */
     if (this->block_n == 0 && this->oack_init) {
          log_info("Sending OACK");

          OptionAckPacket oack = OptionAckPacket(this->opts);
          auto payload = oack.to_binary();

          this->update_sent_time();
          sendto(this->conn_fd, payload.data(), payload.size(), 0,
                 reinterpret_cast<const sockaddr *>(&this->rem_addr),
                 sizeof(this->rem_addr));

          this->set_state(TFTPConnectionState::Awaiting);
          return;
     }

     AcknowledgementPacket ack = AcknowledgementPacket(this->block_n);
     auto payload = ack.to_binary();
     this->update_sent_time();

     /* No data || block 0 => no writing, just send ACK (init or timeo) */
     if (this->block_n == 0 || this->rx_len <= 0) {
          log_info("Sending ACK for block " + this->get_block_n_hex());

          this->update_sent_time();
          sendto(this->conn_fd, payload.data(), payload.size(), 0,
                 reinterpret_cast<const sockaddr *>(&this->rem_addr),
                 sizeof(this->rem_addr));

          this->set_state(TFTPConnectionState::Awaiting);
          return;
     }

     /* Parse packet from buffer */
     DataPacket packet;
     std::vector<char> data;
     try {
          packet = DataPacket::from_binary(std::vector<char>(
              this->rx_buffer.begin(), this->rx_buffer.begin() + this->rx_len));
          data = packet.get_data();
     } catch (const std::exception &e) {
          return send_error(TFTPErrorCode::IllegalOperation,
                            "Failed to parse DATA packet");
     }

     /* Convert to NetASCII if needed */
     if (this->format == TFTPDataFormat::NetASCII && !data.empty()) {
          /* Adjustment for [... CR] | [LF/NUL ...] block split*/
          if (this->cr_end && data[0] == '\n') {
               /* CR | LF -> LF */
               off_t file_size = lseek(this->file_fd, 0, SEEK_END);
               if (file_size > 0 && ftruncate(this->file_fd, file_size - 1))
                    return this->send_error(TFTPErrorCode::AccessViolation,
                                            "Failed to truncate file on CR");
               lseek(this->file_fd, 0, SEEK_END);
          } else if (this->cr_end && data[0] == '\0') {
               /* CR | NUL -> CR */
               data.erase(data.begin());
          }

          data = NetASCII::na_to_vec(data);
     }

     this->cr_end = (!data.empty() && data.back() == '\r');
     log_info("Received block " + this->get_block_n_hex() + " ("
              + std::to_string(data.size()) + " bytes)");

     /* Write to file */
     if (write(this->file_fd, data.data(), data.size()) < 0)
          return this->send_error(TFTPErrorCode::AccessViolation,
                                  "Failed to write to file");

     /* Clear buffer */
     this->rx_buffer.fill(0);
     this->rx_len = 0;

     /* Send ACK */
     log_info("Sending ACK for block " + this->get_block_n_hex());
     this->update_sent_time();
     sendto(this->conn_fd, payload.data(), payload.size(), 0,
            reinterpret_cast<const sockaddr *>(&this->rem_addr),
            sizeof(this->rem_addr));

     /* End transmission if this was the final block */
     if (packet.get_data().size() < TFTP_MAX_DATA) {
          log_info("Download complete!");
          this->set_state(TFTPConnectionState::Completed);
          return;
     }

     /* Await next data block */
     this->set_state(TFTPConnectionState::Awaiting);
}

/**
 * @details Awaits present block DATA packet from the remote host, incl.
 *          all the associated checks – timeo, packet validity
 *          block_n and whatnot. Transitions to `Downloading`
 *          state on success (block_n++), previous state on
 *          timeo, loops on `WOULDBLOCK` and `Errored` on err.
 */
void TFTPConnectionBase::handle_await_download() {
     /* Timeout check */
     if (this->is_timedout()) {
          /* Check if max. no. of retries was reached */
          if (++this->send_tries > TFTP_MAX_RETRIES)
               return this->send_error(TFTPErrorCode::Unknown,
                                       "Retransmission timeout");

          /* if not, retransmit last packet */
          log_info("Retransmitting ACK for block " + this->get_block_n_hex()
                   + " (attempt " + std::to_string(this->send_tries) + ")");
          this->set_state(this->pstate);
          return;
     }

     /* Receive packet */
     auto packet_res = this->recv_packet((this->block_n == 0));
     if (!packet_res.has_value()) return;  // No packet => loop in state
     auto packet_ptr = std::move(packet_res.value());

     /* ERROR handling */
     if (packet_ptr->get_opcode() == TFTPOpcode::ERROR) {
          /* Cast */
          auto *packet = dynamic_cast<ErrorPacket *>(packet_ptr.get());
          auto errmsg = packet->get_message();

          /* Log */
          log_error("Host errored with code "
                    + std::to_string(packet->get_errcode()));
          if (errmsg.has_value()) log_error("'" + errmsg.value() + "'");

          /* Set state */
          this->set_state(TFTPConnectionState::Errored);
          return;
     }

     /* Check if packet type is as expected */
     if (packet_ptr->get_opcode() != TFTPOpcode::DATA
         && packet_ptr->get_opcode() != TFTPOpcode::OACK)
          return send_error(TFTPErrorCode::IllegalOperation,
                            "Received a non-DATA/OACK packet");

     /* OACK handling */
     if (packet_ptr->get_opcode() == TFTPOpcode::OACK) {
          /* Ignore if `oack_expect` is unset */
          if (!this->oack_expect)
               return log_info(
                   "Received OACK but oack_expect is not set, ignoring");
          this->oack_expect = false;

          /* Cast and handle options */
          auto *packet = dynamic_cast<OptionAckPacket *>(packet_ptr.get());
          this->handle_oack(*packet);
     } else {
          /* DATA handling */
          auto *packet = dynamic_cast<DataPacket *>(packet_ptr.get());

          /* Check DATA block number */
          if (packet->get_block_number() < this->block_n + 1) {
               /* Stray old block ACK */
               log_info("Received DATA for block "
                        + std::to_string(packet->get_block_number())
                        + " (stray, ignoring)");
               return;  // As if nothing happened => loop in state
          }

          if (packet->get_block_number() > this->block_n + 1) {
               /* Future block => error */
               return send_error(TFTPErrorCode::IllegalOperation,
                                 "Received ACK for future block");
          }

          /* Increment block number */
          if (this->block_n++ == TFTP_MAX_FILE_BLOCKS - 1)
               return send_error(TFTPErrorCode::Unknown,
                                 "Block overflow (file too big)");
     }

     // DATA/OACK handled, continue
     this->send_tries = 0;

     /* Write to file in `Downloading` state */
     this->set_state(TFTPConnectionState::Downloading);
}

/* === Utility methods === */

/**
 * @details The `recv_packet` method is a wrapper around the `recvfrom`
 *          system call. It will receive a packet from the socket and
 *          parse it into a packet object. If the packet not valid,
 *          it sends ERROR and transitions to `Errored` state.
 *          If no packet was received or error happened, it will return
 *          a nullopt. If the packet IS valid, it will return opt
 *          containing the packet object.
 * @param addr_overwrite If true, the method will overwrite `host_addr`
 *                       with the origin address of the received packet.
 *                       This is used in client connection on first
 *                       packet to store the server-side TID.
 * @return `optional` containing the `BasePacket` object if the
 *         packet was valid,
 * @return `nullopt` otherwise.
 */
std::optional<std::unique_ptr<BasePacket>> TFTPConnectionBase::recv_packet(
    bool addr_overwrite) {
     struct sockaddr_in origin_addr {};
     socklen_t origin_addr_len = sizeof(origin_addr);

     /* Receive packet */
     this->rx_len = recvfrom(
         this->conn_fd, this->rx_buffer.data(), TFTP_MAX_PACKET, 0,
         reinterpret_cast<struct sockaddr *>(&origin_addr), &origin_addr_len);

     /* Handle errors */
     if (rx_len < 0) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
               /* Nothing to receive, loop in state */
               return std::nullopt;
          }

          log_error("Failed to receive packet:");
          send_error(TFTPErrorCode::Unknown, std::string(strerror(errno)));
          return std::nullopt;
     }

     /* Parse incoming packet */
     auto packet_ptr = PacketFactory::create(this->rx_buffer, rx_len);
     if (!packet_ptr) {
          send_error(TFTPErrorCode::IllegalOperation,
                     "Received an invalid packet");
          return std::nullopt;
     }

     Logger::packet(*packet_ptr, origin_addr, this->con_addr);

     /* Overwrite host_addr if applicable,      */
     /* otherwise check TID/origin-client match */
     bool overwrite = !this->addr_static && addr_overwrite;
     if (overwrite) {
          this->rem_addr = origin_addr;
          this->rem_addr_len = origin_addr_len;
     } else if (!this->is_remote_addr(origin_addr)) {
          /* Packet not from established remote host */
          log_info("Received packet from unexpected origin");

          /* Send an ERROR packet to the host */
          ErrorPacket err = ErrorPacket(TFTPErrorCode::UnknownTID,
                                        "Unexpected packet origin");
          auto err_payload = err.to_binary();

          this->update_sent_time();
          sendto(this->conn_fd, err_payload.data(), err_payload.size(), 0,
                 reinterpret_cast<const sockaddr *>(&origin_addr),
                 sizeof(origin_addr));

          /* Clear buffer */
          this->rx_buffer.fill(0);
          this->rx_len = 0;

          /* …as if nothing happened */
          return std::nullopt;
     }

     return packet_ptr;
}

/**
 * @note This will log an error message and send a non-awaited ERROR
 *       packet. Immediatelly afterwards, the connection is set to
 *       terminal `Errored` state, implicitly calling the destructor
 *       and closing the connection.
 */
void TFTPConnectionBase::send_error(TFTPErrorCode code,
                                    const std::string &message) {
     log_error(message);

     ErrorPacket res = ErrorPacket(code, message);
     auto payload = res.to_binary();

     this->update_sent_time();
     sendto(this->conn_fd, payload.data(), payload.size(), 0,
            reinterpret_cast<const sockaddr *>(&this->rem_addr),
            sizeof(this->rem_addr));

     this->set_state(TFTPConnectionState::Errored);
}