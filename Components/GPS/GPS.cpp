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

  GPS ::
    GPS(const char* const compName) :
      GPSComponentBase(compName),
      // Initialize the lock to "false"
      m_locked(false)
  {

  }

  void GPS :: init(
      const NATIVE_INT_TYPE queueDepth,
      const NATIVE_INT_TYPE instance
    )
  {
    GPSComponentBase::init(queueDepth, instance);
    memset(m_gps_buff, '\0', GPS_READ_BUFF_SIZE * sizeof(char));
    memset(m_nmea_buff, '\0', GPS_READ_BUFF_SIZE * sizeof(char));
  }

  //Step 0: The linux serial driver keeps its storage externally. This means that we need to supply it some buffers to
  //        work with. This code will loop through our member variables holding the buffers and send them to the linux
  //        serial driver.  'preamble' is automatically called after the system is constructed, before the system runs
  //        at steady-state. This allows for initialization code that invokes working ports.
  void GPS :: preamble(void)
  {
    for (NATIVE_INT_TYPE buffer = 0; buffer < NUM_UART_BUFFERS; buffer++) {
      //Assign the raw data to the buffer. Make sure to include the side of the region assigned.
      this->m_recvBuffers[buffer].setData((U8*)this->m_uartBuffers[buffer]);
      this->m_recvBuffers[buffer].setSize(UART_READ_BUFF_SIZE);
      // Invoke the port to send the buffer out.
      this->serialBufferOut_out(0, this->m_recvBuffers[buffer]);
    }
  }

  GPS ::
    ~GPS(void)
  {

  }

  // ----------------------------------------------------------------------
  // Handler implementations for user-defined typed input ports
  // ----------------------------------------------------------------------

  // Step 1: serialIn
  //
  // By implementing this "handler" we can respond to the serial device sending us data buffers containing the GPS
  // data. This handles our serial messages. It should perform the actions we expect from the design phases.
  void GPS :: serialRecv_handler(
      const NATIVE_INT_TYPE portNum, /*!< The port number*/
      Fw::Buffer &serBuffer, /*!< Buffer containing data*/
      const Drv::RecvStatus &serial_status /*!< Status of read*/
    )
  {
    // Local variable definitions
    int status = 0;
    float lat = 0.0f, lon = 0.0f;
    GpsPacket packet;
    // Grab the size (used amount of the buffer) and a pointer to the data in the buffer
    U32 buffsize = static_cast<U32>(serBuffer.getSize());
    char* pointer = reinterpret_cast<char*>(serBuffer.getData());
    // Check for invalid read status, log an error, return buffer and abort if there is a problem
    // (void) printf("received\n");
    if (serial_status != Drv::RecvStatus::RECV_OK) {
      // Fw::Logger::logMsg("[WARNING] Received buffer with bad packet: %d\n", serial_status.e);
      // We MUST return the buffer or the serial driver won't be able to reuse it. The same buffer send call is used
      // as we did in "preamble".  Since the buffer's size was overwritten to hold the actual data size, we need to
      // reset it to the full data block size before returning it.
      serBuffer.setSize(UART_READ_BUFF_SIZE);
      this->serialBufferOut_out(0, serBuffer);
      return;
    }
    // If not enough data is available for a full messsage, return the buffer and abort.
    // else if (buffsize < 24) {
    //   // (void) printf("%d\n",buffsize);
    //   // (void) printf("%s\n",pointer);
      
    //     // We MUST return the buffer or the serial driver won't be able to reuse it. The same buffer send call is used
    //     // as we did in "preamble".  Since the buffer's size was overwritten to hold the actual data size, we need to
    //     // reset it to the full data block size before returning it.
    //     // serBuffer.setSize(UART_READ_BUFF_SIZE);
    //     // this->serialBufferOut_out(0, serBuffer);
    //     return;
    // }
    
    //Step 2:
    //  Parse the GPS message from the UART (looking for $GPGGA messages). This uses standard C functions to read all
    //  the defined protocol messages into our GPS package struct. If all 9 items are parsed, we break. Otherwise we
    //  continue to scan the block of data looking for messages further in.
    serBuffer.setSize(UART_READ_BUFF_SIZE);
    this->serialBufferOut_out(0, serBuffer);
    // (void) printf("%d\n",buffsize);
    // (void) printf("%s\n",pointer);
    bool nmea_complete = false;
    int buflen = strlen(m_gps_buff);
    //  (void) printf("gps_buf_size: %d\n",buflen);
    if (buflen + buffsize >= GPS_READ_BUFF_SIZE-1) 
    {
      (void) printf("gps buffer overflow\n");
      return;
    }
    else
    {
      int catlen = 0;
      for (U32 i = 0; i < buffsize; i++) {
        strncat(m_gps_buff, pointer+i, 1);
        catlen ++;
        if(pointer[i]=='\n'){
          nmea_complete = true;
          m_gps_buff[buflen + i+1] = '\0';
          strcpy(m_nmea_buff,m_gps_buff);
          buflen = 0;
          catlen = 0;
          m_gps_buff[0] = '\0';
          (void) printf("nmea_buf: %s\n",m_nmea_buff);
        }
      }
      m_gps_buff[buflen+catlen] = '\0';
      // (void) printf("gps_buf: %s\n",m_gps_buff);
    }
    if(!nmea_complete) return;
    // for (U32 i = 0; i < strlen(m_nmea_buff); i++) {
    status = sscanf(m_nmea_buff, "$GNGGA,%f,%f,%c,%f,%c,%u,%u,%f,%f",
        &packet.utcTime, &packet.dmNS, &packet.northSouth,
        &packet.dmEW, &packet.eastWest, &packet.lock,
        &packet.count, &packet.filler, &packet.altitude);
    //Break when all GPS items are found
    // if (status == 9) {
    //     break;
    // }
    // // }
    (void) printf("Status: %d\n",status);
      // (void) printf("%s\n",pointer);
      
    //If we failed to find the GPGGA then return the buffer and abort.
    if (status == 0) {
        return;
    }
    // If we found some of the message but not all of the message, then log an error, return the buffer and exit.
    else if (status != 9) {
        // Fw::Logger::logMsg("[ERROR] GPS parsing failed: %d\n", status);
        // We MUST return the buffer or the serial driver won't be able to reuse it. The same buffer send call is used
        // as we did in "preamble".  Since the buffer's size was overwritten to hold the actual data size, we need to
        // reset it to the full data block size before returning it.
        return;
    }
    //GPS packet locations are of the form: ddmm.mmmm
    //We will convert to lat/lon in degrees only before downlinking
    //Latitude degrees, add on minutes (converted to degrees), multiply by direction
    lat = (U32)(packet.dmNS/100.0f);
    lat = lat + (packet.dmNS - (lat * 100.0f))/60.0f;
    lat = lat * ((packet.northSouth == 'N') ? 1 : -1);
    //Longitude degrees, add on minutes (converted to degrees), multiply by direction
    lon = (U32)(packet.dmEW/100.0f);
    lon = lon + (packet.dmEW - (lon * 100.0f))/60.f;
    lon = lon * ((packet.eastWest == 'E') ? 1 : -1);
    //Step 4: call the downlink functions to send down data
    tlmWrite_Gps_Latitude(lat);
    tlmWrite_Gps_Longitude(lon);
    tlmWrite_Gps_Altitude(packet.altitude);
    tlmWrite_Gps_Count(packet.count);
    //Lock status update only if changed
    //Step 5,6: note changed lock status
    // Emit an event if the lock has been acquired, or lost
    if (packet.lock == 0 && m_locked) {
        m_locked = false;
        log_WARNING_HI_Gps_LockLost();
    } else if (packet.lock == 1 && !m_locked) {
        m_locked = true;
        log_ACTIVITY_HI_Gps_LockAquired();
    }
    // We MUST return the buffer or the serial driver won't be able to reuse it. The same buffer send call is used
    // as we did in "preamble".  Since the buffer's size was overwritten to hold the actual data size, we need to
    // reset it to the full data block size before returning it.
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
