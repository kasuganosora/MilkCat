//
// The MIT License (MIT)
//
// Copyright 2013-2014 The MilkCat Project Developers
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// utils.h --- Created at 2013-08-10
//



#ifndef SRC_UTILS_UTILS_H_
#define SRC_UTILS_UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "utils/status.h"

#if defined(HAVE_UNORDERED_MAP)
#include <unordered_map>
#elif defined(HAVE_TR1_UNORDERED_MAP)
#include <tr1/unordered_map>
#else
#include <map>
#endif

namespace milkcat {

/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t strlcpy(char *dst, const char *src, size_t siz);
char *trim(char *str);

const char *_filename(const char *path);

// Sleep for seconds
void Sleep(double seconds);

// Get number of processors/cores in current machine
int HardwareConcurrency();

#if defined(HAVE_UNORDERED_MAP)
using std::unordered_map;
#elif defined(HAVE_TR1_UNORDERED_MAP)
using std::tr1::unordered_map;
#else
template<typename TK, typename TV>
using unordered_map = std::map<TK, TV>;
#endif

}  // namespace milkcat

#ifdef DEBUG
#define LOG(...)  printf(__VA_ARGS__)
#else 
#define LOG(...)
#endif 

#define ERROR(message) \
        do { \
          fprintf(stderr,  \
                  "[%s:%d] ERROR: ", \
                  _filename(__FILE__), \
                  __LINE__); \
          fputs(message, stderr); \
          fputs("\n", stderr); \
          exit(1); \
        } while (0);


#ifndef NOASSERT
#define ASSERT(cond, message) \
        if (!(cond)) { \
          fprintf(stderr,  \
                  "[%s:%d] ASSERT failed: ", \
                  _filename(__FILE__), \
                  __LINE__); \
          fputs(message, stderr); \
          fputs("\n", stderr); \
          exit(1); \
        }
#else
#define ASSERT(cond, message)
#endif  // NOASSERT

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
        TypeName(const TypeName&); \
        void operator=(const TypeName&)

#endif  // SRC_UTILS_UTILS_H_
