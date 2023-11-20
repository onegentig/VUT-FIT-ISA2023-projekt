/**
 * @file connection.cpp
 * @author Onegen Something (xkrame00@vutbr,cz)
 * @brief TFTP connection implementation.
 * @date 2023-11-08
 */

#include "server/connection.hpp"

/* === Setup methods === */

TFTPServerConnection::TFTPServerConnection(
    const sockaddr_in& clt_addr, const RequestPacket& reqPacket,
    const std::string& rootDir,
    const std::shared_ptr<std::atomic<bool>>& shutd_flag)
    : shutd_flag(*shutd_flag) {
     this->rem_addr = clt_addr;

     this->file_name = rootDir + "/" + reqPacket.get_filename();
     this->type = reqPacket.get_type();
     this->format = reqPacket.get_mode();
     this->opts = this->proc_opts(reqPacket.get_options());
}

/* === Virtuals === */

/* == Handlers == */

/**
 * @brief Handles an incoming RRQ
 */
void TFTPServerConnection::handle_request_upload() {
     log_info("Requesting read of file " + this->file_name);

     /* Check if file exists */
     if (access(this->file_name.c_str(), F_OK) != 0)
          return this->send_error(TFTPErrorCode::FileNotFound,
                                  "File does not exist");

     /* Open file for reading */
     this->file_fd = open(this->file_name.c_str(), O_RDONLY);
     if (this->file_fd < 0) {
          TFTPErrorCode errcode;
          std::string errmsg;

          if (errno == ENOENT) {
               errcode = TFTPErrorCode::FileNotFound;
               errmsg = "File not found";
          } else if (errno == EACCES) {
               errcode = TFTPErrorCode::AccessViolation;
               errmsg = "Permission denied";
          } else {
               errcode = TFTPErrorCode::AccessViolation;
               errmsg = "Failed to open file";
          }

          return this->send_error(errcode, errmsg);
     }

     /* Check if file doesn’t exceed max allowed size */
     /** @see https://stackoverflow.com/a/6039648 */
     struct stat st;
     if (fstat(this->file_fd, &st) != 0
         || static_cast<uint32_t>(st.st_size)
                > static_cast<uint32_t>(this->blksize) * TFTP_MAX_FILE_BLOCKS
                      - 1) {
          return this->send_error(TFTPErrorCode::Unknown, "File too big");
     }

     /* If any options were accepted, set `oack_init` */
     this->oack_init = !this->opts.empty();

     /* Things are ready for transfer */
     log_info("File ready, starting upload");
     this->set_init_state(TFTPConnectionState::Uploading);
}

/**
 * @brief Handles an incoming WRQ
 */
void TFTPServerConnection::handle_request_download() {
     log_info("Requesting write of file " + this->file_name);

     /* Check if file exists */
     /** @see https://stackoverflow.com/a/12774387 */
     if (access(this->file_name.c_str(), F_OK) == 0)
          return this->send_error(TFTPErrorCode::FileAlreadyExists,
                                  "File already exists");

     /* Create a part file */
     this->file_fd = open(this->file_name.c_str(), O_WRONLY | O_CREAT | O_TRUNC,
                          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

     /* Check if file was created properly */
     if (this->file_fd < 0)
          return this->send_error(TFTPErrorCode::AccessViolation,
                                  "Could not create file");
     this->file_created = true;

     /* If any options were accepted, set `oack_init` */
     this->oack_init = !this->opts.empty();

     /* Things are ready for transfer */
     log_info("File ready, starting download");
     this->set_init_state(TFTPConnectionState::Downloading);
}

/**
 * @brief Checks if the client should shut down
 * @return true if should shut down,
 * @return false otherwise
 */
bool TFTPServerConnection::should_shutd() { return this->shutd_flag.load(); }

/**
 * @brief Obtains the next DataPacket payload to be sent
 * @return std::vector<char> Serialised DataPacket payload
 */
std::vector<char> TFTPServerConnection::next_data() {
     /* Create data payload from file */
     DataPacket packet = DataPacket(this->file_fd, this->block_n);
     packet.set_mode(this->format);
     packet.set_block_size(this->blksize);
     return packet.to_binary();
}
