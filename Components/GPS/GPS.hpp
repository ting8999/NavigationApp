// ======================================================================
// \title  GPS.hpp
// \author ting
// \brief  hpp file for GPS component implementation class
// ======================================================================

#ifndef Gnc_GPS_HPP
#define Gnc_GPS_HPP

#include "Components/GPS/GPSComponentAc.hpp"

namespace Gnc {

  class GPS :
    public GPSComponentBase
  {

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
      // Handler implementations for commands
      // ----------------------------------------------------------------------

      //! Handler implementation for command TODO
      //!
      //! TODO
      void TODO_cmdHandler(
          FwOpcodeType opCode, //!< The opcode
          U32 cmdSeq //!< The command sequence number
      ) override;

  };

}

#endif
