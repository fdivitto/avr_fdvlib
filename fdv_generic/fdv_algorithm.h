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


#ifndef FDV_ALGORITHM_H
#define FDV_ALGORITHM_H

#include <stdlib.h>  
#include <inttypes.h>
#include <math.h>


#include "fdv_utility.h"
#include "fdv_iterator.h"


#undef min
#undef max


namespace fdv
{



  template <typename T>
  inline T const& min(T const& a, T const& b)
  {
    return a < b ? a : b;
  }


  template <typename T, typename Compare>
  inline T const& min(T const& a, T const& b, Compare comp)
  {
    return comp(a, b) ? a : b;
  }


  template <typename T>
  inline T const& max(T const& a, T const& b)
  {
    return a > b ? a : b;
  }


  template <typename T, typename Compare>
  inline T const& max(T const& a, T const& b, Compare comp)
  {
    return comp(a, b) ? a : b;
  }


  template <typename InputIterator, typename OutputIterator>
  OutputIterator copy(InputIterator first, InputIterator last, OutputIterator result)
  {
    while (first != last)
      *result++ = *first++;
    return result;
  }


  template <typename InputIterator, typename OutputIterator>
  OutputIterator* copy(InputIterator* first, InputIterator* last, OutputIterator* result)
  {
    size_t len = last-first;
    if (len > 0)
      memcpy(result, first, len*sizeof(InputIterator));
    return result + len;
  }


  template<typename BidirectionalIterator1, typename BidirectionalIterator2>
  BidirectionalIterator2 copy_backward(BidirectionalIterator1 first,
    BidirectionalIterator1 last,
    BidirectionalIterator2 result)
  {
    while (last!=first)
      *(--result) = *(--last);
    return result;
  }


  template <typename T>
  void swap(T& a, T& b)
  {
    T tmp(a);
    a = b;
    b = tmp;
  }


  template <typename ForwardIterator, typename T>
  void fill(ForwardIterator first, ForwardIterator last, T const& value)
  {
    while (first != last)
      *first++ = value;
  }


  template <typename OutputIterator, typename Size, typename T>
  void fill_n(OutputIterator first, Size n, T const& value)
  {
    for (; n>0; --n)
      *first++ = value;
  }


  template <typename InputIterator1, typename InputIterator2>
  bool equal(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2)
  {
    while (first1 != last1)
      if (*first1++ != *first2++)
        return false;
    return true;
  }


  template <typename InputIterator1, typename InputIterator2, typename BinaryPredicate>
  bool equal(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, BinaryPredicate pred)
  {  
    while (first1 != last1)
      if (!pred(*first1++, *first2++))
        return false;
    return true;
  }


  template <typename InputIterator1, typename InputIterator2>
  bool lexicographical_compare(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2)
  {
    while (first1 != last1)
    {
      if (*first2 < *first1 || first2 == last2)
        return false;
      else if (*first1 < *first2)
        return true;
      first1++; first2++;
    }
    return first2 != last2;
  }


  template <typename InputIterator1, typename InputIterator2, typename Compare>
  bool lexicographical_compare(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, Compare comp)
  {
    while (first1 != last1)
    {
      if (comp(*first2, *first1) || first2 == last2)
        return false;
      else if (comp(*first1, *first2))
        return true;
      first1++; first2++;
    }
    return first2 != last2;
  }


  template<typename InputIterator, typename Function>
  Function for_each(InputIterator first, InputIterator last, Function f)
  {
    while (first != last)
      f(*first++);
    return f;
  }


  template<typename InputIterator, typename T>
  InputIterator find(InputIterator first, InputIterator last, T const& value)
  {
    for (; first!=last; first++)
      if (*first == value) 
        break;
    return first;
  }


  template<typename InputIterator, typename T, typename Predicate>
  InputIterator find(InputIterator first, InputIterator last, T const& value, Predicate pred)
  {
    for (; first!=last; first++)
      if (pred(*first, value))
        break;
    return first;
  }


  template<typename InputIterator, typename Predicate>
  InputIterator find_if(InputIterator first, InputIterator last, Predicate pred)
  {
    for (; first!=last; first++)
      if (pred(*first)) 
        break;
    return first;
  }


