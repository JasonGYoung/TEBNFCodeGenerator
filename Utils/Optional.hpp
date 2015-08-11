/**
 *  Defines optional and none types.
 *  Heavily influenced by boost::optional and boost::none.
 *
 * @author  Jason Young
 * @version 0.1
 */

#ifndef OPTIONAL_HPP
#define OPTIONAL_HPP

namespace Utils
{

  /** The none type.  Makes it possible for optionals to be set to an
   *  uninitialized value using the = operator.
   */
  namespace { struct none_type{}; }
  typedef int none_type::*none_t;
  none_t const none = static_cast<none_t>(0);

  /** Defines an optional type.
   */
  template<typename T>
  class optional
  {
  public:
    optional() : m_val(), m_isInitialized(false) {}
    optional(const T& val) : m_val(val), m_isInitialized(true) {}
    optional(const optional<T>& val) : m_val(val.get()), m_isInitialized(true) {}
    optional(none_t n) : m_val(), m_isInitialized(false) {}
    T& get() { return m_val; }
    const T& get() const { return m_val; }
    bool is_initialized() const { return m_isInitialized; }
    optional& operator=(const optional& rhs)
    {
      if(this != &rhs)
      {
        m_val = rhs.get();
        m_isInitialized = true;
      }
      return *this;
    }
    optional& operator=(const T& rhs)
    {
      if(&m_val != &rhs)
      {
        m_val = rhs;
        m_isInitialized = true;
      }
      return *this;
    }
    optional& operator=(none_t n)
    {
      reset();
      return *this;
    }
    T* operator->() { return get(); }
    T& operator*() { return m_val; }
    const T& operator*() const { return m_val; }
    operator bool() const { return m_isInitialized; }
    void reset(const optional& rhs) { *this = rhs; }
    void reset(const T& rhs) { *this = rhs; }
    void reset() { m_val = ~T(); m_isInitialized = false; }
  private:
    T m_val;
    bool m_isInitialized;
  }; //class optional

} // end namespace Utils

#endif //OPTIONAL_HPP
