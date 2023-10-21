/**
 * Constants and enumerations used in
 * the TFTP protocol.
 */

#ifndef CONST_HPP
#define CONST_HPP

/* === Constants === */

/**
 * @brief Maximum size of a TFTP data packet.
 */
static const size_t MAX_DATA_SIZE = 512;

/**
 * @brief Default TFTP port
 * @see https://datatracker.ietf.org/doc/html/rfc1350#section-4
 */
static const uint16_t DEFAULT_TFTP_PORT = 69;

/**
 * @brief Timeout for the TFTP server in seconds.
 */
static const int TIMEOUT = 4;

/* === Enumerations === */

/**
 * @brief Enumeration of all two-byte TFTP opcodes
 * as defined in RFC 1350.
 * @see https://datatracker.ietf.org/doc/html/rfc1350#autoid-5
 */
enum TFTPOpcode : uint16_t {
     RRQ = 1,   /**< Read request */
     WRQ = 2,   /**< Write request */
     DATA = 3,  /**< Data */
     ACK = 4,   /**< Acknowledgement */
     ERROR = 5, /**< Error */
};

/**
 * @brief Enumeration of TFTP error codes (for ERROR packets).
 * @see https://datatracker.ietf.org/doc/html/rfc1350#page-10
 */
enum TFTPErrorCode : uint16_t {
     Unknown = 0,           /**< Not defined, see error message (if any). */
     FileNotFound = 1,      /**< File not found. */
     AccessViolation = 2,   /**< Access violation. */
     DiskFull = 3,          /**< Disk full or allocation exceeded. */
     IllegalOperation = 4,  /**< Illegal TFTP operation. */
     UnknownTID = 5,        /**< Unknown transfer ID. */
     FileAlreadyExists = 6, /**< File already exists. */
     NoSuchUser = 7,        /**< No such user. */
};

/**
 * @brief Enumeration of the data packet modes.
 */
enum TFTPDataFormat {
     Octet = 0,    /**< Octet mode */
     NetASCII = 1, /**< NetASCII mode */
};

#endif