  template<typename ForwardIterator1, typename ForwardIterator2>
  ForwardIterator1 find_end(ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2, ForwardIterator2 last2)
  {
    ForwardIterator1 limit = first1;
    advance(limit, 1 + distance(first1, last1) - distance(first2, last2));
    ForwardIterator1 ret = last1;

    while (first1 != limit)
    {
      ForwardIterator1 it1 = first1;
      ForwardIterator2 it2 = first2;
      while (*it1 == *it2)
      {
        ++it1; 
        ++it2; 
        if (it2 == last2) 
        {
          ret = first1;
          break;
        } 
      }
      ++first1;
    }
    return ret;
  }


  template<typename ForwardIterator1, typename ForwardIterator2, typename Predicate>
  ForwardIterator1 find_end(ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2, ForwardIterator2 last2, Predicate pred)
  {
    ForwardIterator1 limit = first1;
    advance(limit, 1 + distance(first1, last1) - distance(first2, last2));
    ForwardIterator1 ret = last1;

    while (first1 != limit)
    {
      ForwardIterator1 it1 = first1;
      ForwardIterator2 it2 = first2;
      while (pred(*it1, *it2))
      {
        ++it1; 
        ++it2; 
        if (it2 == last2) 
        {
          ret = first1;
          break;
        } 
      }
      ++first1;
    }
    return ret;
  }


  template <typename ForwardIterator1, typename ForwardIterator2>
  ForwardIterator1 find_first_of(ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2, ForwardIterator2 last2)
  {
    for (; first1 != last1; ++first1)
      for (ForwardIterator2 it=first2; it!=last2; ++it)
        if (*it == *first1)
          return first1;
    return last1;
  }


  template <typename ForwardIterator1, typename ForwardIterator2, typename Compare>
  ForwardIterator1 find_first_of(ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2, ForwardIterator2 last2, Compare comp)
  {
    for (; first1 != last1; ++first1)
      for (ForwardIterator2 it=first2; it!=last2; ++it)
        if (comp(*it, *first1))
          return first1;
    return last1;
  }


  template <typename ForwardIterator>
  ForwardIterator adjacent_find(ForwardIterator first, ForwardIterator last)
  {
    ForwardIterator next = first; 
    ++next;
    if (first != last)
      while (next != last)
        if (*first++ == *next++)
          return first;
    return last;
  }


  template <typename ForwardIterator, typename Predicate>
  ForwardIterator adjacent_find(ForwardIterator first, ForwardIterator last, Predicate pred)
  {
    ForwardIterator next = first; 
    ++next;
    if (first != last)
      while (next != last)
        if (pred(*first++, *next++))
          return first;
    return last;
  }


  template <typename InputIterator, typename T>
  size_t count(InputIterator first, InputIterator last, T const& value)
  {
    size_t ret = 0;
    while (first != last) 
      if (*first++ == value) 
        ++ret;
    return ret;
  }


  template <typename InputIterator, typename Predicate>
  size_t count_if(InputIterator first, InputIterator last, Predicate pred)
  {
    size_t ret = 0;
    while (first != last) 
      if (pred(*first++)) 
        ++ret;
    return ret;
  }


  template <typename InputIterator1, typename InputIterator2>
  pair<InputIterator1, InputIterator2> mismatch(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2)
  {
    while ( first1!=last1 )
    {
      if (*first1 != *first2)
        break;
      ++first1; 
      ++first2;
    }
    return make_pair(first1, first2);
  }


  template <typename InputIterator1, typename InputIterator2, typename Predicate>
  pair<InputIterator1, InputIterator2> mismatch(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, Predicate pred)
  {
    while ( first1!=last1 )
    {
      if (!pred(*first1, *first2))
        break;
      ++first1; 
      ++first2;
    }
    return make_pair(first1, first2);
  }


  template <typename ForwardIterator1, typename ForwardIterator2>
  ForwardIterator1 search(ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2, ForwardIterator2 last2)
  {
    ForwardIterator1 limit = first1; 
    advance(limit, 1 + distance(first1, last1) - distance(first2, last2));

    while (first1 != limit)
    {
      ForwardIterator1 it1 = first1; 
      ForwardIterator2 it2 = first2;
      while (*it1 == *it2)
      { 
        ++it1; 
        ++it2; 
        if (it2 == last2) 
          return first1; 
      }
      ++first1;
    }
    return last1;
  }


  template <typename ForwardIterator1, typename ForwardIterator2, typename Predicate>
  ForwardIterator1 search(ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2, ForwardIterator2 last2, Predicate pred)
  {
    ForwardIterator1 limit = first1; 
    advance(limit, 1 + distance(first1, last1) - distance(first2, last2));

    while (first1 != limit)
    {
      ForwardIterator1 it1 = first1; 
      ForwardIterator2 it2 = first2;
      while (pred(*it1, *it2))
      { 
        ++it1; 
        ++it2; 
        if (it2 == last2) 
          return first1; 
      }
      ++first1;
    }
    return last1;
  }


