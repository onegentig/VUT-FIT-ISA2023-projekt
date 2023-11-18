/**
 * @file server.hpp
 * @author Onegen Something (xkrame00@vutbr.cz)
 * @brief TFTP server implementation.
 * @date 2023-10-21
 */

#include "server/server.hpp"

/**
 * @brief SIGINT flag
 * @details Atomic flag indicating whether SIGINT was recieved,
 * used to gracefully terminate server's connections.
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

/* === Constructors === */

TFTPServer::TFTPServer() : port(TFTP_PORT), rootdir("./") {}

TFTPServer::TFTPServer(std::string rootdir)
    : port(TFTP_PORT), rootdir(std::move(rootdir)) {
     /* Verify root directory */
     if (!this->check_dir()) throw std::runtime_error("Invalid root directory");
}

TFTPServer::TFTPServer(std::string rootdir, int port)
    : port(port), rootdir(std::move(rootdir)) {
     /* Verify port number */
     if (port < 1 || port > 65535)
          throw std::runtime_error("Invalid port number");

     /* Verify root directory */
     if (!this->check_dir()) throw std::runtime_error("Invalid root directory");
}

/* === Server Flow === */

void TFTPServer::start() {
     Logger::glob_op("Starting server...");

     /** @see
      * https://moodle.vut.cz/pluginfile.php/550189/mod_folder/content/0/IPK2022-23L-03-PROGRAMOVANI.pdf#page=21
      */

     /* Create socket */
     this->fd = socket(AF_INET, SOCK_DGRAM, 0);
     if (this->fd == -1) throw std::runtime_error("Failed to create socket");

     Logger::glob_info("socket created with FD " + std::to_string(this->fd));

     /* Set address */
     memset(&(this->addr), 0, this->addr_len);
     this->addr.sin_family = AF_INET;
     this->addr.sin_port = htons(port);
     this->addr.sin_addr.s_addr = htonl(INADDR_ANY);

     /* Set timeout */
     struct timeval timeout {
          TFTP_TIMEO, 0
     };

     if (setsockopt(this->fd, SOL_SOCKET, SO_RCVTIMEO,
                    reinterpret_cast<char*>(&timeout), sizeof(timeout))
         < 0)
          throw std::runtime_error("Failed to set socket timeout");

     /* Allow address:port reuse */
     int optval = 1;
     if (setsockopt(this->fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optval,
                    sizeof(optval))
         < 0)
          throw std::runtime_error("Failed to set socket options");

     /* Bind socket */
     if (bind(this->fd, reinterpret_cast<struct sockaddr*>(&this->addr),
              addr_len)
         < 0)
          throw std::runtime_error("Failed to bind socket : "
                                   + std::string(strerror(errno)));

     Logger::glob_info("socket bound to "
                       + std::string(inet_ntoa(this->addr.sin_addr)) + ":"
                       + std::to_string(ntohs(this->addr.sin_port)));

     /* Make the listening socket non-blocking */
     int flags = fcntl(this->fd, F_GETFL, 0);
     if (flags < 0) throw std::runtime_error("Failed to get socket flags");

     flags |= O_NONBLOCK;
     if (fcntl(this->fd, F_SETFL, flags) < 0)
          throw std::runtime_error("Failed to set socket flags");

     /* Set up signal handler */
     /** @see https://gist.github.com/aspyct/3462238 */
     struct sigaction sig_act {};
     memset(&sig_act, 0, sizeof(sig_act));
     sig_act.sa_handler = signal_handler;
     sigfillset(&sig_act.sa_mask);
     sigaction(SIGINT, &sig_act, NULL);
     this->shutd_flag = std::make_shared<std::atomic<bool>>(false);

     /* Listen */
     conn_listen();
}

