/**
 * @file DataPacket.hpp
 * @author Onegen Something <xkrame00@vutbr.cz>
 * @brief TFTP data packet.
 * @date 2023-09-28
 */

#include "packet/DataPacket.hpp"

/* === Constructors === */

DataPacket::DataPacket() : blockN(0) { opcode = TFTPOpcode::DATA; }

DataPacket::DataPacket(std::vector<char> data, uint16_t blockN)
    : blockN(blockN), data(data) {
     opcode = TFTPOpcode::DATA;
}

DataPacket::DataPacket(int fd, uint16_t blockN) : fd(fd), blockN(blockN) {
     opcode = TFTPOpcode::DATA;
}

DataPacket::DataPacket(const std::string& path, uint16_t blockN)
    : blockN(blockN) {
     opcode = TFTPOpcode::DATA;

     /* Open file */
     int fd = open(path.c_str(), O_RDONLY);
     if (fd == -1) throw std::runtime_error("Cannot open file");
     this->fd = fd;
}

/* === Core Methods === */

std::vector<char> DataPacket::readFileData() const {
     if (blockN < 1)
          return std::vector<char>();  // No data to read, return empty vector
     if (fd == -1)
          throw std::runtime_error(
              "Called readFileData() on invalid file descriptor");

     /* Seek to file start (NetASCII) or block start (octet) */
     off_t startOffset
         = mode == DataFormat::Octet ? (blockN - 1) * MAX_DATA_SIZE : 0;
     if (lseek(fd, startOffset, SEEK_SET) == -1)
          throw std::runtime_error("Cannot seek to file start");

     /* Binary data can be directly cut and returned */
     if (mode == DataFormat::Octet) {
          char data[MAX_DATA_SIZE];
          ssize_t bytesRead = read(fd, data, MAX_DATA_SIZE);
          if (bytesRead == -1)
               throw std::runtime_error("Could not read file (0 bytes read)");

          return std::vector<char>(data, data + bytesRead);
     }

     /* NetASCII data must be properly encoded – for size adjustment, must be
      * converted from start */
     off_t totalBytesRead = 0;
     off_t lastOffset = blockN * MAX_DATA_SIZE;
     std::vector<char> buffer;
     while (totalBytesRead < lastOffset) {
          char chunk[MAX_DATA_SIZE];
          ssize_t bytesRead = read(fd, chunk, sizeof(chunk));
          if (bytesRead == -1)
               throw std::runtime_error("Could not read file (0 bytes read)");
          else if (bytesRead == 0)
               break;  // EOF

          // TODO: Convert chunk to NetASCII
          std::vector<char> chunkVec(chunk, chunk + bytesRead);

          buffer.insert(buffer.end(), chunkVec.begin(), chunkVec.end());
          totalBytesRead += bytesRead;
     }

     /* Cut the last MAX_DATA_SIZE */
     auto startIt = buffer.begin() + ((blockN - 1) * MAX_DATA_SIZE);
     auto endIt = std::min(startIt + MAX_DATA_SIZE, buffer.end());
     return std::vector<char>(startIt, endIt);
}

std::vector<char> DataPacket::readData() const {
     /* Raw data – return cut block */
     if (!data.empty()) {
          size_t start = (blockN - 1) * MAX_DATA_SIZE;
          size_t end = blockN * MAX_DATA_SIZE;
          if (end > data.size()) end = data.size();
          return std::vector<char>(data.begin() + start, data.begin() + end);
     }

     /* File – read from file descriptor */
     if (fd != -1) return readFileData();

     /* No data – return empty vector */
     return std::vector<char>();
}

std::vector<char> DataPacket::toBinary() const {
     std::vector<char> fileData = readData();
     size_t length = 2 /* opcode */ + 2 /* block number */ + fileData.size();
     std::vector<char> binaryData(length);

     /* Convert and copy opcode & blockN in network byte order */
     uint16_t opcode = htons(static_cast<uint16_t>(this->opcode));
     uint16_t blockN = htons(this->blockN);
     std::memcpy(binaryData.data(), &opcode, sizeof(opcode));
     std::memcpy(binaryData.data() + 2, &blockN, sizeof(blockN));

     /* Insert data to vector */
     size_t offset = 4;  // after 2B opcode + 2B block number
     std::memcpy(binaryData.data() + offset, fileData.data(), fileData.size());

     return binaryData;
}

void DataPacket::fromBinary(const std::vector<char>& binaryData) {
     return fromBinary(binaryData, DataFormat::Octet);
}

void DataPacket::fromBinary(const std::vector<char>& binaryData,
                            DataFormat mode) {
     if (binaryData.size()
         < 4)  // Min. size is 4B (2B opcode + 2B block number)
          throw std::invalid_argument("Incorrect packet size");

     // TODO: Implement this method
     (void)binaryData;
     (void)mode;
}
