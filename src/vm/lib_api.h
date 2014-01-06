/***************************************************************************
 * Shared library API header file
 *
 * Copyright (c) 2011-2013, Randy Hollines
 * All rights reserved.
 *
 * Redistribution and use in sohurce and binary forms, with or without
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

#ifndef __LIB_API_H__
#define __LIB_API_H__

#include "common.h"

using namespace std;

// offset for Objeck arrays
#define ARRAY_HEADER_OFFSET 3

// function declaration for native C++ callbacks
typedef void(*APITools_MethodCall_Ptr) (long* op_stack, long *stack_pos, long *instance, 
					const wchar_t* cls_name, const wchar_t* mthd_name);
typedef void(*APITools_MethodCallId_Ptr) (long* op_stack, long *stack_pos, long *instance, 
					const int cls_id, const int mthd_id);
typedef long*(*APITools_AllocateObject_Ptr) (const wchar_t*, long* op_stack, long stack_pos, bool collect);
typedef long*(*APITools_AllocateArray_Ptr) (const long size, const instructions::MemoryType type, 
					    long* op_stack, long stack_pos, bool collect);
// context structure
struct VMContext {
  long* data_array;
  long* op_stack;
  long* stack_pos;
  APITools_AllocateArray_Ptr alloc_array;
  APITools_AllocateObject_Ptr alloc_obj;
  APITools_MethodCall_Ptr call_method_by_name;
  APITools_MethodCallId_Ptr call_method_by_id;
};

// function identifiers consist of two integer IDs
enum FunctionId {
  CLS_ID = 0,
  MTHD_ID
};

// gets the size of an Object[] array
int APITools_GetArgumentCount(VMContext &context) {
  if(context.data_array) {
    return context.data_array[0];
  }

  return 0;
}

long APITools_GetIntArrayElement(long* array, int index) {
  if(!array) {
    return 0;
  }
  
  const long src_array_len = array[0];
  if(index < src_array_len) {
    long* src_array_ptr = array + 3;
    return src_array_ptr[index];
  }
  return 0;
}

void APITools_SetIntArrayElement(long* array, int index, long value) {
  if(!array) {
    return;
  }
  
  const long src_array_len = array[0];
  if(index < src_array_len) {
    long* src_array_ptr = array + 3;
    src_array_ptr[index] = value;
  }
}

double APITools_GetFloatArrayElement(long* array, int index) {
  if(!array) {
    return 0.0;
  }
  
  const long src_array_len = array[0];
  if(index < src_array_len) {
    long* src_array_ptr = array + 3;
    double value;
#ifdef _X64
    memcpy(&value, &src_array_ptr[index], sizeof(value));
#else
    memcpy(&value, &src_array_ptr[index * 2], sizeof(value));
#endif
    return value;
  }
  
  return 0.0;
}

void APITools_SetFloatArrayElement(long* array, int index, double value) {
  if(!array) {
    return;
  }
  
  const long src_array_len = array[0];
  if(index < src_array_len) {
    long* src_array_ptr = array + 3;
#ifdef _X64
    memcpy(&src_array_ptr[index], &value, sizeof(value));
#else
    memcpy(&src_array_ptr[index * 2], &value, sizeof(value));
#endif
  }
}

unsigned char* APITools_GetByteArray(long* array) {
  if(array) {
    return (unsigned char*)(array + 3);
  }
  
  return NULL;
}

wchar_t* APITools_GetCharArray(long* array) {
  if(array) {
    return (wchar_t*)(array + 3);
  }
  
  return NULL;
}

int APITools_GetArraySize(long* array) {
  if(array) {
    return array[0];
  }
  
  return -1;
}

long* APITools_MakeByteArray(VMContext &context, const long char_array_size) {
  // create character array
  const long char_array_dim = 1;
  long* char_array = (long*)context.alloc_array(char_array_size + 1 +
						((char_array_dim + 2) *
						 sizeof(long)),
						BYTE_ARY_TYPE,
						context.op_stack, *context.stack_pos, false);
  char_array[0] = char_array_size + 1;
  char_array[1] = char_array_dim;
  char_array[2] = char_array_size;

  return char_array;
}

long* APITools_MakeCharArray(VMContext &context, const long char_array_size) {
  // create character array
  const long char_array_dim = 1;
  long* char_array = (long*)context.alloc_array(char_array_size + 1 +
						((char_array_dim + 2) *
						 sizeof(long)),
						CHAR_ARY_TYPE,
						context.op_stack, *context.stack_pos, false);
  char_array[0] = char_array_size + 1;
  char_array[1] = char_array_dim;
  char_array[2] = char_array_size;

  return char_array;
}

// gets the requested function ID from an Object[]
int APITools_GetFunctionValue(VMContext &context, int index, FunctionId id) {
  long* data_array = context.data_array;
  if(data_array && index < data_array[0]) {
    data_array += ARRAY_HEADER_OFFSET;
    long* int_holder = (long*)data_array[index];
    
    if(id == CLS_ID) {
      return int_holder[0];
    }
    else {
      return int_holder[1];
    }
  }
  
  return 0;
}

// sets the requested function ID from an Object[].  Please note, that 
// memory should be allocated for this element prior to array access.
void APITools_SetFunctionValue(VMContext &context, int index, FunctionId id, int value) {
  long* data_array = context.data_array;
  if(data_array && index < data_array[0]) {
    data_array += ARRAY_HEADER_OFFSET;
    long* int_holder = (long*)data_array[index];
    
    if(id == CLS_ID) {
      int_holder[0] = value;
    }
    else {
      int_holder[1] = value;
    }
  }
}

// get the requested integer value from an Object[].
long APITools_GetIntValue(VMContext &context, int index) {
  long* data_array = context.data_array;
  if(data_array && index < data_array[0]) {
    data_array += ARRAY_HEADER_OFFSET;
    long* int_holder = (long*)data_array[index];
#ifdef _DEBUG
    assert(int_holder);
#endif
    return int_holder[0];
  }

  return 0;
}

// get the requested integer address from an Object[].
long* APITools_GetIntAddress(VMContext &context, int index) {
  long* data_array = context.data_array;
  if(data_array && index < data_array[0]) {
    data_array += ARRAY_HEADER_OFFSET;
    long* int_holder = (long*)data_array[index];
#ifdef _DEBUG
    assert(int_holder);
#endif
    return int_holder;
  }

  return NULL;
}

// sets the requested function ID from an Object[].  Please note, that 
// memory should be allocated for this element prior to array access.
void APITools_SetIntValue(VMContext &context, int index, long value) {
  long* data_array = context.data_array;
  if(data_array && index < data_array[0]) {
    data_array += ARRAY_HEADER_OFFSET;
    long* int_holder = (long*)data_array[index];
#ifdef _DEBUG
    assert(int_holder);
#endif
    int_holder[0] = value;
  }
}

// get the requested double value from an Object[].
double APITools_GetFloatValue(VMContext &context, int index) {
  long* data_array = context.data_array;
  if(data_array && index < data_array[0]) {
    data_array += ARRAY_HEADER_OFFSET;
    long* float_holder = (long*)data_array[index];

#ifdef _DEBUG
    assert(float_holder);
#endif		
    double value;
    memcpy(&value, float_holder, sizeof(value));
    return value;
  }

  return 0.0;
} 

// get the requested double address from an Object[].
long* APITools_GetFloatAddress(VMContext &context, int index) {
  long* data_array = context.data_array;
  if(data_array && index < data_array[0]) {
    data_array += ARRAY_HEADER_OFFSET;
    long* float_holder = (long*)data_array[index];

#ifdef _DEBUG
    assert(float_holder);
#endif		
    return float_holder;
  }

  return NULL;
} 

// sets the requested float value for an Object[].  Please note, that 
// memory should be allocated for this element prior to array access.
void APITools_SetFloatValue(VMContext &context, int index, double value) {
 long* data_array = context.data_array;
  if(data_array && index < data_array[0]) {
    data_array += ARRAY_HEADER_OFFSET;
    long* float_holder = (long*)data_array[index];

#ifdef _DEBUG
    assert(float_holder);
#endif
    memcpy(float_holder, &value, sizeof(value));
  }
}

// sets the requested Base object for an Object[].  Please note, that 
// memory should be allocated for this element prior to array access.
void APITools_SetObjectValue(VMContext &context, int index, long* obj) {
  long* data_array = context.data_array;
  if(data_array && index < data_array[0]) {
    data_array += ARRAY_HEADER_OFFSET;
    data_array[index] = (long)obj;
  }
}

// sets the requested String object for an Object[].  Please note, that 
// memory should be allocated for this element prior to array access.
void APITools_SetStringValue(VMContext &context, int index, const wstring &value) {
  // create character array
  const long char_array_size = value.size();
  const long char_array_dim = 1;
  long* char_array = (long*)context.alloc_array(char_array_size + 1 +
						((char_array_dim + 2) *
						 sizeof(long)),
						CHAR_ARY_TYPE,
						context.op_stack, *context.stack_pos, false);
  char_array[0] = char_array_size + 1;
  char_array[1] = char_array_dim;
  char_array[2] = char_array_size;
  
  // copy string
  wchar_t* char_array_ptr = (wchar_t*)(char_array + 3);
  wcsncpy(char_array_ptr, value.c_str(), char_array_size);
  
  // create 'System.String' object instance
  long* str_obj = context.alloc_obj(L"System.String", (long*)context.op_stack, 
				    *context.stack_pos, false);
  str_obj[0] = (long)char_array;
  str_obj[1] = char_array_size;
  str_obj[2] = char_array_size;
  
  APITools_SetObjectValue(context, index, str_obj);
}

// get the requested string value from an Object[].
wchar_t* APITools_GetStringValue(VMContext &context, int index) {
  long* data_array = context.data_array;
  if(data_array && index < data_array[0]) {
    data_array += ARRAY_HEADER_OFFSET;
    long* string_holder = (long*)data_array[index];
    
#ifdef _DEBUG
    assert(string_holder);
#endif
    long* char_array = (long*)string_holder[0];
    wchar_t* str = (wchar_t*)(char_array + 3);
    return str;
  }
  
  return NULL;
}

long* APITools_GetObjectValue(VMContext &context, int index) {
  long* data_array = context.data_array;
  if(data_array && index < data_array[0]) {
    data_array += ARRAY_HEADER_OFFSET;
    long* object_holder = (long*)data_array[index];
    
#ifdef _DEBUG
    assert(object_holder);
#endif
    return object_holder;
  }
  
  return NULL;
}


// invokes a runtime Objeck method
void APITools_CallMethod(VMContext &context, long* instance, const wchar_t* mthd_name) {
  const wstring qualified_method_name(mthd_name);
  size_t delim = qualified_method_name.find(':');
  if(delim != wstring::npos) {
    wstring cls_name = qualified_method_name.substr(0, delim);
    (*context.call_method_by_name)(context.op_stack, context.stack_pos, instance, cls_name.c_str(), mthd_name);
    
#ifdef _DEBUG
    assert(*context.stack_pos == 0);
#endif
  }
  else {
    cerr << ">>> DLL call: Invalid method name: '" << mthd_name << "'" << endl;
    exit(1);
  }
}

// invokes a runtime Objeck method
void APITools_CallMethod(VMContext &context, long* instance, const int cls_id, const int mthd_id) {
  (*context.call_method_by_id)(context.op_stack, context.stack_pos, instance, cls_id, mthd_id);
    
#ifdef _DEBUG
    assert(*context.stack_pos == 0);
#endif
}

// pushes an integer value onto the runtime stack
void APITools_PushInt(VMContext &context, long value) {
  context.op_stack[(*context.stack_pos)++] = value;
}

// pushes an double value onto the runtime stack
void APITools_PushFloat(VMContext &context, double v) {
  memcpy(&context.op_stack[(*context.stack_pos)], &v, sizeof(double));
#ifdef _X64
  (*context.stack_pos)++;
#else
  (*context.stack_pos) += 2;
#endif
}

#endif
