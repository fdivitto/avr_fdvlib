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



#ifndef FDV_ITERATOR_H
#define FDV_ITERATOR_H

namespace fdv
{

  struct input_iterator_tag {};
  struct output_iterator_tag {};
  struct forward_iterator_tag: public input_iterator_tag {}; 
  struct bidirectional_iterator_tag: public forward_iterator_tag {}; 
  struct random_access_iterator_tag: public bidirectional_iterator_tag {};


  template <typename Category, typename T, typename Distance = size_t, typename Pointer = T*, typename Reference = T&>
  struct iterator 
  {
    typedef T         value_type;
    typedef Distance  difference_type;
    typedef Pointer   pointer;
    typedef Reference reference;
    typedef Category  iterator_category;
  };


  template <typename Iterator> 
  struct iterator_traits 
  { 
    typedef typename Iterator::difference_type difference_type; 
    typedef typename Iterator::value_type value_type; 
    typedef typename Iterator::pointer pointer; 
    typedef typename Iterator::reference reference; 
    typedef typename Iterator::iterator_category iterator_category;
  };


  template <typename T>
  struct iterator_traits<T*>
  { 
    typedef size_t difference_type; 
    typedef T value_type; 
    typedef T* pointer;
    typedef T& reference;
    typedef random_access_iterator_tag iterator_category; 
  };


  template <typename T> 
  struct iterator_traits<T const*> 
  { 
    typedef size_t difference_type; 
    typedef T value_type; 
    typedef T const* pointer;
    typedef T const& reference;
    typedef random_access_iterator_tag iterator_category; 
  };  


  template <typename InputIterator, typename Distance>
  void advance(InputIterator& i, Distance n)
  {
    i += n;
  }


  template <typename InputIterator>
  typename iterator_traits<InputIterator>::difference_type distance(InputIterator first, InputIterator last)
  {
    return last - first;
  }



  /*
  template <typename Iterator>
  class reverse_iterator : public iterator<typename iterator_traits<Iterator>::iterator_category, 
  typename iterator_traits<Iterator>::value_type, 
  typename iterator_traits<Iterator>::difference_type, 
  typename iterator_traits<Iterator>::pointer,
  typename iterator_traits<Iterator>::reference>
  {

  protected:

  Iterator current;

  public:

  typedef Iterator                                            iterator_type; 
  typedef typename iterator_traits<Iterator>::difference_type difference_type; 
  typedef typename iterator_traits<Iterator>::reference       reference; 
  typedef typename iterator_traits<Iterator>::pointer         pointer;


  reverse_iterator() {}

  explicit reverse_iterator(iterator_type x) 
  : current(x) 
  {
  }

  template <typename U>
  reverse_iterator(reverse_iterator<U> const& x)
  : current(x.base()) 
  {
  }

  iterator_type base() const 
  { 
  return current; 
  }

  reference operator*() const 
  {
  Iterator tmp(current);
  return *--tmp;
  }

  pointer operator->() const 
  { 
  return &(operator*()); 
  }

  reverse_iterator<Iterator>& operator++() 
  {
  --current;
  return *this;
  }

  reverse_iterator<Iterator> operator++(int) 
  {
  reverse_iterator<Iterator> tmp(*this);
  --current;
  return tmp;
  }

  reverse_iterator<Iterator>& operator--() 
  {
  ++current;
  return *this;
  }

  reverse_iterator<Iterator> operator--(int) 
  {
  reverse_iterator<Iterator> tmp(*this);
  ++current;
  return tmp;
  }

  reverse_iterator<Iterator> operator+(difference_type n) const 
  {
  return reverse_iterator<Iterator>(current - n);
  }

  reverse_iterator<Iterator>& operator+=(difference_type n) 
  {
  current -= n;
  return *this;
  }

  reverse_iterator<Iterator> operator-(difference_type n) const 
  {
  return reverse_iterator<Iterator>(current + n);
  }

  reverse_iterator<Iterator>& operator-=(difference_type n) 
  {
  current += n;
  return *this;
  }

  reference operator[](difference_type n) const 
  { 
  return *(*this + n); 
  }  
  }; 


  template <typename Iterator>
  inline bool operator==(reverse_iterator<Iterator> const& x, 
  reverse_iterator<Iterator> const& y) 
  {
  return x.base() == y.base();
  }


  template <typename Iterator>
  inline bool operator<(reverse_iterator<Iterator> const& x, 
  reverse_iterator<Iterator> const& y) 
  {
  return y.base() < x.base();
  }


  template <typename Iterator>
  inline bool operator!=(reverse_iterator<Iterator> const& x, 
  reverse_iterator<Iterator> const& y) 
  {
  return !(x == y);
  }


  template <typename Iterator>
  inline bool operator>(reverse_iterator<Iterator> const& x, 
  reverse_iterator<Iterator> const& y) 
  {
  return y < x;
  }


  template <typename Iterator>
  inline bool operator<=(reverse_iterator<Iterator> const& x, 
  reverse_iterator<Iterator> const& y) 
  {
  return !(y < x);
  }


  template <typename Iterator>
  inline bool operator>=(reverse_iterator<Iterator> const& x, 
  reverse_iterator<Iterator> const& y) 
  {
  return !(x < y);
  }


  template <typename Iterator>
  inline typename reverse_iterator<Iterator>::difference_type operator-(reverse_iterator<Iterator> const& x, reverse_iterator<Iterator> const& y)
  {
  return y.base() - x.base();
  }


  template <typename Iterator>
  inline reverse_iterator<Iterator> operator+(typename reverse_iterator<Iterator>::difference_type n, reverse_iterator<Iterator> const& x) 
  {
  return reverse_iterator<Iterator>(x.base() - n);
  }
  */


};


#endif	// FDV_ITERATOR_H


