/**
 * @file DataPacket.hpp
 * @author Filip J. Kramec <xkrame00@vutbr.cz>
 * @brief TFTP data packet.
 * @date 2023-09-28
 */

#include "packet/DataPacket.hpp"

/* === Constructors === */

DataPacket::DataPacket() : block_n(0) { opcode = TFTPOpcode::DATA; }

DataPacket::DataPacket(std::vector<char> data, uint16_t block_n)
    : block_n(block_n), data(std::move(data)) {
     opcode = TFTPOpcode::DATA;
}

DataPacket::DataPacket(int fd, uint16_t block_n) : fd(fd), block_n(block_n) {
     opcode = TFTPOpcode::DATA;
}

DataPacket::DataPacket(const std::string& path, uint16_t block_n)
    : block_n(block_n) {
     opcode = TFTPOpcode::DATA;

     /* Open file */
     int fd = open(path.c_str(), O_RDONLY);
     if (fd == -1) throw std::runtime_error("Cannot open file");
     this->fd = fd;
}

/* === Core Methods === */

std::vector<char> DataPacket::read_file_data() const {
     if (block_n < 1)
          return std::vector<char>();  // No data to read, return empty vector
     if (fd == -1)
          throw std::runtime_error(
              "Called read_file_data() on invalid file descriptor");

     /* Seek to file start (NetASCII) or block start (octet) */
     off_t ofst_start
         = mode == TFTPDataFormat::Octet ? (block_n - 1) * TFTP_MAX_DATA : 0;
     if (lseek(fd, ofst_start, SEEK_SET) == -1)
          throw std::runtime_error("Cannot seek to file start");

     /* Binary data can be directly cut and returned */
     if (mode == TFTPDataFormat::Octet) {
          char data[TFTP_MAX_DATA];
          ssize_t bytes_rx = read(fd, data, TFTP_MAX_DATA);
          if (bytes_rx == -1)
               throw std::runtime_error("Could not read file (0 bytes read)");

          return std::vector<char>(data, data + bytes_rx);
     }

     /* NetASCII data must be properly encoded – for size adjustment, must be
      * converted from start */
     off_t bytes_all = 0;
     off_t ofst_last = block_n * TFTP_MAX_DATA;
     std::vector<char> buffer;
     while (bytes_all < ofst_last) {
          std::vector<char> chunk(TFTP_MAX_DATA);
          ssize_t bytes_rx = read(fd, chunk.data(), chunk.size());

          if (bytes_rx == -1) {
               throw std::runtime_error("Could not read file (-1 error)");
          } else if (bytes_rx == 0) {
               break;  // EOF
          }

          /* Convert to NetASCII */
          chunk.resize(bytes_rx);
          chunk = NetASCII::to_netascii(chunk);

          /* Add to buffer */
          buffer.insert(buffer.end(), chunk.begin(), chunk.end());
          bytes_all += bytes_rx;
     }

     /* Cut the last MAX_DATA_SIZE */
     auto it_start = buffer.begin() + ((block_n - 1) * TFTP_MAX_DATA);
     auto it_end = std::min(it_start + TFTP_MAX_DATA, buffer.end());
     return std::vector<char>(it_start, it_end);
}

std::vector<char> DataPacket::read_data() const {
     /* Raw data – return cut block */
     if (!data.empty()) {
          size_t start = (block_n - 1) * TFTP_MAX_DATA;
          size_t end = block_n * TFTP_MAX_DATA;
          if (end > data.size()) end = data.size();
          return std::vector<char>(data.begin() + start, data.begin() + end);
     }

     /* File – read from file descriptor */
     if (fd != -1) return read_file_data();

     /* No data – return empty vector */
     return std::vector<char>();
}

std::vector<char> DataPacket::to_binary() const {
     std::vector<char> filedata = read_data();
     size_t length = 2 /* opcode */ + 2 /* block number */ + filedata.size();
     std::vector<char> bin_data(length);

     /* Convert and copy opcode & block_n in network byte order */
     uint16_t opcode = htons(static_cast<uint16_t>(this->opcode));
     uint16_t block_n = htons(this->block_n);
     std::memcpy(bin_data.data(), &opcode, sizeof(opcode));
     std::memcpy(bin_data.data() + 2, &block_n, sizeof(block_n));

     /* Insert data to vector */
     if (!filedata.empty()) {
          size_t offset = 4;  // after 2B opcode + 2B block number
          std::memcpy(bin_data.data() + offset, filedata.data(),
                      filedata.size());
     }

     return bin_data;
}

DataPacket DataPacket::from_binary(const std::vector<char>& bin_data) {
     return from_binary(bin_data, TFTPDataFormat::Octet);
}

DataPacket DataPacket::from_binary(const std::vector<char>& bin_data,
                                   TFTPDataFormat mode) {
     if (bin_data.size() < 4)  // Min. size is 4B (2B opcode + 2B block number)
          throw std::invalid_argument("Incorrect packet size");

     /* Obtain and validate opcode */
     uint16_t opcode;
     std::memcpy(&opcode, bin_data.data(), sizeof(opcode));
     opcode = ntohs(opcode);

     if (opcode != static_cast<uint16_t>(TFTPOpcode::DATA))
          throw std::invalid_argument("Incorrect opcode");

     /* Obtain and validate block number */
     uint16_t block_n;
     std::memcpy(&block_n, bin_data.data() + 2, sizeof(block_n));
     block_n = ntohs(block_n);

     /* Obtain data */
     size_t offset = 4;  // after 2B opcode + 2B block number
     std::vector<char> data
         = std::vector<char>(bin_data.begin() + offset, bin_data.end());

     DataPacket packet = DataPacket(data, block_n);
     packet.set_mode(mode);
     return packet;
}
