/**
 * @file main.cpp
 * @author Onegen Something (xkrame00@vutbr.cz)
 * @brief Entry point of the TFTP client.
 * @date 2023-09-26
 */

#include "client/client.hpp"
#include "common.hpp"

void send_help() {
     std::cout
         << "TFTP-Client (ISA 2023 by Onegen)" << std::endl
         << "Usage: tftp-client <-h hostname> [-p port] [-f path] <-t dest>"
         << std::endl
         << std::endl
         << " Option       Meaning" << std::endl
         << "  -h           IP or hostname of the remote TFTP server"
         << std::endl
         << "  -p port      Port to connect to (default: 69)" << std::endl
         << "  -f path      Path to remote file to download" << std::endl
         << "                If unset, data to upload are read from stdin"
         << std::endl
         << "  -t dest      Path where to upload/download the file"
         << std::endl;
}

int main(int argc, char* argv[]) {
     /* No options – send help */
     if (argc == 1) {
          send_help();
          return EXIT_SUCCESS;
     }

     std::string usage
         = "  Usage: tftp-client <-h hostname> [-p port] [-f path] <-t dest>\n"
           "   Try 'tftp-client' (no opts) for more info.";

     /* Parse command line options */
     int opt;
     int port = TFTP_PORT;
     std::string hostname;
     std::optional<std::string> filepath = std::nullopt;
     std::string destpath;
     while ((opt = getopt(argc, argv, "h:p:f:t:")) != -1) {
          switch (opt) {
               case 'h':
                    hostname = optarg;
                    break;
               case 'p':
                    port = std::stoi(optarg);
                    break;
               case 'f':
                    filepath = optarg;
                    break;
               case 't':
                    destpath = optarg;
                    break;
               default:
                    std::cerr << usage << std::endl;
                    return EXIT_FAILURE;
          }
     }

     /* Validate options */
     if (hostname.empty()) {
          std::cerr << "!ERR! Hostname not specified!" << std::endl
                    << usage << std::endl;
          return EXIT_FAILURE;
     }

     if (port == 0) {
          std::cerr << "!ERR! Invalid port!" << std::endl << usage << std::endl;
          return EXIT_FAILURE;
     }

     if (destpath.empty()) {
          std::cerr << "!ERR! Destination path not specified!" << std::endl
                    << usage << std::endl;
          return EXIT_FAILURE;
     }

     /* Create client */
     try {
          TFTPClient client(hostname, port, destpath, filepath);
          client.run();
     } catch (const std::exception& e) {
          std::cerr << "!ERR! " << e.what() << std::endl;
     }
}