/**
 * @file server.hpp
 * @author Filip J. Kramec (xkrame00@vutbr.cz)
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

/**
 * @details The `start` method creates a socket for the server, binds it, and
 * then starts listening for new connections in `srv_poll`, making this call
 *          blocking.
 */
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
     srv_poll();
}

/**
 * @details `srv_poll` is the main loop of the server, listening for new
 *          connections and executing running connections. The server loop
 *          uses `poll()` to wait for events on `pollfd` objects in `fds`
 *          and handles them accordingly.
 */
void TFTPServer::srv_poll() {
     /* Add server to polling vector */
     this->srv_fd.fd = this->fd;
     this->srv_fd.events = POLLIN;
     this->fds.push_back(this->srv_fd);
     Logger::glob_op("Listening for connections...");

     /** @see https://beej.us/guide/bgnet/pdf/bgnet_a4_c_1.pdf#section.7.2 */
     while (true) {
          /* Check for SIGINT */
          if (quit.load()) return this->stop();

          /* Finished connections cleanup */
          this->conn_cleanup(); /** @see TFTPServer::conn_cleanup */

          /* Poll */
          int n_events = poll(fds.data(), fds.size(), POLL_TIMEO);
          if (n_events == 0) continue;  // Nothing new on the server front

          /* Check for errors */
          if (n_events < 0) {
               if (errno == EINTR) continue;  // Handled by signal handler
               throw std::runtime_error("Polling error : "
                                        + std::string(strerror(errno)));
          }

          /* Handling loop */
          for (auto& fd : fds) {
               if (fd.revents & POLLIN) {
                    /* Server event => new connection */
                    if (fd.fd == this->fd) {
                         this->new_conn();
                         continue;
                    }

                    /* Connection event => continue `exec()` */
                    auto conn = this->find_conn(fd.fd);
                    if (!conn) continue;
                    conn->get()->exec(); /** @see TFTPConnectionBase::exec */

                    /* Remove connection, if finished */
                    if (!conn->get()->is_running())
                         this->conn_remove(
                             fd.fd); /** @see TFTPServer::conn_remove */
               }

               /* Clear events */
               fd.revents &= 0;
          }
     }
}

void TFTPServer::stop() {
     Logger::glob_op("Stopping server...");

     /* Set shared shutdown flag */
     this->shutd_flag->store(true);

     /* Wait for all connections to close */
     while (!this->connections.empty()) {
          /* `exec` connections to let them transition to `Errored` */
          for (auto& conn : this->connections) conn->exec();

          /* Cleanup */
          this->conn_cleanup(); /** @see TFTPServer::conn_cleanup */
          std::this_thread::sleep_for(
              std::chrono::milliseconds(TFTP_THREAD_DELAY));
     }

     shutdown(this->fd, SHUT_RDWR);
     close(this->fd);
     this->fd = -1;
}

/* == `poll()` handler methods == */

/**
 * @details `new_conn` is a `srv_poll` subroutine that handles new connections,
 *          first obtaining and parsing the packet, logging it, validating that
 *          it is a RRQ/WRQ and then instantiating a new connection object.
 */
void TFTPServer::new_conn() {
     /* Prepare addr and buffer */
     struct sockaddr_in c_addr;
     socklen_t c_addr_len = sizeof(c_addr);
     std::array<char, TFTP_MAX_PACKET> buffer{};

     /* Receive packet */
     ssize_t bytes_rx = recvfrom(srv_fd.fd, buffer.data(), buffer.size(), 0,
                                 (struct sockaddr*)&c_addr, &c_addr_len);
     if (bytes_rx <= 0) return;

     /* Parse packet */
     auto packet_ptr = PacketFactory::create(buffer, bytes_rx);
     if (!packet_ptr) return Logger::glob_err("Received an unparsable packet!");

     Logger::packet(*packet_ptr, c_addr);
     if (packet_ptr->get_opcode() != TFTPOpcode::RRQ
         && packet_ptr->get_opcode() != TFTPOpcode::WRQ) {
          // TODO: Am I supposed to handle this somehow?
          return;
     }

     Logger::glob_event("New connection from "
                        + std::string(inet_ntoa(c_addr.sin_addr)) + ":"
                        + std::to_string(ntohs(c_addr.sin_port)));

     /* Cast to RRQ/WRQ */
     RequestPacket* req_packet_ptr
         = dynamic_cast<RequestPacket*>(packet_ptr.get());

     /* Instantiate a connection */
     auto conn = std::make_shared<TFTPServerConnection>(
         c_addr, *req_packet_ptr, this->rootdir, this->shutd_flag);
     conn->set_addr_static();  // Client already has generated TID
     conn->set_await_exit();   // `Awaiting` should only progress on `poll()`
                               // event
     conn->sock_init();

     /* Add connection to storage and polling vectors */
     struct pollfd conn_fd {
          conn->get_fd(), POLLIN, 0
     };

     this->connections.push_back(conn);
     this->fds.push_back(conn_fd);

     /* Send response to request */
     conn->exec();  // With `set_await_exit`, `exec` will stop after sending
                    // response
}

/**
 * @details The `conn_remove` is a clean-up method used to remove a specific
 *          fd-identified connection from both the `connections` vector and
 *          `fds` vector. This method is called after a connection finished
 *          execution. The cleanup could be made more efficient by ex. using
 *          a map or something, but… time crunch. :)
 */
void TFTPServer::conn_remove(int fd) {
     /* Remove from `connections` */
     for (size_t i = 0; i < connections.size(); ++i) {
          if (connections[i]->get_fd() == fd) {
               connections.erase(connections.begin() + i);
               break;
          }
     }

     /* Remove from `fds` */
     for (size_t i = 0; i < fds.size(); ++i) {
          if (fds[i].fd == fd) {
               fds.erase(fds.begin() + i);
               break;
          }
     }
}

/**
 * @details `conn_cleanup` is another clean-up subroutine of `srv_poll`, used
 *          to remove all finished (whether successfully or not) connections
 *          from the `connections` and `fds` vectors. It simply loops over
 *          `connections`, tests if they are running, and if not it calls
 *          `conn_remove` upon them. Simple and straightforward, but not
 *          that performant, so it is called only every 100th iteration of
 *          `srv_poll` (most conns are closed right away anyway).
 */
void TFTPServer::conn_cleanup() {
     for (size_t idx = 0; idx < this->connections.size(); idx++) {
          if (this->connections[idx]->is_running()) continue;
          this->conn_remove(this->connections[idx]->get_fd());
     }
}

/* === Helper Methods === */

/**
 * @details `find_conn` is a helper method used to find a connection object
 *          in the `connections` vector by its file descriptor. It is used
 *          to execute the connection by its `poll()` event. Again, could
 *          be made more efficient with maps or smth.
 */
std::shared_ptr<TFTPServerConnection>* TFTPServer::find_conn(int fd) {
     for (auto& conn : this->connections)
          if (conn->get_fd() == fd) return &conn;

     return nullptr;
}

/**
 * @details Very straightforward util that just checks whether the root
 *          directory is a valid directory and is readable and writable.
 * @see
 * https://www.geeksforgeeks.org/how-to-check-a-file-or-directory-exists-in-cpp/
 */
bool TFTPServer::check_dir() const {
     struct stat sb;

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