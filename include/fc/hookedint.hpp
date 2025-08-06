#pragma once
#ifndef include_guard_hookedint
#define include_guard_hookedint

/**
 * @file HookedInt is a wrapper (template-class) that wraps an integer type (now supports especially int128) to aid debugging by informing when value changes (for some cases)
 * @todo make sure it supports other integral types (was tested on __uint128_t) test in general fc::uint128_t (on platforms without __uint128_t)
 * @todo support other operators - only some of them call log properly, this code is not finished
 */

/// mostly vibecoded

#include <fc/reflect/typename.hpp>
#include <iostream>


// to print the integer:
#include <string>
#include <cstdint>
#include <array>
#include <sstream>
#include <iomanip>

void HookedInt_trigger(int level);

namespace detail {
	std::string uint128_to_string(__uint128_t value);

	template <typename T> std::string almost_any_int_to_str(T the_int) {
		std::ostringstream oss;
		oss<<the_int;
		return oss.str();
	}
	template <> std::string almost_any_int_to_str(__uint128_t the_int)
		;
}

template<typename T>
class HookedInt {
private:
    T value;

    mutable T value_old;
    mutable bool value_old_isset;

    std::ostream& log_startline() const {
	    std::cout << "(HookedInt)(ptr=" << (static_cast<const void*>(this)) << ")"
		<< " (value="<<detail::almost_any_int_to_str(value)<<")"
		<< " (old="<< ( value_old ? detail::almost_any_int_to_str(value) : "none" )<<")";
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

    operator T&() { log_startline()<<"Access r/w\n"; return value; }
    operator const T&() const { log_startline()<<"read."; return value; }

    T& get() { log_startline()<<"Get r/w\n"; return value; }
    const T& get() const { log_startline()<<"Get readonly\n"; return value; }

    T& unwrap() { return get(); }
    const T& unwrap() const { return get(); }

    // Transparent pointer access
    T* operator->() { return &get(); }
    const T* operator->() const { return &get(); }


    // Arithmetic
    HookedInt operator+(const HookedInt& other) const { return value + other.value; }
    HookedInt operator-(const HookedInt& other) const { return value - other.value; }
    HookedInt operator*(const HookedInt& other) const { return value * other.value; }
    HookedInt operator/(const HookedInt& other) const { return value / other.value; }

    // Modify
    HookedInt& operator+=(const HookedInt& other) { value += other.value; log_startline()<<"op+= called"; return *this; }
    HookedInt& operator-=(const HookedInt& other) { value -= other.value; log_startline()<<"op-= called"; return *this; }
    HookedInt& operator*=(const HookedInt& other) { value *= other.value; log_startline()<<"op*= called";  return *this; }
    HookedInt& operator/=(const HookedInt& other) { value /= other.value; log_startline()<<"op/= called";  return *this; }

    // Comparison
    bool operator==(const HookedInt& other) const { return value == other.value; }
    bool operator!=(const HookedInt& other) const { return value != other.value; }
    bool operator<(const HookedInt& other)  const { return value < other.value; }
    bool operator<=(const HookedInt& other) const { return value <= other.value; }
    bool operator>(const HookedInt& other)  const { return value > other.value; }
    bool operator>=(const HookedInt& other) const { return value >= other.value; }

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

template<>
struct get_typename<HookedInt<__uint128_t>> {
    static const char* name() { return "HookedInt<__uint128_t>"; }
};

}


#endif
