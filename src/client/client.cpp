/**
 * @file client.cpp
 * @author Filip J. Kramec (xkrame00@vutbr.cz)
 * @brief TFTP client implementation
 * @date 2023-11-17
 */

#include "client/client.hpp"

/* === Signal handler for graceful shutdown === */

/**
 * @brief SIGINT flag
 * @details Atomic flag indicating whether SIGINT was recieved,
 * used to gracefully terminate the client (and send ERROR).
 */
std::atomic<bool> quit(false);

/**
 * @brief Sets the quit flag to true on SIGINT.
 * @param signal - signal number
 */
void signal_handler(int signal) {
     (void)signal;
     quit.store(true);
}

/* === Setup methods === */

TFTPClient::TFTPClient(const std::string &hostname, int port,
                       const std::string &destpath,
                       const std::optional<std::string> &filepath)
    : hostname(hostname), port(port), destpath(destpath), filepath(filepath) {
     this->unset_addr_static();

     /* Set some placeholder opts for testing */
     // TODO: DO NOT FORGET TO REMOVE THIS AFTER TESTING!!!!
     this->opts = {{"blksize", "1024"}, {"timeout", "1"}};

     /* Verify hostname */
     if (hostname.empty()) throw std::runtime_error("Invalid hostname");

     /* Verify port number */
     if (port < 1 || port > 65535)
          throw std::runtime_error("Invalid port number");

     /* Verify destination path */
     if (destpath.empty()) throw std::runtime_error("Invalid destination path");

     /* Verify filepath */
     if (filepath.has_value()) {
          if (filepath.value().empty())
               throw std::runtime_error("Invalid filepath");

          /* Check if destination file exists */
          if (access(destpath.c_str(), F_OK) != -1)
               throw std::runtime_error("File " + destpath + " already exists");

          this->type = TFTPRequestType::Read;
          this->file_name = destpath;  // `file_name` is name of downloaded file
     } else {
          this->type = TFTPRequestType::Write;
     }

     /* Resolve hostname and get host */
     /** @see
      * https://moodle.vut.cz/pluginfile.php/550189/mod_folder/content/0/IPK2022-23L-03-PROGRAMOVANI.pdf#page=18
      */
     this->host = gethostbyname(this->hostname.c_str());
     if (this->host == NULL)
          throw std::runtime_error("Host not found: " + this->hostname);

     memset(reinterpret_cast<char *>(&(this->rem_addr)), 0, this->rem_addr_len);
     this->rem_addr.sin_family = AF_INET;
     this->rem_addr.sin_port = htons(this->port);
     memcpy(this->host->h_addr,
            reinterpret_cast<char *>(&(this->rem_addr).sin_addr.s_addr),
            this->host->h_length);

     /* Convert hostname to IP address */
     /** @see https://beej.us/guide/bgnet/pdf/bgnet_a4_c_1.pdf#page=82 */
     int ip_vld = inet_pton(AF_INET, this->hostname.c_str(),
                            &(this->rem_addr.sin_addr));
     if (ip_vld == 0) throw std::runtime_error("Hostname IP is not valid");
     if (ip_vld < 0)
          throw std::runtime_error("Failed to convert IP address: "
                                   + std::string(strerror(errno)));

     /* Set input buffer size */
     file_buffer.resize(TFTP_MAX_DATA);

     /* Set up signal handler */
     /** @see https://gist.github.com/aspyct/3462238 */
     struct sigaction sig_act {};
     memset(&sig_act, 0, sizeof(sig_act));
     sig_act.sa_handler = signal_handler;
     sigfillset(&sig_act.sa_mask);
     sigaction(SIGINT, &sig_act, NULL);
}

/* === Virtuals === */

/* == Handlers == */

/**
 * @brief Sends a WRQ to the server
 */
void TFTPClient::handle_request_upload() {
     log_info("Requesting write to file " + this->destpath);

     /* Create request payload */
     RequestPacket packet
         = RequestPacket(TFTPRequestType::Write, this->destpath, this->format);
     packet.set_options(this->opts);
     auto payload = packet.to_binary();

     /* Send request */
     this->update_sent_time();
     sendto(this->conn_fd, payload.data(), payload.size(), 0,
            reinterpret_cast<const sockaddr *>(&this->rem_addr),
            sizeof(this->rem_addr));

     /* If options were set, allow OACK */
     this->oack_expect = packet.get_options_count() > 0;

     /* Await ACK or OACK */
     this->set_state(TFTPConnectionState::Awaiting);
}

/**
 * @brief Sends a RRQ to the server
 */
void TFTPClient::handle_request_download() {
     log_info("Requesting read from file " + this->filepath.value());

     /* Create a part file, if not created already */
     if (!this->file_created) {
          this->file_fd
              = open(this->destpath.c_str(), O_WRONLY | O_CREAT | O_TRUNC,
                     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

          /* Check if file was created properly */
          if (this->file_fd < 0)
               return send_error(TFTPErrorCode::AccessViolation,
                                 "Failed to create file");
          this->file_created = true;
     }

     /* Create request payload */
     RequestPacket packet = RequestPacket(TFTPRequestType::Read,
                                          this->filepath.value(), this->format);
     packet.set_options(this->opts);
     auto payload = packet.to_binary();

     /* Send request */
     this->update_sent_time();
     sendto(this->conn_fd, payload.data(), payload.size(), 0,
            reinterpret_cast<const sockaddr *>(&this->rem_addr),
            sizeof(this->rem_addr));

     /* If options were set, allow OACK */
     this->oack_expect = packet.get_options_count() > 0;

     /* Await DATA or OACK */
     this->set_state(TFTPConnectionState::Awaiting);
}

/**
 * @details `handle_oack` is called when OACK packet is received and
 *          `oack_expect` is set to true. It parses options from the
 *          OACK and sets them to the connection.
 */
void TFTPClient::handle_oack(const OptionAckPacket &oack) {
     auto new_opts = oack.get_options();

     /* Process and set options */
     auto acc_opts = this->proc_opts(new_opts);

     // TODO: When should error 8 be sent?

     this->log_info(
         "Options accepted (count: " + std::to_string(acc_opts.size()) + ")");

     return;
}

/**
 * @brief Checks if the client should shut down
 * @return true if should shut down,
 * @return false otherwise
 */
bool TFTPClient::should_shutd() { return quit.load(); }

/**
 * @brief Obtains the next DataPacket payload to be sent
 * @return std::vector<char> Serialised DataPacket payload
 */
std::vector<char> TFTPClient::next_data() {
     /* Load chunk from stdin */
     std::fill(file_buffer.begin(), file_buffer.end(), 0);
     std::cin.read(file_buffer.data(), TFTP_MAX_DATA);
     this->file_rx_len = std::cin.gcount();

     /* Create data payload */
     DataPacket packet
         = DataPacket(std::vector<char>(file_buffer.begin(),
                                        file_buffer.begin() + file_rx_len),
                      this->block_n);
     packet.set_no_seek(true);
     packet.set_mode(this->format);
     return packet.to_binary();
}