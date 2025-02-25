// ======================================================================
// \title  GPS.hpp
// \author ting
// \brief  hpp file for GPS component implementation class
// ======================================================================

#ifndef Gnc_GPS_HPP
#define Gnc_GPS_HPP

#include "Components/GPS/GPSComponentAc.hpp"

// Need to define the memory footprint of our buffers. This means defining a count of buffers, and how big each is. In
// this example, we will allow the Gps component to manage its own buffers.
#define NUM_UART_BUFFERS 20
#define UART_READ_BUFF_SIZE 1024  // 1 kb
#define GPS_READ_BUFF_SIZE 256  // 256 bytes

namespace Gnc {

  class GPS :
    public GPSComponentBase {
    // GpsPacket:
    //   A structure containing the information in the GPS location packet received via the NMEA GPS receiver.
    struct GpsPacket {
      float utcTime;
      float dmNS;
      char northSouth;
      float dmEW;
      char eastWest;
      unsigned int lock;
      unsigned int count;
      float filler;
      float altitude;
    };
    public:

      // ----------------------------------------------------------------------
      // Component construction and destruction
      // ----------------------------------------------------------------------

      //! Construct GPS object
      GPS(
          const char* const compName //!< The component name
      );

      //! Initialize object Gps
      //!
      void init(
          const NATIVE_INT_TYPE queueDepth, /*!< The queue depth*/
          const NATIVE_INT_TYPE instance = 0 /*!< The instance number*/
      );

      //! Preamble
      //!
      void preamble(void);

      //! Destroy GPS object
      ~GPS(void);

    PRIVATE:
      // ----------------------------------------------------------------------
      // Handler implementations for user-defined typed input ports
      // ----------------------------------------------------------------------

      //! Handler implementation for serialRecv
      //!
      void serialRecv_handler(
          const NATIVE_INT_TYPE portNum, /*!< The port number*/
          Fw::Buffer &serBuffer, /*!< Buffer containing data*/
          const Drv::RecvStatus &serial_status /*!< Status of read*/
      );

    PRIVATE:

      // ----------------------------------------------------------------------
      // Handler implementations for commands
      // ----------------------------------------------------------------------

      //! Handler implementation for command Gps_ReportLockStatus
      //!
      //! A command to force an EVR reporting lock status.
      void Gps_ReportLockStatus_cmdHandler(
          const FwOpcodeType opCode, //!< The opcode
          const U32 cmdSeq //!< The command sequence number
      ) override;

      //!< Has the device acquired GPS lock?
      bool m_locked;
      //!< Create member variables to store buffers and the data array that those buffers use for storage
      Fw::Buffer m_recvBuffers[NUM_UART_BUFFERS];
      BYTE m_uartBuffers[NUM_UART_BUFFERS][UART_READ_BUFF_SIZE];
      char m_gps_buff[GPS_READ_BUFF_SIZE];
      char m_nmea_buff[GPS_READ_BUFF_SIZE];

  };

}

#endif