  template <typename ForwardIterator, typename Size, typename T>
  ForwardIterator search_n(ForwardIterator first, ForwardIterator last, Size count, T const& value )
  {
    ForwardIterator limit = first; 
    advance(limit, distance(first, last) - count);

    while (first != limit)
    {
      ForwardIterator it = first; 
      Size i = 0;
      while (*it == value)
      { 
        ++it; 
        if (++i == count)
          return first; 
      }
      ++first;
    }
    return last;
  }


  template <typename ForwardIterator, typename Size, typename T, typename Predicate>
  ForwardIterator search_n(ForwardIterator first, ForwardIterator last, Size count, T const& value, Predicate pred)
  {
    ForwardIterator limit = first; 
    advance(limit, distance(first, last) - count);

    while (first != limit)
    {
      ForwardIterator it = first; 
      Size i = 0;
      while (pred(*it, value))
      { 
        ++it; 
        if (++i == count)
          return first; 
      }
      ++first;
    }
    return last;
  }


  template <typename ForwardIterator1, typename ForwardIterator2>
  ForwardIterator2 swap_ranges(ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2)
  {
    while (first1 != last1)
      swap(*first1++, *first2++);
    return first2;
  }


  template <typename ForwardIterator1, typename ForwardIterator2>
  void iter_swap(ForwardIterator1 a, ForwardIterator2 b)
  {
    swap(*a, *b);
  }


  template <typename InputIterator, typename OutputIterator, typename UnaryOperator>
  OutputIterator transform(InputIterator first1, InputIterator last1, OutputIterator result, UnaryOperator op)
  {
    while (first1 != last1)
      *result++ = op(*first1++);
    return result;
  }


  template <typename InputIterator1, typename InputIterator2, typename OutputIterator, typename BinaryOperator>
  OutputIterator transform(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, OutputIterator result, BinaryOperator binary_op)
  {
    while (first1 != last1)
      *result++ = binary_op(*first1++, *first2++);
    return result;
  }


  template <typename ForwardIterator, typename T>
  void replace(ForwardIterator first, ForwardIterator last, T const& old_value, T const& new_value )
  {
    for (; first != last; ++first)
      if (*first == old_value) 
        *first = new_value;
  }


  template <typename ForwardIterator, typename Predicate, typename T>
  void replace_if(ForwardIterator first, ForwardIterator last, Predicate pred, T const& new_value)
  {
    for (; first != last; ++first)
      if (pred(*first)) 
        *first = new_value;
  }


  template <typename InputIterator, typename OutputIterator, typename T>
  OutputIterator replace_copy(InputIterator first, InputIterator last, OutputIterator result, T const& old_value, T const& new_value)
  {
    for (; first != last; ++first, ++result)
      *result = (*first == old_value) ? new_value : *first;
    return result;
  }


  template <typename InputIterator, typename OutputIterator, typename Predicate, typename T>
  OutputIterator replace_copy_if(InputIterator first, InputIterator last, OutputIterator result, Predicate pred, T const& new_value)
  {
    for (; first != last; ++first, ++result)
      *result = (pred(*first)) ? new_value : *first;
    return result;
  }


  template <typename ForwardIterator, typename Generator>
  void generate(ForwardIterator first, ForwardIterator last, Generator gen)
  {
    while (first != last)  
      *first++ = gen();
  }


  template <typename OutputIterator, typename Size, typename Generator>
  void generate(OutputIterator first, Size n, Generator gen)
  {
    for (; n>0; --n)  
      *first++ = gen();
  }


  template <typename ForwardIterator, typename T>
  ForwardIterator remove(ForwardIterator first, ForwardIterator last, T const& value)
  {
    ForwardIterator result = first;
    for (; first != last; ++first)
      if (!(*first == value))
        *result++ = *first;
    return result;
  }


  template <typename ForwardIterator, typename Predicate>
  ForwardIterator remove_if(ForwardIterator first, ForwardIterator last, Predicate pred)
  {
    ForwardIterator result = first;
    for (; first != last; ++first)
      if (!pred(*first)) 
        *result++ = *first;
    return result;
  }


