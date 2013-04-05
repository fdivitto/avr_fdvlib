// 2010 by Fabrizio Di Vittorio (fdivitto@tiscali.it)

#include <inttypes.h>

#include "fdv_wirelessrpc.h"

namespace fdv
{


  // storage for WirelessRPC class
  uint8_t WirelessRPC::s_localDeviceID;
  bool WirelessRPC::s_serverMode;
  uint32_t WirelessRPC::ACKTimeouts = 0;
  uint32_t WirelessRPC::SuccessCalls;
  uint32_t WirelessRPC::FailedCalls;



} // end of fdv namespace
