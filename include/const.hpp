/**
 * Constants and enumerations used in
 * the TFTP protocol.
 */

#ifndef TFTP_CONSTS_HPP
#define TFTP_CONSTS_HPP

/* === Constants === */

/**
 * @brief Maximum size of a TFTP DATA block.
 */
static const size_t TFTP_MAX_DATA = 512;

/**
 * @brief Maximum size of a TFTP packet.
 */
static const size_t TFTP_MAX_PACKET = TFTP_MAX_DATA + 4;

/**
 * @brief Maximum number of blocks TFTP can transfer.
 */
static const uint16_t TFTP_MAX_FILE_BLOCKS = 0xFFFF;

/**
 * @brief Default TFTP port
 * @see https://datatracker.ietf.org/doc/html/rfc1350#section-4
 */
static const uint16_t TFTP_PORT = 69;

/**
 * @brief Timeout for the TFTP server in seconds.
 */
static const int TFTP_TIMEO = 4;

/**
 * @brief Timeout for TFTP packets (retransmit after) in seconds.
 */
static const int TFTP_PACKET_TIMEO = 3;

/**
 * @brief Maximum numbeer of retransmit attempts.
 */
static const int TFTP_MAX_RETRIES = 4;

/**
 * @brief Short delay for the handling loop in miliseconds.
 */
static const int TFTP_THREAD_DELAY = 100;

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
 * @brief Enumeration of the request packet types.
 */
enum TFTPRequestType {
     Read = 0, /**< Read request */
     Write = 1 /**< Write request */
};

/**
 * @brief Enumeration of the data packet modes.
 */
enum TFTPDataFormat {
     Octet = 0,    /**< Octet mode */
     NetASCII = 1, /**< NetASCII mode */
};

/**
 * @brief Enumeration for all possible states of a connection
 */
enum class TFTPConnectionState {
     Idle,        /**< Initial state */
     Requested,   /**< Initial transitory state entered on WRQ/RRQ */
     Uploading,   /**< TRANSFERING state on reading */
     Downloading, /**< TRANSFERING state on writing */
     Awaiting,    /**< Awaiting ACK */
     Timedout,    /**< Lost packet state; attempts retransmit */
     Errored,     /**< Mid-transfer error; terminal state */
     Completed    /**< Transfer completed; terminal state */
};

#endif
