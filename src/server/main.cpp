/**
 * @file main.cpp
 * @author Filip J. Kramec (xkrame00@vutbr.cz)
 * @brief Entry point of the TFTP server.
 * @date 2023-09-26
 */

#include "common.hpp"

int main(int argc, char* argv[]) {
     std::string usage = "  Usage: tftp-server [-p port] <path>";

     /* Parse command line options */
     int opt;
     int port = DEFAULT_TFTP_PORT;
     std::string rootdir;
     while ((opt = getopt(argc, argv, "p:")) != -1) {
          switch (opt) {
               case 'p':
                    port = std::stoi(optarg);
                    break;
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

     // TODO: Implement server
     std::cerr << "TFTP server not implemented" << std::endl;
}