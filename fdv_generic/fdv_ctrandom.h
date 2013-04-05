// 2012 by Fabrizio Di Vittorio (fdivitto@tiscali.it)

#ifndef FDV_CTRANDOM_H
#define FDV_CTRANDOM_H

namespace fdv
{


  /////////////////////////////////////////////////////////////////////////////////
  // CompileTimeRandom

  template <uint16_t N>
  struct CompileTimeRandomAlgo
  {
    enum { value = 7 * CompileTimeRandomAlgo<N-1>::value + 17 };
  };

  template <>
  struct CompileTimeRandomAlgo<1>
  {
    enum { value = __COUNTER__};
  };

  // CompileTimeRandom::value will be 0...65535
  struct CompileTimeRandom 
  {
      enum { value = CompileTimeRandomAlgo<__COUNTER__>::value % 65535 };
  };



};  // end of fdv namespace



#endif /* FDV_CTRANDOM_H */