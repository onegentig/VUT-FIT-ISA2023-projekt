/**
 * @file main.cpp
 * @author Onegen Something (xkrame00@vutbr.cz)
 * @brief Entry point of the TFTP server.
 * @date 2023-09-26
 */

#include "common.hpp"
#include "server/server.hpp"

void send_help() {
     std::cout << "TFTP-Server (ISA 2023 by Onegen)" << std::endl
               << "Usage: tftp-server [-h] [-p port] <path>" << std::endl
               << std::endl
               << " Option       Meaning" << std::endl
               << "  -h           Show this help message and exit" << std::endl
               << "  -p port      Port to listen on (default: 69)" << std::endl
               << "  <path>       Root folder of the TFTP server" << std::endl;
}

int main(int argc, char* argv[]) {
     /* No options – send help */
     if (argc == 1) {
          send_help();
          return EXIT_SUCCESS;
     }

     std::string usage
         = "  Usage: tftp-server [-h] [-p port] <path>\n"
           "   Try 'tftp-server -h' for more info.";

     /* Parse command line options */
     int opt;
     int port = TFTP_PORT;
     std::string rootdir;
     while ((opt = getopt(argc, argv, "hp:")) != -1) {
          switch (opt) {
               case 'p':
                    port = std::stoi(optarg);
                    break;
               case 'h':
                    send_help();
                    return EXIT_SUCCESS;
               default:
                    std::cerr << usage << std::endl;
                    return EXIT_FAILURE;
          }
     }

     if (optind >= argc) {
          // Path missing
          std::cerr << "!ERR! Root folder not specified!" << std::endl
                    << usage << std::endl;
          return EXIT_FAILURE;
     }

     rootdir = argv[optind];

     /* Validate options */
     if (port == 0) {
          std::cerr << "!ERR! Port not specified!" << std::endl
                    << usage << std::endl;
          return EXIT_FAILURE;
     }

     if (rootdir.empty()) {
          std::cerr << "!ERR! Root folder not specified!" << std::endl
                    << usage << std::endl;
          return EXIT_FAILURE;
     }

     /* Create server */
     try {
          TFTPServer server(rootdir, port);
          server.start();
     } catch (const std::exception& e) {
          std::cerr << "!ERR! " << e.what() << std::endl;
     }
}