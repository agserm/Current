/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2016 Dmitry "Dima" Korolev <dmitry.korolev@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/

#ifndef KARL_SERVICE_IS_PRIME_H
#define KARL_SERVICE_IS_PRIME_H

#include "claire.h"

#include "../Blocks/HTTP/api.h"

namespace karl_unittest {

class ServiceIsPrime final {
 public:
  explicit ServiceIsPrime(uint16_t port, const current::karl::Locator& karl)
      : http_scope_(HTTP(port).Register(
            "/is_prime",
            [this](Request r) {
              r(IsPrime(current::FromString<int>(r.url.query.get("x", "0"))) ? "YES\n" : "NO\n");
            })),
        claire_("is_prime", karl, port) {}

 private:
  static bool IsPrime(int x) {
    if (x < 2) {
      return false;
    } else {
      for (int i = 2; i * i <= x; ++i) {
        if ((x % i) == 0) {
          return false;
        }
      }
      return true;
    }
  }

  const HTTPRoutesScope http_scope_;
  const current::karl::Claire claire_;
};

}  // namespace karl_unittest

#endif  // KARL_SERVICE_IS_PRIME_H
