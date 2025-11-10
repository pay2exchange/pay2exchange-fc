#pragma once

#include <memory>
#include <stdexcept>
#include <utility>

namespace fc {

namespace stdcomp { // TODO move to library  stdcomp ... 

template <typename T>
class optional {
    std::unique_ptr<T> m_value; // renamed from value_

public:
    optional() noexcept = default;

    optional(const T& v) : m_value(new T(v)) {}

    optional(T&& v) : m_value(new T(std::move(v))) {}

    optional(const optional& other)
        : m_value(other.m_value ? std::make_unique<T>(*other.m_value) : nullptr) {}

    optional(optional&& other) noexcept = default;

    optional& operator=(const optional& other) {
        if (other.m_value)
            m_value = std::make_unique<T>(*other.m_value);
        else
            m_value.reset();
        return *this;
    }

    optional& operator=(optional&& other) noexcept = default;

    bool has_value() const noexcept { return static_cast<bool>(m_value); }

    T& value() {
        if (!m_value) throw std::logic_error("bad optional access");
        return *m_value;
    }

    const T& value() const {
        if (!m_value) throw std::logic_error("bad optional access");
        return *m_value;
    }
};

} // namespace stdcomp

} // namespace fc
