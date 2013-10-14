/*
# Created by Fabrizio Di Vittorio (fdivitto@gmail.com)
# Copyright (c) 2013 Fabrizio Di Vittorio.
# All rights reserved.

# GNU GPL LICENSE
#
# This module is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; latest version thereof,
# available at: <http://www.gnu.org/licenses/gpl.txt>.
#
# This module is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this module; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
*/



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