  template <typename InputIterator, typename OutputIterator, typename T>
  OutputIterator remove_copy(InputIterator first, InputIterator last, OutputIterator result, T const& value)
  {
    for (; first != last; ++first)
      if (!(*first == value)) 
        *result++ = *first;
    return result;
  }


  template <typename InputIterator, typename OutputIterator, typename Predicate>
  OutputIterator remove_copy_if(InputIterator first, InputIterator last, OutputIterator result, Predicate pred)
  {
    for (; first != last; ++first)
      if (!pred(*first)) 
        *result++ = *first;
    return result;
  }


  template <typename ForwardIterator>
  ForwardIterator unique(ForwardIterator first, ForwardIterator last)
  {
    ForwardIterator result = first;
    while (++first != last)
    {
      if (!(*result == *first))
        *(++result) = *first;
    }
    return ++result;
  }


  template <typename ForwardIterator, typename Predicate>
  ForwardIterator unique(ForwardIterator first, ForwardIterator last, Predicate pred)
  {
    ForwardIterator result = first;
    while (++first != last)
    {
      if (!pred(*result, *first))
        *(++result) = *first;
    }
    return ++result;
  }


  template <typename InputIterator, typename OutputIterator>
  OutputIterator unique_copy(InputIterator first, InputIterator last, OutputIterator result)
  {
    *result = *first;
    while (++first != last)
    {
      if (!(*result == *first))
        *(++result) = *first;
    }
    return ++result;
  }


  template <typename InputIterator, typename OutputIterator, typename Predicate>
  OutputIterator unique_copy(InputIterator first, InputIterator last, OutputIterator result, Predicate pred)
  {
    *result = *first;
    while (++first != last)
    {
      if (!pred(*result, *first))
        *(++result) = *first;
    }
    return ++result;
  }


  template <typename BidirectionalIterator>
  void reverse(BidirectionalIterator first, BidirectionalIterator last)
  {
    while ((first != last) && (first != --last))
      swap(*first++, *last);
  }


  template <typename BidirectionalIterator, typename OutputIterator>
  OutputIterator reverse_copy(BidirectionalIterator first, BidirectionalIterator last, OutputIterator result)
  {
    while (first != last) 
      *result++ = *--last;
    return result;
  }


  template <typename ForwardIterator>
  void rotate(ForwardIterator first, ForwardIterator middle, ForwardIterator last)
  {
    ForwardIterator next = middle;
    while (first != next)
    {
      swap(*first++, *next++);
      if (next == last) 
        next = middle;
      else if (first == middle) 
        middle = next;
    }
  }


  template <typename ForwardIterator, typename OutputIterator>
  OutputIterator rotate_copy(ForwardIterator first, ForwardIterator middle, ForwardIterator last, OutputIterator result)
  {
    result = copy(middle, last, result);
    return copy(first, middle, result);
  }


  template <typename RandomAccessIterator, typename RandomNumberGenerator>
  void random_shuffle(RandomAccessIterator first, RandomAccessIterator last, RandomNumberGenerator& rand)
  {
    size_t i, n;
    n = (last-first);
    for (i=2; i<n; ++i)
      swap(first[i], first[rand(i)]);
  }

  /*
  template <typename RandomAccessIterator>
  void random_shuffle(RandomAccessIterator first, RandomAccessIterator last)
  {

  static struct Randomizer
  {
  Randomizer()
  {
  srandom(micros());
  }
  } randomizer;

  size_t i, n;
  n = (last-first);
  for (i=2; i<n; ++i)
  swap(first[i], first[ random() % i ]);
  }*/


  template <typename BidirectionalIterator, typename Predicate>
  BidirectionalIterator partition(BidirectionalIterator first, BidirectionalIterator last, Predicate pred)
  {
    while (true)
    {
      while (first != last && pred(*first)) 
        ++first;
      if (first == last--) 
        break;
      while (first != last && !pred(*last)) 
        --last;
      if (first == last) 
        break;
      swap(*first++, *last);
    }
    return first;
  }


  template <typename ForwardIterator, typename T>
  ForwardIterator lower_bound(ForwardIterator first, ForwardIterator last, T const& value)
  {
    size_t count = distance(first, last);
    while (count > 0)
    {
      ForwardIterator it = first;
      size_t const step = count / 2; 
      advance(it, step);
      if (*it < value)
      { 
        first = ++it; 
        count -= step + 1;
      }
      else 
        count = step;
    }
    return first;
  }


