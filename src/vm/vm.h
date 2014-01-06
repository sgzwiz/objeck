/***************************************************************************
* Defines the VM execution model.
*
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
* - Neither the name of the Objeck Team nor the names of its
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

#ifndef __VM_H__
#define __VM_H__

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif
#include "common.h"
#include "loader.h"
#include "interpreter.h"

extern "C"
{
#ifdef _WIN32
  __declspec(dllexport) int Execute(const int argc, const char* argv[]);
#else
  int Execute(const int argc, const char* argv[]);
#endif
}

static wchar_t** ProcessCommandLine(const int argc, const char* argv[]) {
  wchar_t** wide_args = new wchar_t*[argc];
  for(int i = 0; i < argc; i++) {
    const char* arg = argv[i];
    const int len = strlen(arg);
    wchar_t* wide_arg = new wchar_t[len + 1];
    for(int j = 0; j < len; j++) {
      wide_arg[j] = arg[j];
    }
    wide_arg[len] = L'\0';
    wide_args[i] = wide_arg;
  }

  return wide_args;
}

static void CleanUpCommandLine(const int argc, wchar_t** wide_args) {
  for(int i = 0; i < argc; i++) {
    wchar_t* wide_arg = wide_args[i];
    delete[] wide_arg;
    wide_arg = NULL;
  }

  delete[] wide_args;
  wide_args = NULL;
}

#endif