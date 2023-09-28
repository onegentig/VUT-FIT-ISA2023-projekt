/**
 * Constants and enumerations used in
 * the TFTP protocol.
 */

#ifndef CONST_HPP
#define CONST_HPP

/**
 * @brief Maximum number of bytes of a cstring to print in `dprint()`.
 */
constexpr uint8_t MAX_DEBUG_STRING = 5;

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

#endif