  template <typename ForwardIterator, typename T, typename Compare>
  ForwardIterator lower_bound(ForwardIterator first, ForwardIterator last, T const& value, Compare comp)
  {
    size_t count = distance(first, last);
    while (count > 0)
    {
      ForwardIterator it = first;
      size_t step = count / 2; 
      advance(it, step);
      if (comp(*it, value))
      { 
        first = ++it; 
        count -= step + 1;
      }
      else 
        count = step;
    }
    return first;
  }


  template <typename ForwardIterator, typename T>
  ForwardIterator upper_bound(ForwardIterator first, ForwardIterator last, T const& value)
  {
    size_t count = distance(first, last);
    while (count > 0)
    {
      ForwardIterator it = first; 
      size_t const step = count/2; 
      advance(it, step);
      if (!(value < *it))                 
      { 
        first = ++it; 
        count -= step + 1;
      }
      else 
        count = step;
    }
    return first;
  }


  template <typename ForwardIterator, typename T, typename Compare>
  ForwardIterator upper_bound(ForwardIterator first, ForwardIterator last, T const& value, Compare comp)
  {
    size_t count = distance(first, last);
    while (count > 0)
    {
      ForwardIterator it = first; 
      size_t const step = count/2; 
      advance(it, step);
      if (!comp(value, *it))
      { 
        first = ++it; 
        count -= step + 1;
      }
      else 
        count = step;
    }
    return first;
  }


  template <typename ForwardIterator, typename T>
  pair<ForwardIterator, ForwardIterator> equal_range(ForwardIterator first, ForwardIterator last, T const& value)
  {
    ForwardIterator it = lower_bound(first, last, value);
    return make_pair(it, upper_bound(it, last, value));
  }


  template <typename ForwardIterator, typename T, typename Compare>
  pair<ForwardIterator, ForwardIterator> equal_range(ForwardIterator first, ForwardIterator last, T const& value, Compare comp)
  {
    ForwardIterator it = lower_bound(first, last, value, comp);
    return make_pair(it, upper_bound(it, last, value, comp));
  }


  template <typename ForwardIterator, typename T>
  bool binary_search(ForwardIterator first, ForwardIterator last, T const& value)
  {
    first = lower_bound(first, last, value);
    return first != last && !(value < *first);
  }


  template <typename ForwardIterator, typename T, typename Compare>
  bool binary_search(ForwardIterator first, ForwardIterator last, T const& value, Compare comp)
  {
    first = lower_bound(first, last, value, comp);
    return first != last && !comp(value, *first);
  }


  template <typename InputIterator1, typename InputIterator2, typename OutputIterator>
  OutputIterator merge(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, OutputIterator result)
  {
    while (true) 
    {
      *result++ = (*first2 < *first1) ? *first2++ : *first1++;
      if (first1 == last1) return 
        copy(first2, last2, result);
      if (first2 == last2) return 
        copy(first1, last1, result);
    }
  }


  template <typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Compare>
  OutputIterator merge(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, OutputIterator result, Compare comp)
  {
    while (true) 
    {
      *result++ = comp(*first2, *first1) ? *first2++ : *first1++;
      if (first1 == last1) return 
        copy(first2, last2, result);
      if (first2 == last2) return 
        copy(first1, last1, result);
    }
  }


  template <typename InputIterator1, typename InputIterator2>
  bool includes(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2)
  {
    while (first1 != last1)
    {
      if (*first2 < *first1) 
        break;
      else if (*first1 < *first2) 
        ++first1;
      else 
      { 
        ++first1; 
        ++first2; 
      }
      if (first2 == last2) 
        return true;
    }
    return false;
  }


  template <typename InputIterator1, typename InputIterator2, typename Compare>
  bool includes(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, Compare comp)
  {
    while (first1 != last1)
    {
      if (comp(*first2, *first1))
        break;
      else if (comp(*first1, *first2))
        ++first1;
      else 
      { 
        ++first1; 
        ++first2; 
      }
      if (first2 == last2) 
        return true;
    }
    return false;
  }


  template <typename InputIterator1, typename InputIterator2, typename OutputIterator>
  OutputIterator set_union(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, OutputIterator result)
  {
    while (true)
    {
      if (*first1 < *first2) 
        *result++ = *first1++;
      else if (*first2 < *first1) 
        *result++ = *first2++;
      else 
      {
        *result++ = *first1++; 
        first2++; 
      }      
      if (first1 == last1) 
        return copy(first2, last2, result);
      if (first2 == last2) 
        return copy(first1, last1, result);
    }
  }


