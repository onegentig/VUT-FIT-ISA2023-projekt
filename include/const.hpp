/**
 * Constants and enumerations used in
 * the TFTP protocol.
 */

#ifndef CONST_HPP
#define CONST_HPP

/* === Constants === */

/**
 * @brief Maximum number of bytes of a cstring to print in `dprint()`.
 */
constexpr uint8_t MAX_DEBUG_STRING = 5;

/**
 * @brief Maximum size of a TFTP data packet.
 */
static const size_t MAX_DATA_SIZE = 512;

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
 * @brief Enumeration of the data packet modes.
 */
enum DataFormat {
     Octet = 0,    /**< Octet mode */
     NetASCII = 1, /**< NetASCII mode */
};

#endif
