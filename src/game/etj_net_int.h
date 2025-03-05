/*
 * MIT License
 *
 * Copyright (c) 2025 ETJump team <zero@etjump.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <cassert>
#include <cstdint>

// integers that occupy 4 bytes of space in memory,
// but may only represent a certain amount of bits as a value
// there are used in entityState_t and playerState_t to enforce correct
// maximum data sizes to match the actual transmitted sizes,
// while keeping the struct fields at correct sizes

#define ASSERT_UNSIGNED(x) assert(inRange(x, 0, max))
#define ASSERT_SIGNED(x) assert(inRange(x, min, max))

inline bool inRange(const int val, const int min, const int max) {
  return val >= min && val <= max;
}

template <uint32_t bits>
class net_uint_t {
  static_assert(bits < 32, "net_uint_t must be less than 32 bits.");

  // underlying storage type is always an uint32_t
  uint32_t value : bits;
  static constexpr int32_t max = (1U << bits) - 1;

public:
  net_uint_t() : value(0) {}
  explicit net_uint_t(const int val) { value = val; }

  // we want implicit int conversions for these,
  // so we can pass these to functions that take in an int
  // NOLINTNEXTLINE(google-explicit-constructor)
  operator int() const {
    return static_cast<int>(value);
  }

  // template so we can do enum value conversions
  template <typename T>
  bool operator==(const T val) const {
    return value == val;
  }

  explicit operator bool() const {
    return value != 0;
  }

  net_uint_t &operator=(const int val) {
    ASSERT_UNSIGNED(val);
    value = val;
    return *this;
  }

  net_uint_t &operator+=(const int val) {
    ASSERT_UNSIGNED(value + val);
    value = value + val;
    return *this;
  }

  net_uint_t &operator-=(const int val) {
    ASSERT_UNSIGNED(value - val);
    value = value - val;
    return *this;
  }

  net_uint_t &operator*=(const int val) {
    ASSERT_UNSIGNED(value * val);
    value = value * val;
    return *this;
  }

  net_uint_t &operator/=(const int val) {
    ASSERT_UNSIGNED(value / val);
    value = value / val;
    return *this;
  }

  // pre-increment
  net_uint_t &operator++() {
    ASSERT_UNSIGNED(value + 1);
    value = value + 1;
    return *this;
  }

  // post-increment
  net_uint_t operator++(int) {
    net_uint_t ret = *this;
    operator++();
    return ret;
  }

  // pre-decrement
  net_uint_t &operator--() {
    ASSERT_UNSIGNED(value - 1);
    value = value - 1;
    return *this;
  }

  // post-decrement
  net_uint_t operator--(int) {
    net_uint_t ret = *this;
    operator--();
    return ret;
  }

  template <typename T>
  bool operator<(const T val) const {
    return value < val;
  }

  template <typename T>
  bool operator<=(const T val) const {
    return value <= val;
  }

  template <typename T>
  bool operator>(const T val) const {
    return value > val;
  }

  template <typename T>
  bool operator>=(const T val) const {
    return value >= val;
  }

  // Bitwise operators
  template <typename T>
  net_uint_t operator&(const T other) const {
    return static_cast<net_uint_t>(value & other);
  }

  net_uint_t &operator&=(const int val) {
    value &= val;
    return *this;
  }

  net_uint_t &operator|=(const int val) {
    ASSERT_UNSIGNED(value | val);
    value |= val;
    return *this;
  }

  net_uint_t &operator^=(const int val) {
    ASSERT_UNSIGNED(value ^ val);
    value ^= val;
    return *this;
  }

  // explicit conversion to enum type
  template <typename EnumType>
  EnumType toEnum() const {
    ASSERT_UNSIGNED(static_cast<EnumType>(value));
    return static_cast<EnumType>(value);
  }

  // getter for raw value
  int get() const { return value; }

  int getMax() const {
    return max;
  }

  template <typename T>
  void wrap(const T val) {
    value = static_cast<int>(val) % (max + 1);
  }
};


template <uint32_t bits>
class net_int_t {
  static_assert(bits < 32, "net_int_t must be less than 32 bits.");

  // underlying storage type is always an uint32_t
  int32_t value : bits;
  static constexpr int32_t min = -(1 << (bits - 1));
  static constexpr int32_t max = (1 << (bits - 1)) - 1;

public:
  net_int_t() : value(0) {}
  explicit net_int_t(const int val) { value = val; }

  // we want implicit int conversions for these,
  // so we can pass these to functions that take in an int
  // NOLINTNEXTLINE(google-explicit-constructor)
  operator int() const {
    return value;
  }

  // template so we can do enum value conversions
  template <typename T>
  bool operator==(const T val) const {
    return value == val;
  }

  explicit operator bool() const {
    return value != 0;
  }

  net_int_t &operator=(const int val) {
    ASSERT_SIGNED(val);
    value = val;
    return *this;
  }

  net_int_t &operator+=(const int val) {
    ASSERT_SIGNED(value + val);
    value = value + val;
    return *this;
  }

  net_int_t &operator-=(const int val) {
    ASSERT_SIGNED(value - val);
    value = value - val;
    return *this;
  }

  net_int_t &operator*=(const int val) {
    ASSERT_SIGNED(value * val);
    value = value * val;
    return *this;
  }

  net_int_t &operator/=(const int val) {
    ASSERT_SIGNED(value / val);
    value = value / val;
    return *this;
  }

  // pre-increment
  net_int_t &operator++() {
    ASSERT_SIGNED(value + 1);
    value = value + 1;
    return *this;
  }

  // post-increment
  net_int_t operator++(int) {
    net_int_t ret = *this;
    operator++();
    return ret;
  }

  // pre-decrement
  net_int_t &operator--() {
    ASSERT_SIGNED(value - 1);
    value = value - 1;
    return *this;
  }

  // post-decrement
  net_int_t operator--(int) {
    net_int_t ret = *this;
    operator--();
    return ret;
  }

  template <typename T>
  bool operator<(const T val) const {
    return value < val;
  }

  template <typename T>
  bool operator<=(const T val) const {
    return value <= val;
  }

  template <typename T>
  bool operator>(const T val) const {
    return value > val;
  }

  template <typename T>
  bool operator>=(const T val) const {
    return value >= val;
  }

  // Bitwise operators
  template <typename T>
  net_int_t operator&(const T other) const {
    return static_cast<net_int_t>(value & other);
  }

  net_int_t &operator&=(const int val) {
    value &= val;
    return *this;
  }

  net_int_t &operator|=(const int val) {
    ASSERT_UNSIGNED(value | val);
    value |= val;
    return *this;
  }

  net_int_t &operator^=(const int val) {
    ASSERT_UNSIGNED(value ^ val);
    value ^= val;
    return *this;
  }

  // explicit conversion to enum type
  template <typename EnumType>
  EnumType toEnum() const {
    ASSERT_UNSIGNED(static_cast<EnumType>(value));
    return static_cast<EnumType>(value);
  }

  // getter for raw value
  int get() const { return value; }

  int getMin() const{
    return min;
  }

  int getMax() const {
    return max;
  }
};
