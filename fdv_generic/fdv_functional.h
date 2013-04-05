// 2010 by Fabrizio Di Vittorio (fdivitto@tiscali.it)

#ifndef FDV_FUNCTIONAL_H
#define FDV_FUNCTIONAL_H

namespace fdv
{


  template <typename Arg, typename Result>
  struct unary_function
  {
    typedef Arg argument_type;
    typedef Result result_type;
  };
  
  
  template <typename Arg1, typename Arg2, typename Result>
  struct binary_function
  {
    typedef Arg1 first_argument_type;
    typedef Arg2 second_argument_type;
    typedef Result result_type;
  };
  
  
  template <typename T>
  struct multiplies : binary_function<T, T, T>
  {
    T operator() (T const& x, T const& y) const
    {
      return x * y;
    }
  };
  
  
  template <typename T>
  struct divides : binary_function<T, T, T>
  {
    T operator() (T const& x, T const& y) const
    {
      return x / y;
    }
  };
  
  
  template <typename T>
  struct modulus : binary_function<T, T, T> 
  {
    T operator() (T const& x, T const& y) const
    {
      return x % y;
    }
  };
  
  
  template <typename T>
  struct negate : unary_function<T, T> 
  {
    T operator() (T const& x) const
    {
      return -x;
    }
  };
  
  
  template <typename T>
  struct plus : binary_function<T, T, T> 
  {
    T operator() (T const& x, T const& y) const
    {
      return x + y;
    }
  };
  
  
  template <typename T>
  struct minus : binary_function<T, T, T> 
  {
    T operator() (T const& x, T const& y) const
    {
      return x - y;
    }
  };
  
  
  template <typename T> 
  struct equal_to : binary_function<T, T, bool> 
  {
    bool operator() (T const& x, T const& y) const
    {
      return x == y;
    }
  };
  
  
  template <typename T>
  struct not_equal_to : binary_function<T, T, bool>
  {
    bool operator() (T const& x, T const& y) const
    {
      return x != y;
    }
  };
  
  
  template <typename T>
  struct greater : binary_function<T, T, bool>
  {
    bool operator() (T const& x, T const& y) const
    {
      return x > y;
    }
  };
  
  
  template <typename T>
  struct less : binary_function<T, T, bool>
  {
    bool operator() (T const& x, T const& y) const
    {
      return x < y;
    }
  };
  
  
  template <typename T>
  struct greater_equal : binary_function<T, T, bool>
  {
    bool operator() (T const& x, T const& y) const
    {
      return x >= y;
    }
  };
  
  
  template <typename T>
  struct less_equal : binary_function<T, T, bool>
  {
    bool operator() (T const& x, T const& y) const
    {
      return x <= y;
    }
  };
  
  
  template <typename T>
  struct logical_and : binary_function<T, T, bool>
  {
    bool operator() (T const& x, T const& y) const
    {
      return x && y;
    }
  };
  
  
  template <typename T>
  struct logical_or : binary_function<T, T, bool>
  {
    bool operator() (T const& x, T const& y) const
    {
      return x || y;
    }
  };
  
  
  template <typename T>
  struct logical_not : unary_function<T, bool>
  {
    bool operator() (T const& x) const
    {
      return !x;
    }
  };
  
  
  template <typename Predicate>
  class unary_negate : public unary_function<typename Predicate::argument_type, bool>
  {
  protected:
    Predicate fn;
  public:
    explicit unary_negate(Predicate const& pred)
    : fn(pred) 
    {}
    
    bool operator() (typename Predicate::argument_type const& x) const
    {
      return !fn(x); 
    }
  };
  
  
  template <typename Predicate>
  class binary_negate : public binary_function<typename Predicate::first_argument_type, typename Predicate::second_argument_type, bool>
  {
  protected:
    Predicate fn;
  public:
    explicit binary_negate(Predicate const& pred) 
    : fn (pred)
    {}
    
    bool operator() (typename Predicate::first_argument_type const& x, typename Predicate::second_argument_type const& y) const
    { 
      return !fn(x, y); 
    }
  };  
  
  
  template <typename Predicate>
  unary_negate<Predicate> not1(Predicate const& pred)
  {
    return unary_negate<Predicate>(pred);
  }
  
  
  template <typename Predicate>
  binary_negate<Predicate> not2(Predicate const& pred)
  {
    return binary_negate<Predicate>(pred);
  }
  
  
  template <typename Operation>
  class binder1st : public unary_function<typename Operation::second_argument_type, typename Operation::result_type>
  {
  protected:
    Operation op;
    typename Operation::first_argument_type value;
  public:
    binder1st(Operation const& x, typename Operation::first_argument_type const& y)
    : op (x), value(y)
    {}
    
    typename Operation::result_type operator() (typename Operation::second_argument_type const& x) const
    { 
      return op(value, x); 
    }
  };
  
  
  template <typename Operation, class T>
  binder1st<Operation> bind1st(Operation const& op, T const& x)
  {
    return binder1st<Operation>(op, typename Operation::first_argument_type(x));
  }
  
  
  template <typename Operation>
  class binder2nd : public unary_function <typename Operation::first_argument_type, typename Operation::result_type>
  {
  protected:
    Operation op;
    typename Operation::second_argument_type value;
  public:
    binder2nd (Operation const& x, typename Operation::second_argument_type const& y)
    : op (x), value(y) 
    {}
    
    typename Operation::result_type operator() (typename Operation::first_argument_type const& x) const
    { 
      return op(x, value); 
    }
  };  
  
  
  template <typename Operation, typename T>
  binder2nd<Operation> bind2nd(Operation const& op, T const& x)
  {
    return binder2nd<Operation>(op, typename Operation::second_argument_type(x));
  }

  
}

#endif // FDV_FUNCTIONAL_H

