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

#include <string>
#include <sstream>
#include <vector>
#include <fmt/printf.h>

namespace ETJump {
std::string hash(const std::string &input);
std::string getBestMatch(const std::vector<std::string> &words,
                         const std::string &current);
std::string sanitize(const std::string &text, bool toLower = false,
                     bool removeControlChars = true);
// returns the value if it's specified, else the default value
std::string getValue(const char *value, const std::string &defaultValue = "");
std::string getValue(const std::string &value,
                     const std::string &defaultValue = "");

template <typename... Targs>
std::string stringFormat(const std::string &format, const Targs &...Fargs) {
  try {
    return fmt::sprintf(format, Fargs...);
  } catch (const fmt::v10::format_error &e) {
#ifdef _DEBUG
    return "^3" + std::string(__func__) + ": ^7" + e.what() + "\n";
#else
    return "^3" + std::string(__func__) + ": ^7" + e.what() +
           ", please report this error to the developers.\n";
#endif
  }
}

std::string trimStart(const std::string &input);
std::string trimEnd(const std::string &input);
std::string trim(const std::string &input);

std::vector<std::string> wrapWords(const std::string &input, char separator,
                                   size_t maxLength);

template <typename T>
std::string getPluralizedString(const T &val, const std::string &str) {
  return std::to_string(val) + " " + str + (val == 1 ? "" : "s");
}

std::string getSecondsString(const int &seconds);
std::string getMinutesString(const int &minutes);
std::string getHoursString(const int &hours);
std::string getDaysString(const int &days);
std::string getWeeksString(const int &weeks);
std::string getMonthsString(const int &months);
std::string getYearsString(const int &years);

namespace StringUtil {
std::string toLowerCase(const std::string &input);
std::string toUpperCase(const std::string &input);
std::string eraseLast(const std::string &input, const std::string &substring);
template <typename T>
std::string join(const T &v, const std::string &delim) {
  std::ostringstream s;
  for (const auto &i : v) {
    if (&i != &v[0]) {
      s << delim;
    }
    s << i;
  }
  return s.str();
}
std::vector<std::string> split(const std::string &input,
                               const std::string &delimiter);
void replaceAll(std::string &input, const std::string &from,
                const std::string &to);

// replaces all occurrences of specified character
// and the following N characters in the string
// substitution will "eat" the substitution character, so it does need to
// be accounted for when defining how many characters to replace
void stringSubstitute(std::string &input, char character,
                      const std::string &replacement, size_t numChars);

bool startsWith(const std::string &str, const std::string &prefix);
bool endsWith(const std::string &str, const std::string &suffix);

template <typename T>
bool contains(const std::string &str, const T &text) {
  return str.find(text) != std::string::npos;
}

// case-insensitive string comparison, optionally with sanitized strings
bool iEqual(const std::string &str1, const std::string &str2,
            bool sanitized = false);
// Counts the extra padding needed when using format specifiers like
// %-20s with text that contains ET color codes
unsigned countExtraPadding(const std::string &input);

// removes any leading and trailing zeroes from a number
// always returns at least 0 even if there are no significant numbers
// e.g. 0000.0000 returns 0
// this expects a valid integer/floating point string as an input
// if input is invalid, returns an empty string
std::string normalizeNumberString(const std::string &input);

// if input contains only chars to remove, result is empty string
void removeTrailingChars(std::string &str, char charToRemove);

// if input contains only chars to remove, result is empty string
void removeLeadingChars(std::string &str, char charToRemove);

// true if the character in the given index is a color string
// std::string version of Q_IsColorString macro from q_shared.h
bool isColorString(const std::string &str, size_t idx);

// truncates a string, preserving any color codes in the string
// color codes are excluded from the truncation length, so this can be used
// to truncate colored strings to display only a certain number of characters
// e.g. truncate("^1foo^2bar", 3) -> "^1foo"
std::string truncate(const std::string &str, size_t len);

// sorts strings either with case sensitivity or insensitivity
// identical strings are kept in order
template <typename T>
void sortStrings(T &v, const bool noCase) {
  std::sort(
      v.begin(), v.end(), [&](const std::string &lhs, const std::string &rhs) {
        if (noCase) {
          return StringUtil::toUpperCase(lhs) < StringUtil::toUpperCase(rhs);
        } else {
          return lhs < rhs;
        }
      });
}
} // namespace StringUtil
} // namespace ETJump
