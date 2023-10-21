/**
 * @file main.cpp
 * @author Filip J. Kramec (xkrame00@vutbr.cz)
 * @brief Entry point of the TFTP server.
 * @date 2023-09-26
 */

#include <sys/stat.h>

#include "common.hpp"

/**
 * @brief Checks if the given path exists and can be accessed.
 *
 * @param path Path to check
 * @return true when path exists and can be read/written
 * @return false otherwise
 */
bool validatePath(const std::string& path) {
     struct stat sb;

     /** @see
      * https://www.geeksforgeeks.org/how-to-check-a-file-or-directory-exists-in-cpp/
      */
     if (stat(path.c_str(), &sb) != 0) {
          std::cerr << "!ERR! '" << path << "' does not seem exist!"
                    << std::endl;
          return false;
     } else if (!(sb.st_mode & S_IFDIR)) {
          std::cerr << "!ERR! '" << path << "' is not a directory!"
                    << std::endl;
          return false;
     } else if (access(path.c_str(), R_OK) != 0) {
          std::cerr << "!ERR! '" << path << "' is not readable!" << std::endl;
          return false;
     } else if (access(path.c_str(), W_OK) != 0) {
          std::cerr << "!ERR! '" << path << "' is not writable!" << std::endl;
          return false;
     }

     return true;
}

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

     if (!validatePath(rootdir)) {
          return EXIT_FAILURE;
     }

     // TODO: Implement server
     std::cerr << "TFTP server not implemented" << std::endl;
}