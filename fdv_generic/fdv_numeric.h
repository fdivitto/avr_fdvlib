/*
# Created by Fabrizio Di Vittorio (fdivitto2013@gmail.com)
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



#ifndef FDV_NUMERIC_H
#define FDV_NUMERIC_H

namespace fdv
{

  template <typename InputIterator, typename T>
  T accumulate(InputIterator first, InputIterator last, T init)
  {
    while (first != last)
      init = init + *first++; // avoid to need +=
    return init;
  }


  template <typename InputIterator, typename T, typename BinaryOperation>
  T accumulate(InputIterator first, InputIterator last, T init, BinaryOperation binary_op)
  {
    while (first != last)
      init = binary_op(init, *first++);
    return init;
  }


  template <typename InputIterator, typename OutputIterator>
  OutputIterator adjacent_difference(InputIterator first, InputIterator last, OutputIterator result)
  {
    typename iterator_traits<InputIterator>::value_type val, prev;
    *result++ = prev = *first++;
    while (first != last) 
    {
      val = *first++;
      *result++ = val - prev;
      prev = val;
    }
    return result;
  }


  template <typename InputIterator, typename OutputIterator, typename BinaryOperation>
  OutputIterator adjacent_difference(InputIterator first, InputIterator last, OutputIterator result, BinaryOperation binary_op)
  {
    typename iterator_traits<InputIterator>::value_type val, prev;
    *result++ = prev = *first++;
    while (first != last) 
    {
      val = *first++;
      *result++ = binary_op(val, prev);
      prev = val;
    }
    return result;
  }


  template <typename InputIterator1, typename InputIterator2, typename T>
  T inner_product(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, T init)
  {
    while (first1 != last1)
      init = init + (*first1++) * (*first2++);
    return init;
  }


  template <typename InputIterator1, typename InputIterator2, typename T, typename BinaryOperation1, typename BinaryOperation2>
  T inner_product(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, T init, BinaryOperation1 binary_op1, BinaryOperation2 binary_op2)
  {
    while (first1 != last1)
      init = binary_op1(init, binary_op2(*first1++, *first2++));
    return init;
  }


  template <typename InputIterator, typename OutputIterator>
  OutputIterator partial_sum(InputIterator first, InputIterator last, OutputIterator result)
  {
    typename iterator_traits<InputIterator>::value_type val;
    *result++ = val = *first++;
    while (first != last)
      *result++ = val = val + *first++;
    return result;
  }


  template <typename InputIterator, typename OutputIterator, typename BinaryOperation>
  OutputIterator partial_sum(InputIterator first, InputIterator last, OutputIterator result, BinaryOperation binary_op)
  {
    typename iterator_traits<InputIterator>::value_type val;
    *result++ = val = *first++;
    while (first != last)
      *result++ = val = binary_op(val,*first++);
    return result;
  }




}


#endif // FDV_NUMERIC_H