void TFTPServer::conn_listen() {
     Logger::glob_op("Listening for connections...");

     /** @see
      * https://moodle.vut.cz/pluginfile.php/550189/mod_folder/content/0/IPK2022-23L-03-PROGRAMOVANI.pdf#page=23
      */
     while (true) {
          /* Check for SIGINT */
          if (quit.load()) return this->stop();

          /* Prepare all the variables */
          struct sockaddr_in c_addr {};
          socklen_t c_addr_len = sizeof(c_addr);
          std::array<char, TFTP_MAX_PACKET> buffer{0};
          memset(buffer.data(), 0, buffer.size());
          memset(&c_addr, 0, c_addr_len);

          /* Check for and remove closed connections */
          // @see: https://stackoverflow.com/a/39019851
          connections.erase(
              std::remove_if(
                  connections.begin(), connections.end(),
                  [](const std::shared_ptr<TFTPServerConnection>& conn) {
                       return !conn->is_running();
                  }),
              connections.end());

          /* Recieve message */
          ssize_t read_size = recvfrom(
              this->fd, buffer.data(), TFTP_MAX_PACKET, 0,
              reinterpret_cast<struct sockaddr*>(&c_addr), &c_addr_len);

          /* Handle errors */
          if (read_size < 0) {
               if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    /* No new connections */
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(TFTP_THREAD_DELAY));
                    continue;
               }

               Logger::glob_err("Failed to receive data: "
                                + std::string(strerror(errno)));
               continue;
          }

          /* Parse incoming packet */
          auto packet_ptr = PacketFactory::create(buffer, read_size);
          if (!packet_ptr) {
               Logger::glob_err("Received an unparsable packet!");
               continue;
          }

          Logger::packet(*packet_ptr, c_addr);

          if (packet_ptr->get_opcode() != TFTPOpcode::RRQ
              && packet_ptr->get_opcode() != TFTPOpcode::WRQ) {
               // TODO: Am I supposed to handle this somehow?
               continue;
          }

          auto* req_packet_ptr = dynamic_cast<RequestPacket*>(packet_ptr.get());

          /* Instantiate connection instance */
          auto conn = std::make_shared<TFTPServerConnection>(
              c_addr, *req_packet_ptr, this->rootdir, this->shutd_flag);
          this->connections.push_back(conn);

          /* Exec connection in a separate thread */
          std::thread tConn(&TFTPServerConnection::run, conn);
          tConn.detach();

          /* Short sleep (to not overload CPU) */
          std::this_thread::sleep_for(
              std::chrono::milliseconds(TFTP_THREAD_DELAY));
     }
}

void TFTPServer::stop() {
     Logger::glob_op("Stopping server...");

     /* Set shared shutdown flag */
     this->shutd_flag->store(true);

     /* Wait for all connections to shutdown */
     while (!connections.empty()) {
          /* Erase closed connections */
          connections.erase(
              std::remove_if(
                  connections.begin(), connections.end(),
                  [](const std::shared_ptr<TFTPServerConnection>& conn) {
                       return !conn->is_running();
                  }),
              connections.end());

          std::this_thread::sleep_for(
              std::chrono::milliseconds(TFTP_THREAD_DELAY));
     }

     shutdown(this->fd, SHUT_RDWR);
     close(this->fd);
     this->fd = -1;
}

/* === Helper Methods === */

bool TFTPServer::check_dir() const {
     struct stat sb;

     /** @see
      * https://www.geeksforgeeks.org/how-to-check-a-file-or-directory-exists-in-cpp/
      */
     if (stat(this->rootdir.c_str(), &sb) != 0) {
          Logger::glob_err("Failed to stat root directory: "
                           + std::string(strerror(errno)));
          return false;
     } else if (!(sb.st_mode & S_IFDIR)) {
          Logger::glob_err("Root directory is not a directory");
          return false;
     } else if (access(this->rootdir.c_str(), R_OK) != 0) {
          Logger::glob_err("Root directory is not readable");
          return false;
     } else if (access(this->rootdir.c_str(), W_OK) != 0) {
          Logger::glob_err("Root directory is not writable");
          return false;
     }

     return true;
}