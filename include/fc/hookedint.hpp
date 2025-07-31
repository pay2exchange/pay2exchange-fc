#pragma once
#ifndef include_guard_hookedint
#define include_guard_hookedint

#include <iostream>


void HookedInt_trigger(int level);

template<typename T>
class HookedInt {
	/*
    static_assert(std::is_arithmetic<T>::value ||
                  std::is_same<T, __uint128_t>::value ||
                  std::is_same<T, boost::multiprecision::uint128_t>::value,
                  "HookedInt<T> should wrap an integer-like type");
*/
private:
    T value;

    mutable T value_old;
    mutable bool value_old_isset;

    std::ostream& log_startline() const {
	    std::cout << "(HookedInt)(ptr=" << (static_cast<const void*>(this)) << ") ";
	    return std::cout;
    }

    void on_construct() const { log_startline() << " Constructor\n"; HookedInt_trigger(2); }
    void on_destruct() const { log_startline() << " Destructor\n"; HookedInt_trigger(2); }

public:
    // Constructors and Destructor
    HookedInt() : value() { on_construct(); }
    HookedInt(const T& val) : value(val) { on_construct(); }
    HookedInt(const HookedInt& other) : value(other.value) { on_construct(); }
    HookedInt(HookedInt&& other) noexcept : value(std::move(other.value)) { on_construct(); }
    ~HookedInt() { on_destruct(); }

    // Assignment operators
    HookedInt& operator=(const HookedInt& other) { value = other.value; return *this; }
    HookedInt& operator=(HookedInt&& other) noexcept { value = std::move(other.value); return *this; }

    // Implicit conversion
    operator T&() { return value; }
    operator const T&() const { return value; }

    // Arithmetic
    HookedInt operator+(const HookedInt& other) const { return value + other.value; }
    HookedInt operator-(const HookedInt& other) const { return value - other.value; }
    HookedInt operator*(const HookedInt& other) const { return value * other.value; }
    HookedInt operator/(const HookedInt& other) const { return value / other.value; }

    HookedInt& operator+=(const HookedInt& other) { value += other.value; return *this; }
    HookedInt& operator-=(const HookedInt& other) { value -= other.value; return *this; }
    HookedInt& operator*=(const HookedInt& other) { value *= other.value; return *this; }
    HookedInt& operator/=(const HookedInt& other) { value /= other.value; return *this; }

    // Comparison
    bool operator==(const HookedInt& other) const { return value == other.value; }
    bool operator!=(const HookedInt& other) const { return value != other.value; }
    bool operator<(const HookedInt& other)  const { return value < other.value; }
    bool operator<=(const HookedInt& other) const { return value <= other.value; }
    bool operator>(const HookedInt& other)  const { return value > other.value; }
    bool operator>=(const HookedInt& other) const { return value >= other.value; }

    // Access underlying value
    T& get() { return value; }
    const T& get() const { return value; }

    // Transparent pointer access
    T* operator->() { return &value; }
    const T* operator->() const { return &value; }

    T& unwrap() { return value; }
    const T& unwrap() const { return value; }

    void debug_instrument() const {
	    if (value_old_isset) {
		    log_startline() << " first access.\n";
		    HookedInt_trigger(2);
	} else {
		if (value  != value_old) {
			log_startline() << "*** VALUE CHANGED *** \n";
			HookedInt_trigger(3);
		}
		else {
		value_old = value;
		    log_startline() << " access.\n";
		    HookedInt_trigger(1);
		}
	}
    }

};


namespace fc {
namespace raw {

template<typename Stream, typename T>
inline void pack(Stream& s, const HookedInt<T>& v, uint32_t _depth = 0) {
    pack(s, v.unwrap(), _depth);  // Forward to wrapped value
}

template<typename Stream, typename T>
inline void unpack(Stream& s, HookedInt<T>& v, uint32_t _depth = 0) {
    T inner;
    unpack(s, inner, _depth);
    v = HookedInt<T>(inner);  // Assign or construct wrapped value
}

} // namespace raw
} // namespace fc


#endif
