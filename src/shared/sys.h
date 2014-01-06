/***************************************************************************
 * Copyright (c) 2008-2013, Randy Hollines
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the distribution.
 * - Neither the name of the StackVM Team nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ***************************************************************************/

#ifndef __SYS_H__
#define __SYS_H__

// define windows type
#ifdef _WIN32
#include <windows.h>
#include <stdint.h>
#endif
#include <string>
#include <map>

// memory size for local stack frames
#define LOCAL_SIZE 192

#ifdef _MINGW
#define INT_VALUE int
#define FLOAT_VALUE double
#else
#define INT_VALUE int32_t
#define FLOAT_VALUE double
#endif

using namespace std;

namespace instructions {
  // vm types
  typedef enum _MemoryType {
    NIL_TYPE = -1000,
    BYTE_ARY_TYPE,
    CHAR_ARY_TYPE,
    INT_TYPE,
    FLOAT_TYPE,
    FUNC_TYPE
  } 
  MemoryType;

  // garbage types
  typedef enum _ParamType {
    CHAR_PARM = -1500,
    INT_PARM,
    FLOAT_PARM,
    BYTE_ARY_PARM,
    CHAR_ARY_PARM,
    INT_ARY_PARM,
    FLOAT_ARY_PARM,
    OBJ_PARM,
    OBJ_ARY_PARM,
    FUNC_PARM,
  } 
  ParamType;
}

static map<const wstring, wstring> ParseCommnadLine(const wstring &path_string) {    
  map<const wstring, wstring> arguments;

  size_t pos = 0;
  size_t end = path_string.size();  
  while(pos < end) {
    // ignore leading white space
    while( pos < end && (path_string[pos] == L' ' || path_string[pos] == L'\t')) {
      pos++;
    }
    if(path_string[pos] == '-') {
      // parse key
      int start =  ++pos;
      while( pos < end && path_string[pos] != L' ' && path_string[pos] != L'\t') {
        pos++;
      }
      wstring key = path_string.substr(start, pos - start);
      // parse value
      while(pos < end && (path_string[pos] == L' ' || path_string[pos] == L'\t')) {
        pos++;
      }
      start = pos;
      bool is_string = false;
      if(pos < end && path_string[pos] == L'\'') {
        is_string = true;
        start++;
        pos++;
      }
      bool not_end = true;
      while(pos < end && not_end) {
        // check for end
        if(is_string) {
          not_end = path_string[pos] != L'\'';
        }
        else {
          not_end = path_string[pos] != L' ' && path_string[pos] != L'\t' && path_string[pos] != L'-';
        }
        // update position
        if(not_end) {
          pos++;
        }
      }
      wstring value = path_string.substr(start, pos - start);
      
      // close string and add
      if(path_string[pos] == L'\'') {
        pos++;
      }
      arguments.insert(pair<wstring, wstring>(key, value));
    }
    else {
      pos++;
    }
  }

  return arguments;
}

/****************************
 * Converts a UTF-8 bytes to
 * native a unicode string
 ****************************/
static bool BytesToUnicode(const string &in, wstring &out) {    
#ifdef _WIN32
  // allocate space
  int wsize = MultiByteToWideChar(CP_UTF8, 0, in.c_str(), -1, NULL, 0);
  if(!wsize) {
    return false;
  }
  wchar_t* buffer = new wchar_t[wsize];

  // convert
  int check = MultiByteToWideChar(CP_UTF8, 0, in.c_str(), -1, buffer, wsize);
  if(!check) {
    delete[] buffer;
    buffer = NULL;
    return false;
  }
  
  // create string
  out.append(buffer, wsize - 1);

  // clean up
  delete[] buffer;
  buffer = NULL;  
#else
  // allocate space
  size_t size = mbstowcs(NULL, in.c_str(), in.size());
  if(size == (size_t)-1) {
    return false;
  }
  wchar_t* buffer = new wchar_t[size + 1];

  // convert
  size_t check = mbstowcs(buffer, in.c_str(), in.size());
  if(check == (size_t)-1) {
    delete[] buffer;
    buffer = NULL;
    return false;
  }
  buffer[size] = L'\0';

  // create string
  out.append(buffer, size);

  // clean up
  delete[] buffer;
  buffer = NULL;
#endif

  return true;
}

static wstring BytesToUnicode(const string &in) {
  wstring out;
  if(BytesToUnicode(in, out)) {
    return out;
  }

  return L"";
}

/****************************
 * Converts UTF-8 bytes to
 * native a unicode character
 ****************************/
static bool BytesToCharacter(const string &in, wchar_t &out) {
  wstring buffer;
  if(!BytesToUnicode(in, buffer)) {
    return false;
  }
  
  if(buffer.size() != 1) {
    return false;
  }
  
  out = buffer[0];  
  return true;
}

/****************************
 * Converts a native string
 * to UTF-8 bytes
 ****************************/
static bool UnicodeToBytes(const wstring &in, string &out) {
#ifdef _WIN32
  // allocate space
  int size = WideCharToMultiByte(CP_UTF8, 0, in.c_str(), -1, NULL, 0, NULL, NULL);
  if(!size) {
    return false;
  }
  char* buffer = new char[size];
  
  // convert string
  int check = WideCharToMultiByte(CP_UTF8, 0, in.c_str(), -1, buffer, size, NULL, NULL);
  if(!check) {
    delete[] buffer;
    buffer = NULL;
    return false;
  }
  
  // append output
  out.append(buffer, size - 1);

  // clean up
  delete[] buffer;
  buffer = NULL;
#else
  // convert string
  size_t size = wcstombs(NULL, in.c_str(), in.size());
  if(size == (size_t)-1) {
    return false;
  }
  char* buffer = new char[size + 1];
  
  wcstombs(buffer, in.c_str(), size);
  if(size == (size_t)-1) {
    delete[] buffer;
    buffer = NULL;
    return false;
  }
  buffer[size] = '\0';
  
  // create string      
  out.append(buffer, size);

  // clean up
  delete[] buffer;
  buffer = NULL;
#endif
  
  return true;
}

static string UnicodeToBytes(const wstring &in) {
  string out;
  if(UnicodeToBytes(in, out)) {
    return out;
  }

  return "";
}

/****************************
 * Converts a native character
 * to UTF-8 bytes
 ****************************/
static bool CharacterToBytes(wchar_t in, string &out) {
  if(in == L'\0') {
    return true;
  }
  
  wchar_t buffer[2];
  buffer[0] = in;
  buffer[1] = L'\0';

  if(!UnicodeToBytes(buffer, out)) {
    return false;
  }
  
  return true;
}
#endif
