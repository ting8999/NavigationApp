// ======================================================================
// \title  GPS.cpp
// \author ting
// \brief  cpp file for GPS component implementation class
// ======================================================================

#include "Components/GPS/GPS.hpp"
#include "Fw/Types/BasicTypes.hpp"
#include "Fw/Logger/Logger.hpp"
// #include "Drv/ByteStreamDriverModel/ByteStreamRecvPortAc.hpp"
#include <cstring>
#include <string.h>

namespace Gnc {

  // ----------------------------------------------------------------------
  // Component construction and destruction
  // ----------------------------------------------------------------------

  GPS :: GPS(const char* const compName) : GPSComponentBase(compName){
    // Initialize the lock status to false
    m_locked = false;
    memset(this->m_uartBuffers, 0, sizeof(this->m_uartBuffers));
  }

  GPS ::
    ~GPS(void)
  {

  }

  // ----------------------------------------------------------------------
  // Handler implementations for user-defined typed input ports
  // ----------------------------------------------------------------------

  void GPS ::recv_handler(const NATIVE_INT_TYPE portNum,Fw::Buffer &recvBuffer,const Drv::RecvStatus &recvStatus){
    int status = 0;
    U32 buffsize = recvBuffer.getSize();
    char* ptr = reinterpret_cast<char*>(recvBuffer.getData());
    GpsPacket packet;

    if (recvStatus != Drv::RecvStatus::RECV_OK) {
        Fw::Logger::log("[WARNING] Received buffer with bad packet: %d\n", recvStatus);
        this->deallocate_out(0, recvBuffer);
        return;
    }
    // append to recv buffer
    if (this->m_recvSize < GPS_DATA_LENGTH){
      // copy data to buffer
      memcpy(&this->m_uartBuffers[this->m_recvSize], ptr, buffsize);
      this->m_recvSize += buffsize;
      this->m_uartBuffers[this->m_recvSize] = '\0';
    } else {
      
      float time, latitude, longitude, altitude;
      char ns, ew;
      int quality, satellites;
      float hdop, geoidheight;
      ptr = this->m_uartBuffers;

      while ((ptr = strstr(ptr, "$GPGGA")) != NULL) {
        
        int parsed = sscanf(ptr, "$GPGGA,%f,%f,%c,%f,%c,%d,%d,%f,%f,M,%f,M",
                            &time, &latitude, &ns, &longitude, &ew, &quality, &satellites, &hdop, &altitude, &geoidheight);
        if (parsed == 10) {
            //printf("Time: %f, Latitude: %f%c, Longitude: %f%c, Quality: %d, Satellites: %d, HDOP: %f, Altitude: %f, GeoidHeight: %f\n",
            //      time, latitude, ns, longitude, ew, quality, satellites, hdop, altitude, geoidheight);
            break;
        }
        ptr++;  // Move to the next character to avoid infinite loop
      
      }
      //printf("Time: %f, Latitude: %f%c, Longitude: %f%c, Quality: %d, Satellites: %d, HDOP: %f, Altitude: %f, GeoidHeight: %f\n\n",
      //            time, latitude, ns, longitude, ew, quality, satellites, hdop, altitude, geoidheight);
      this->m_recvSize = 0;
      
      // set the packet data
      packet.utcTime = time;
      packet.latitude = latitude;
      packet.northSouth = ns;
      packet.longitude = longitude;
      packet.eastWest = ew;
      packet.gpsQuality = quality;
      packet.numSatellites = satellites;
      packet.hdop = hdop;
      packet.altitude = altitude;        
      packet.geoidalSeparation  = geoidheight;
      
      float lat_deg = (int)(packet.latitude / 100.0f);  // Extracting degrees from ddmm.mm format
      float lat_min = packet.latitude - (lat_deg * 100.0f);  // Extracting minutes from ddmm.mm format
      float lat = lat_deg + lat_min / 60.0f;  // Converting to decimal format
      lat = lat * ((packet.northSouth == 'N') ? 1 : -1);  // Applying North/South orientation

      float lon_deg = (int)(packet.longitude / 100.0f);  // Extracting degrees from dddmm.mm format
      float lon_min = packet.longitude - (lon_deg * 100.0f);  // Extracting minutes from dddmm.mm format
      float lon = lon_deg + lon_min / 60.0f;  // Converting to decimal format
      lon = lon * ((packet.eastWest == 'E') ? 1 : -1);  // Applying East/West orientation

      this->tlmWrite_Gps_Latitude(lat);
      this->tlmWrite_Gps_Longitude(lon);
      this->tlmWrite_Gps_Altitude(packet.altitude);
      this->tlmWrite_Gps_Count(packet.numSatellites);

      if (packet.gpsQuality == 0 && m_locked) {
          m_locked = false;
          this->log_WARNING_HI_Gps_LockLost();
      } else if (packet.gpsQuality >= 1 && !m_locked) {
          m_locked = true;
          this->log_ACTIVITY_HI_Gps_LockAquired();
      }

    }
    this->deallocate_out(0, recvBuffer);
}

  // ----------------------------------------------------------------------
  // Command handler implementations
  // ----------------------------------------------------------------------
  //Step 7,8: respond to a command to report lock status.
  //
  // When a status command is received, respond by emitting the
  // current lock status as an Event.
  void GPS ::
    Gps_ReportLockStatus_cmdHandler(
        const FwOpcodeType opCode,
        const U32 cmdSeq
    )
  {
    //Locked-force print
    if (m_locked) {
        log_ACTIVITY_HI_Gps_LockAquired();
    } else {
        log_WARNING_HI_Gps_LockLost();
    }
    //Step 9: complete command
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

}
