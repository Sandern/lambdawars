///
/// @file STLHelpers.h
///
/// @brief The header for all of the STL helper functions.
///
/// @author
///
/// This file is a part of Awesomium, a Web UI bridge for native apps.
///
/// Website: <http://www.awesomium.com>
///
/// Copyright (C) 2012 Khrona. All rights reserved. Awesomium is a
/// trademark of Khrona.
///
#ifndef AWESOMIUM_STL_HELPERS_H_
#define AWESOMIUM_STL_HELPERS_H_

#include <Awesomium/WebString.h>
#include <string>
#include <ostream>
#include <istream>

namespace Awesomium {

// Convert WebString to a std::string
inline std::string ToString(const WebString& str) {
  std::string result;

  if (str.IsEmpty())
    return std::string();

  unsigned int len = str.ToUTF8(NULL, 0);

  char* buffer = new char[len];
  str.ToUTF8(buffer, len);

  result.assign(buffer, len);
  delete[] buffer;

  return result;
}

// Convert std::string to a WebString
inline WebString ToWebString(const std::string& str) {
  return WebString::CreateFromUTF8(str.data(), str.length());
}

// Stream operator to allow WebString output to an ostream
inline std::ostream& operator<<(std::ostream& out, const WebString& str) {
    out << ToString(str);
    return out;
}

// Stream operator to allow WebString to be input from an istream
inline std::istream& operator>>(std::istream& in, WebString& out) {
    std::string x;
    in >> x;
    out.Append(WebString::CreateFromUTF8(x.data(), x.length()));
    return in;
}

// Web String Literal (for quick, inline string definitions)
inline WebString WSLit(const char* string_literal) {
  return WebString::CreateFromUTF8(string_literal, strlen(string_literal));
}

}  // namespace Awesomium

#endif  // AWESOMIUM_STL_HELPERS_H_
