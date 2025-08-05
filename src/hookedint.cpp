
#include <fc/hookedint.hpp>
#include <fc/popcount.hpp>

void trigger1() {
}

void HookedInt_trigger(int level) {
	trigger1();
}

namespace detail {

	template <> std::string almost_any_int_to_str(__uint128_t the_int) {
		return uint128_to_string(the_int);
	}

	std::string uint128_to_string(const __uint128_t value) {
	    if (value == 0) return "0"; // fast path

	    __uint128_t now = value;

	    const __uint128_t divisor = 1000000000000000000ULL; // 10^18
	    std::array<uint64_t, 3> parts = {0};

	    size_t i = 0;
	    while (now > 0) {
		__uint128_t q = now / divisor;
		__uint128_t r = now % divisor;

		if (!( i < (sizeof(parts)/sizeof(parts[0])))) { std::cerr<<"ERROR assert failed at " << __FILE__ << ":" << __LINE__ << "\n";  std::abort(); } // assert
		parts[i++] = static_cast<uint64_t>(r);
		now = q;
	    }

	    std::ostringstream oss;
	    if (i > 0) {
		oss << parts[--i]; // first (most significant) part: no padding
		while (i > 0) {
		    --i;
		    oss << std::setw(18) << std::setfill('0') << parts[i];
		}
	    }

	    oss << "{popcount: " << (static_cast<int>(fc::popcount(value))) << "}" ;
	    return oss.str();
	}

}

