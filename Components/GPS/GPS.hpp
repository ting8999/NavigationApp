// ======================================================================
// \title  GPS.hpp
// \author ting
// \brief  hpp file for GPS component implementation class
// ======================================================================

#ifndef Gnc_GPS_HPP
#define Gnc_GPS_HPP

#include "Components/GPS/GPSComponentAc.hpp"

#define GPS_DATA_LENGTH 1024    //bytes

namespace Gnc {

  class GPS :
    public GPSComponentBase
  {

    /**
   * GpsPacket:
   *   A structure containing the information in the GPS location packet
   * received via the NMEA GPS receiver.
   */
  struct GpsPacket {
    float utcTime;           // 1) Time (UTC)
    float latitude;          // 2) Latitude (in ddmm.mm format)
    char northSouth;         // 3) N or S (North or South)
    float longitude;         // 4) Longitude (in dddmm.mm format)
    char eastWest;           // 5) E or W (East or West)
    unsigned int gpsQuality; // 6) GPS Quality Indicator
    unsigned int numSatellites; // 7) Number of satellites in view
    float hdop;              // 8) Horizontal Dilution of Precision
    float altitude;          // 9) Antenna Altitude above/below mean-sea-level (geoid)
    char altitudeUnits;      // 10) Units of antenna altitude
    float geoidalSeparation; // 11) Geoidal separation
    char geoidalUnits;       // 12) Units of geoidal separation
    float dgpsDataAge;       // 13) Age of differential GPS data
    unsigned int dgpsStationId; // 14) Differential reference station ID
};


    public:

      // ----------------------------------------------------------------------
      // Component construction and destruction
      // ----------------------------------------------------------------------

      //! Construct GPS object
      GPS(
          const char* const compName //!< The component name
      );

      //! Destroy GPS object
      ~GPS();

    PRIVATE:
      // ----------------------------------------------------------------------
      // Handler implementations for typed input ports
      // ----------------------------------------------------------------------

      //! Handler implementation for recv
      void recv_handler(
        const NATIVE_INT_TYPE portNum, /*!< The port number*/
        Fw::Buffer &recvBuffer, 
        const Drv::RecvStatus &recvStatus 
      );

    PRIVATE:

      // ----------------------------------------------------------------------
      // Handler implementations for commands
      // ----------------------------------------------------------------------

      //! Handler implementation for command Gps_ReportLockStatus
      //!
      //! A command to force an EVR reporting lock status.
      void Gps_ReportLockStatus_cmdHandler(
          const  FwOpcodeType opCode, //!< The opcode
          U32 cmdSeq //!< The command sequence number
      ) override;

      //!< Has the device acquired GPS lock?
      bool m_locked;
      //!< Create member variables to store buffers and the data array that those buffers use for storage
      char m_uartBuffers[GPS_DATA_LENGTH];
      U16 m_recvSize = 0;

  };

}

#endif