  template <typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Compare>
  OutputIterator set_union(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, OutputIterator result, Compare comp)
  {
    while (true)
    {
      if (comp(*first1, *first2))
        *result++ = *first1++;
      else if (comp(*first2, *first1))
        *result++ = *first2++;
      else 
      {
        *result++ = *first1++; 
        first2++; 
      }      
      if (first1 == last1) 
        return copy(first2, last2, result);
      if (first2 == last2) 
        return copy(first1, last1, result);
    }
  }


  template <typename InputIterator1, typename InputIterator2, typename OutputIterator>
  OutputIterator set_intersection(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, OutputIterator result)
  {
    while (first1 != last1 && first2 != last2)
    {
      if (*first1 < *first2) 
        ++first1;
      else if (*first2 < *first1) 
        ++first2;
      else 
      { 
        *result++ = *first1++; 
        first2++; 
      }
    }
    return result;
  }


  template <typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Compare>
  OutputIterator set_intersection(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, OutputIterator result, Compare comp)
  {
    while (first1 != last1 && first2 != last2)
    {
      if (comp(*first1, *first2))
        ++first1;
      else if (comp(*first2, *first1))
        ++first2;
      else 
      { 
        *result++ = *first1++; 
        first2++; 
      }
    }
    return result;
  }


  template <typename InputIterator1, typename InputIterator2, typename OutputIterator>
  OutputIterator set_difference(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, OutputIterator result)
  {
    while (first1 != last1 && first2 != last2)
    {
      if (*first1 < *first2) 
        *result++ = *first1++;
      else if (*first2 < *first1) 
        first2++;
      else 
      { 
        first1++; 
        first2++; 
      }      
    }
    return copy(first1, last1, result);
  }


  template <typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Compare>
  OutputIterator set_difference(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, OutputIterator result, Compare comp)
  {
    while (first1 != last1 && first2 != last2)
    {
      if (comp(*first1, *first2))
        *result++ = *first1++;
      else if (comp(*first2, *first1))
        first2++;
      else 
      { 
        first1++; 
        first2++; 
      }      
    }
    return copy(first1, last1, result);
  }


  template <typename InputIterator1, typename InputIterator2, typename OutputIterator>
  OutputIterator set_symmetric_difference(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, OutputIterator result)
  {
    while (true)
    {
      if (*first1 < *first2) 
        *result++ = *first1++;
      else if (*first2 < *first1)
        *result++ = *first2++;
      else 
      { 
        first1++; 
        first2++; 
      }      
      if (first1 == last1) 
        return copy(first2, last2, result);
      if (first2 == last2) 
        return copy(first1, last1, result);
    }
  }


  template <typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Compare>
  OutputIterator set_symmetric_difference(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, OutputIterator result, Compare comp)
  {
    while (true)
    {
      if (comp(*first1, *first2))
        *result++ = *first1++;
      else if (comp(*first2, *first1))
        *result++ = *first2++;
      else 
      { 
        first1++; 
        first2++; 
      }      
      if (first1 == last1) 
        return copy(first2, last2, result);
      if (first2 == last2) 
        return copy(first1, last1, result);
    }
  }


  template <typename ForwardIterator>
  ForwardIterator min_element(ForwardIterator first, ForwardIterator last)
  {
    ForwardIterator lowest = first;
    if (first == last) 
      return last;
    while (++first != last)
      if (*first < *lowest)
        lowest = first;
    return lowest;
  }


  template <typename ForwardIterator, typename Compare>
  ForwardIterator min_element(ForwardIterator first, ForwardIterator last, Compare comp)
  {
    ForwardIterator lowest = first;
    if (first == last) 
      return last;
    while (++first != last)
      if (comp(*first, *lowest))
        lowest = first;
    return lowest;
  }


  template <typename ForwardIterator>
  ForwardIterator max_element(ForwardIterator first, ForwardIterator last)
  {
    ForwardIterator largest = first;
    if (first == last) 
      return last;
    while (++first != last)
      if (*largest < *first)
        largest = first;
    return largest;
  }


  template <typename ForwardIterator, typename Compare>
  ForwardIterator max_element(ForwardIterator first, ForwardIterator last, Compare comp)
  {
    ForwardIterator largest = first;
    if (first == last) 
      return last;
    while (++first != last)
      if (comp(*largest, *first))
        largest = first;
    return largest;
  }





}  // end of namspace fdv

#endif // FDV_ALGORITHM_H


