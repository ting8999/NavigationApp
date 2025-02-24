// ======================================================================
// \title  GPS.cpp
// \author ting
// \brief  cpp file for GPS component implementation class
// ======================================================================

#include "Components/GPS/GPS.hpp"

namespace Gnc {

  // ----------------------------------------------------------------------
  // Component construction and destruction
  // ----------------------------------------------------------------------

  GPS ::
    GPS(const char* const compName) :
      GPSComponentBase(compName)
  {

  }

  GPS ::
    ~GPS()
  {

  }

  // ----------------------------------------------------------------------
  // Handler implementations for commands
  // ----------------------------------------------------------------------

  void GPS ::
    TODO_cmdHandler(
        FwOpcodeType opCode,
        U32 cmdSeq
    )
  {
    // TODO
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

}
