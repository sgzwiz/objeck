/***************************************************************************
 * Program loader.
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

#include "loader.h"
#include "common.h"
#include "../shared/version.h"

StackProgram* Loader::program;

StackProgram* Loader::GetProgram() {
  return program;
}

void Loader::LoadConfiguration()
{
  ifstream in("obr.conf");
  if(in.good()) {
    string line;
    do {
      getline(in, line);
      size_t pos = line.find('=');
      if(pos != string::npos) {
        string name = line.substr(0, pos);
        string value = line.substr(pos + 1);
        params.insert(pair<const wstring, int>(BytesToUnicode(name), atoi(value.c_str())));
      }
    }
    while(!in.eof());
  }
  in.close();
}

void Loader::Load()
{
  const int ver_num = ReadInt();
  if(ver_num != VER_NUM) {
    wcerr << L"This executable appears to be invalid or compiled with a different version of the toolchain." << endl;
    exit(1);
  } 

  const int magic_num = ReadInt();
  switch(magic_num) {
  case MAGIC_NUM_LIB:
    wcerr << L"Unable to use execute shared library '" << filename << L"'." << endl;
    exit(1);

  case MAGIC_NUM_EXE:
    break;

  case MAGIC_NUM_WEB:
    is_web = true;
    break;

  default:
    wcerr << L"Unknown file type for '" << filename << L"'." << endl;
    exit(1);
  }

  // read string id
  string_cls_id = ReadInt();;

  int i;
  // read float strings
  num_float_strings = ReadInt();
  FLOAT_VALUE** float_strings = new FLOAT_VALUE*[num_float_strings];
  for(i = 0; i < num_float_strings; i++) {
    const int float_string_length = ReadInt();
    FLOAT_VALUE* float_string = new FLOAT_VALUE[float_string_length];
    // copy string    
#ifdef _DEBUG
    wcout << L"Loaded static float string[" << i << L"]: '";
#endif
    for(int j = 0; j < float_string_length; j++) {
      float_string[j] = ReadDouble();
#ifdef _DEBUG
      wcout << float_string[j] << L",";
#endif
    }
#ifdef _DEBUG
    wcout << L"'" << endl;
#endif
    float_strings[i] = float_string;
  }
  program->SetFloatStrings(float_strings, num_float_strings);

  // read int strings
  num_int_strings = ReadInt();
  INT_VALUE** int_strings = new INT_VALUE*[num_int_strings];
  for(i = 0; i < num_int_strings; i++) {
    const int int_string_length = ReadInt();
    INT_VALUE* int_string = new INT_VALUE[int_string_length];
    // copy string    
#ifdef _DEBUG
    wcout << L"Loaded static int string[" << i << L"]: '";
#endif
    for(int j = 0; j < int_string_length; j++) {
      int_string[j] = ReadInt();
#ifdef _DEBUG
      wcout << int_string[j] << L",";
#endif
    }
#ifdef _DEBUG
    wcout << L"'" << endl;
#endif
    int_strings[i] = int_string;
  }
  program->SetIntStrings(int_strings, num_int_strings);

  // read char strings
  num_char_strings = ReadInt();
  wchar_t** char_strings = new wchar_t*[num_char_strings + arguments.size()];
  for(i = 0; i < num_char_strings; i++) {
    const wstring value = ReadString();
    wchar_t* char_string = new wchar_t[value.size() + 1];
    // copy string
    size_t j = 0;
    for(; j < value.size(); j++) {
      char_string[j] = value[j];
    }
    char_string[j] = L'\0';
#ifdef _DEBUG
    wcout << L"Loaded static character string[" << i << L"]: '" << char_string << L"'" << endl;
#endif
    char_strings[i] = char_string;
  }

  // copy command line params
  for(size_t j = 0; j < arguments.size(); i++, j++) {
#ifdef _WIN32
    char_strings[i] = _wcsdup((arguments[j]).c_str());
#else
    char_strings[i] = wcsdup((arguments[j]).c_str());
#endif

#ifdef _DEBUG
    wcout << L"Loaded static string: '" << char_strings[i] << L"'" << endl;
#endif
  }
  program->SetCharStrings(char_strings, num_char_strings);

#ifdef _DEBUG
  wcout << L"=======================================" << endl;
#endif
  
  // read start class and method ids
  start_class_id = ReadInt();
  start_method_id = ReadInt();
#ifdef _DEBUG
  wcout << L"Program starting point: " << start_class_id << L","
	<< start_method_id << endl;
#endif

  LoadEnums();
  LoadClasses();

  wstring name = L"$Initialization$:";
  StackDclr** dclrs = new StackDclr*[1];
  dclrs[0] = new StackDclr;
  dclrs[0]->name = L"args";
  dclrs[0]->type = OBJ_ARY_PARM;

  init_method = new StackMethod(-1, name, false, false, dclrs,	1, 0, 1, NIL_TYPE, NULL);
  LoadInitializationCode(init_method);
  program->SetInitializationMethod(init_method);
  program->SetStringObjectId(string_cls_id);
}

void Loader::LoadEnums()
{
  const int number = ReadInt();
  for(int i = 0; i < number; i++) {
    // read enum
    // const string &enum_name = ReadString();
    ReadString();

    // const long enum_offset = ReadInt();
    ReadInt();

    // read enum items
    const long num_items = ReadInt();
    for(int i = 0; i < num_items; i++) {
      // const string &item_name = ReadString();
      ReadString();

      // const long item_id = ReadInt();
      ReadInt();
    }
  }
}

void Loader::LoadClasses()
{
  const int number = ReadInt();
  int* cls_hierarchy = new int[number];
  int** cls_interfaces = new int*[number];
  StackClass** classes = new StackClass*[number];

#ifdef _DEBUG
  wcout << L"Reading " << number << L" classe(s)..." << endl;
#endif

  for(int i = 0; i < number; i++) {
    // read id and pid
    const int id = ReadInt();
    wstring name = ReadString();
    const int pid = ReadInt();
    wstring parent_name = ReadString();

    // read interface ids
    const int interface_size = ReadInt();
    if(interface_size > 0) {
      int* interfaces = new int[interface_size + 1];
      int i = 0;
      while(i < interface_size) {
        interfaces[i++] = ReadInt();
      }
      interfaces[i] = -1;
      cls_interfaces[id] = interfaces;
    }
    else {
      cls_interfaces[id] = NULL;
    }

    // read interface names
    const int interface_names_size = ReadInt();
    for(int i = 0; i < interface_names_size; i++) {
      ReadString();
    }

    // is interface (covered by is virtual)
    ReadInt();

    const bool is_virtual = ReadInt() != 0;
    const bool is_debug = ReadInt() != 0;
    wstring file_name;
    if(is_debug) {
      file_name = ReadString();
    }

    // space
    const int cls_space = ReadInt();
    const int inst_space = ReadInt();

    // read class types
    const int cls_num_dclrs = ReadInt();
    StackDclr** cls_dclrs = new StackDclr*[cls_num_dclrs];
    for(int i = 0; i < cls_num_dclrs; i++) {
      // set type
      int type = ReadInt();
      // set name
      wstring name;
      if(is_debug) {
        name = ReadString();
      }
      cls_dclrs[i] = new StackDclr;
      cls_dclrs[i]->name = name;
      cls_dclrs[i]->type = (ParamType)type;
    }

    // read instance types
    const int inst_num_dclrs = ReadInt();
    StackDclr** inst_dclrs = new StackDclr*[inst_num_dclrs];
    for(int i = 0; i < inst_num_dclrs; i++) {
      // set type
      int type = ReadInt();
      // set name
      wstring name;
      if(is_debug) {
        name = ReadString();
      }
      inst_dclrs[i] = new StackDclr;
      inst_dclrs[i]->name = name;
      inst_dclrs[i]->type = (ParamType)type;
    }

    cls_hierarchy[id] = pid;
    StackClass* cls = new StackClass(id, name, file_name, pid, is_virtual, 
				     cls_dclrs, cls_num_dclrs, inst_dclrs, 
				     inst_num_dclrs, cls_space, inst_space, is_debug);

#ifdef _DEBUG
    wcout << L"Class(" << cls << L"): id=" << id << L"; name='" << name << L"'; parent='"
	  << parent_name << L"'; class_bytes=" << cls_space << L"'; instance_bytes="
	  << inst_space << endl;
#endif

    // load methods
    LoadMethods(cls, is_debug);
    // add class
#ifdef _DEBUG
    assert(id < number);
#endif
    classes[id] = cls;
  }

  // set class hierarchy and interfaces
  program->SetClasses(classes, number);
  program->SetHierarchy(cls_hierarchy);
  program->SetInterfaces(cls_interfaces);
}

void Loader::LoadMethods(StackClass* cls, bool is_debug)
{
  const int number = ReadInt();
#ifdef _DEBUG
  wcout << L"Reading " << number << L" method(s)..." << endl;
#endif

  StackMethod** methods = new StackMethod*[number];
  for(int i = 0; i < number; i++) {
    // id
    const int id = ReadInt();
    // method type
    ReadInt();
    // virtual
    const bool is_virtual = ReadInt() != 0;
    // has and/or
    const bool has_and_or = ReadInt() != 0;
    // is native
    ReadInt();
    // is static
    ReadInt();
    // name
    const wstring name = ReadString();
    // return
    const wstring rtrn_name = ReadString();
    // params
    const int params = ReadInt();
    // space
    const int mem_size = ReadInt();
    // read type parameters
    const int num_dclrs = ReadInt();

    StackDclr** dclrs = new StackDclr*[num_dclrs];
    for(int i = 0; i < num_dclrs; i++) {
      // set type
      const int type = ReadInt();
      // set name
      wstring name;
      if(is_debug) {
        name = ReadString();
      }
      dclrs[i] = new StackDclr;
      dclrs[i]->name = name;
      dclrs[i]->type = (ParamType)type;
    }

    // parse return
    MemoryType rtrn_type;
    switch(rtrn_name[0]) {
    case L'l': // bool
    case L'b': // byte
    case L'c': // character
    case L'i': // int
    case L'o': // object
      rtrn_type = INT_TYPE;
      break;

    case L'f': // float
      if(rtrn_name.size() > 1) {
        rtrn_type = INT_TYPE;
      } else {
        rtrn_type = FLOAT_TYPE;
      }
      break;

    case L'n': // nil
      rtrn_type = NIL_TYPE;
      break;

    case L'm': // function
      rtrn_type = FUNC_TYPE;
      break;

    default:
      wcerr << L">>> unknown type <<<" << endl;
      exit(1);
      break;
    }

    StackMethod* mthd = new StackMethod(id, name, is_virtual, has_and_or, dclrs,
					num_dclrs, params, mem_size, rtrn_type, cls);    
    // load statements
#ifdef _DEBUG
    wcout << L"Method(" << mthd << L"): id=" << id << L"; name='" << name << L"'; return='" 
	  << rtrn_name << L"'; params=" << params << L"; bytes=" 
	  << mem_size << endl;
#endif    
    LoadStatements(mthd, is_debug);

    // add method
#ifdef _DEBUG
    assert(id < number);
#endif
    methods[id] = mthd;
  }
  cls->SetMethods(methods, number);
}

void Loader::LoadInitializationCode(StackMethod* method)
{
  vector<StackInstr*> instrs;

  instrs.push_back(new StackInstr(-1, LOAD_INT_LIT, (long)arguments.size()));
  instrs.push_back(new StackInstr(-1, NEW_INT_ARY, (long)1));
  instrs.push_back(new StackInstr(-1, STOR_LOCL_INT_VAR, 0L, LOCL));

  for(size_t i = 0; i < arguments.size(); i++) {
    instrs.push_back(new StackInstr(-1, LOAD_INT_LIT, (long)arguments[i].size()));
    instrs.push_back(new StackInstr(-1, NEW_CHAR_ARY, 1L));
    instrs.push_back(new StackInstr(-1, LOAD_INT_LIT, (long)(num_char_strings + i)));
    instrs.push_back(new StackInstr(-1, LOAD_INT_LIT, (long)instructions::CPY_CHAR_STR_ARY));
    instrs.push_back(new StackInstr(-1, TRAP_RTRN, 3L));

    instrs.push_back(new StackInstr(-1, NEW_OBJ_INST, (long)string_cls_id));
    // note: method ID is position dependant
    instrs.push_back(new StackInstr(-1, MTHD_CALL, (long)string_cls_id, 2L, 0L));

    instrs.push_back(new StackInstr(-1, LOAD_INT_LIT, (long)i));
    instrs.push_back(new StackInstr(-1, LOAD_LOCL_INT_VAR, 0L, LOCL));
    instrs.push_back(new StackInstr(-1, STOR_INT_ARY_ELM, 1L, LOCL));
  }

  instrs.push_back(new StackInstr(-1, LOAD_LOCL_INT_VAR, 0L, LOCL));
  instrs.push_back(new StackInstr(-1, LOAD_INST_MEM));
  instrs.push_back(new StackInstr(-1, MTHD_CALL, (long)start_class_id, 
				  (long)start_method_id, 0L));
  instrs.push_back(new StackInstr(-1, RTRN));

  // copy and set instructions
  StackInstr** mthd_instrs = new StackInstr*[instrs.size()];
  copy(instrs.begin(), instrs.end(), mthd_instrs);
  method->SetInstructions(mthd_instrs, instrs.size());
}

void Loader::LoadStatements(StackMethod* method, bool is_debug)
{
  vector<StackInstr*> instrs;

  int index = 0;
  int type = ReadByte();
  int line_num = -1;
  while(type != END_STMTS) {
    if(is_debug) {
      line_num = ReadInt();
    }
    switch(type) {
    case LOAD_INT_LIT:
      instrs.push_back(new StackInstr(line_num, LOAD_INT_LIT, (long)ReadInt()));
      break;

    case LOAD_CHAR_LIT:
      instrs.push_back(new StackInstr(line_num, LOAD_CHAR_LIT, (long)ReadChar()));
      break;

    case SHL_INT:
      instrs.push_back(new StackInstr(line_num, SHL_INT));
      break;

    case SHR_INT:
      instrs.push_back(new StackInstr(line_num, SHR_INT));
      break;

    case LOAD_INT_VAR: {
      long id = ReadInt();
      MemoryContext mem_context = (MemoryContext)ReadInt();
      instrs.push_back(new StackInstr(line_num, 
				      mem_context == LOCL ? LOAD_LOCL_INT_VAR : LOAD_CLS_INST_INT_VAR, 
				      id, mem_context));
    }
      break;

    case LOAD_FUNC_VAR: {
      long id = ReadInt();
      MemoryContext mem_context = (MemoryContext)ReadInt();
      instrs.push_back(new StackInstr(line_num, LOAD_FUNC_VAR, id, mem_context));
    }
      break;

    case LOAD_FLOAT_VAR: {
      long id = ReadInt();
      MemoryContext mem_context = (MemoryContext)ReadInt();
      instrs.push_back(new StackInstr(line_num, LOAD_FLOAT_VAR, id, mem_context));
    }
      break;

    case STOR_INT_VAR: {
      long id = ReadInt();
      MemoryContext mem_context = (MemoryContext)ReadInt();
      instrs.push_back(new StackInstr(line_num, 
				      mem_context == LOCL ? STOR_LOCL_INT_VAR : STOR_CLS_INST_INT_VAR, 
				      id, mem_context));
    }
      break;

    case STOR_FUNC_VAR: {
      long id = ReadInt();
      MemoryContext mem_context = (MemoryContext)ReadInt();
      instrs.push_back(new StackInstr(line_num, STOR_FUNC_VAR, id, mem_context));
    }
      break;

    case STOR_FLOAT_VAR: {
      long id = ReadInt();
      long mem_context = ReadInt();
      instrs.push_back(new StackInstr(line_num, STOR_FLOAT_VAR, id, mem_context));
    }
      break;

    case COPY_INT_VAR: {
      long id = ReadInt();
      MemoryContext mem_context = (MemoryContext)ReadInt();
      instrs.push_back(new StackInstr(line_num, 
				      mem_context == LOCL ? COPY_LOCL_INT_VAR : COPY_CLS_INST_INT_VAR, 
				      id, mem_context));
    }
      break;

    case COPY_FLOAT_VAR: {
      long id = ReadInt();
      MemoryContext mem_context = (MemoryContext)ReadInt();
      instrs.push_back(new StackInstr(line_num, COPY_FLOAT_VAR, id, mem_context));
    }
      break;

    case LOAD_BYTE_ARY_ELM: {
      long dim = ReadInt();
      MemoryContext mem_context = (MemoryContext)ReadInt();
      instrs.push_back(new StackInstr(line_num, LOAD_BYTE_ARY_ELM, dim, mem_context));
    }
      break;

    case LOAD_CHAR_ARY_ELM: {
      long dim = ReadInt();
      MemoryContext mem_context = (MemoryContext)ReadInt();
      instrs.push_back(new StackInstr(line_num, LOAD_CHAR_ARY_ELM, dim, mem_context));
    }
      break;
      
    case LOAD_INT_ARY_ELM: {
      long dim = ReadInt();
      MemoryContext mem_context = (MemoryContext)ReadInt();
      instrs.push_back(new StackInstr(line_num, LOAD_INT_ARY_ELM, dim, mem_context));
    }
      break;

    case LOAD_FLOAT_ARY_ELM: {
      long dim = ReadInt();
      MemoryContext mem_context = (MemoryContext)ReadInt();
      instrs.push_back(new StackInstr(line_num, LOAD_FLOAT_ARY_ELM, dim, mem_context));
    }
      break;

    case STOR_BYTE_ARY_ELM: {
      long dim = ReadInt();
      MemoryContext mem_context = (MemoryContext)ReadInt();
      instrs.push_back(new StackInstr(line_num, STOR_BYTE_ARY_ELM, dim, mem_context));
    }
      break;

    case STOR_CHAR_ARY_ELM: {
      long dim = ReadInt();
      MemoryContext mem_context = (MemoryContext)ReadInt();
      instrs.push_back(new StackInstr(line_num, STOR_CHAR_ARY_ELM, dim, mem_context));
    }
      break;

    case STOR_INT_ARY_ELM: {
      long dim = ReadInt();
      MemoryContext mem_context = (MemoryContext)ReadInt();
      instrs.push_back(new StackInstr(line_num, STOR_INT_ARY_ELM, dim, mem_context));
    }
      break;

    case STOR_FLOAT_ARY_ELM: {
      long dim = ReadInt();
      long mem_context = ReadInt();
      instrs.push_back(new StackInstr(line_num, STOR_FLOAT_ARY_ELM, dim, mem_context));
    }
      break;

    case NEW_FLOAT_ARY: {
      long dim = ReadInt();
      instrs.push_back(new StackInstr(line_num, NEW_FLOAT_ARY, dim));
    }
      break;

    case NEW_INT_ARY: {
      long dim = ReadInt();
      instrs.push_back(new StackInstr(line_num, NEW_INT_ARY, dim));
    }
      break;

    case NEW_BYTE_ARY: {
      long dim = ReadInt();
      instrs.push_back(new StackInstr(line_num, NEW_BYTE_ARY, dim));

    }
      break;

    case NEW_CHAR_ARY: {
      long dim = ReadInt();
      instrs.push_back(new StackInstr(line_num, NEW_CHAR_ARY, dim));

    }
      break;

    case NEW_OBJ_INST: {
      long obj_id = ReadInt();
      instrs.push_back(new StackInstr(line_num, NEW_OBJ_INST, obj_id));
    }
      break;

    case DYN_MTHD_CALL: {
      long num_params = ReadInt();
      long rtrn_type = ReadInt();
      instrs.push_back(new StackInstr(line_num, DYN_MTHD_CALL, num_params, rtrn_type));
    }
      break;

    case MTHD_CALL: {
      long cls_id = ReadInt();
      long mthd_id = ReadInt();
      long is_native = ReadInt();
      instrs.push_back(new StackInstr(line_num, MTHD_CALL, cls_id, mthd_id, is_native));
    }
      break;

    case ASYNC_MTHD_CALL: {
      long cls_id = ReadInt();
      long mthd_id = ReadInt();
      long is_native = ReadInt();
      instrs.push_back(new StackInstr(line_num, ASYNC_MTHD_CALL, cls_id, mthd_id, is_native));
    }
      break;

    case LIB_OBJ_INST_CAST:
      wcerr << L">>> unsupported instruction for executable: LIB_OBJ_INST_CAST <<<" << endl;
      exit(1);

    case LIB_NEW_OBJ_INST:
      wcerr << L">>> unsupported instruction for executable: LIB_NEW_OBJ_INST <<<" << endl;
      exit(1);

    case LIB_MTHD_CALL:
      wcerr << L">>> unsupported instruction for executable: LIB_MTHD_CALL <<<" << endl;
      exit(1);

    case JMP: {
      long label = ReadInt();
      long cond = ReadInt();
      instrs.push_back(new StackInstr(line_num, JMP, label, cond));
    }
      break;

    case LBL: {
      long id = ReadInt();
      instrs.push_back(new StackInstr(line_num, LBL, id));
      method->AddLabel(id, index);
    }
      break;

    case OBJ_INST_CAST: {
      long to = ReadInt();
      instrs.push_back(new StackInstr(line_num, OBJ_INST_CAST, to));
    }
      break;

    case OBJ_TYPE_OF: {
      long check = ReadInt();
      instrs.push_back(new StackInstr(line_num, OBJ_TYPE_OF, check));
    }
      break;

    case OR_INT:
      instrs.push_back(new StackInstr(line_num, OR_INT));
      break;

    case AND_INT:
      instrs.push_back(new StackInstr(line_num, AND_INT));
      break;

    case ADD_INT:
      instrs.push_back(new StackInstr(line_num, ADD_INT));
      break;

    case CEIL_FLOAT:
      instrs.push_back(new StackInstr(line_num, CEIL_FLOAT));
      break;

    case CPY_BYTE_ARY:
      instrs.push_back(new StackInstr(line_num, CPY_BYTE_ARY));
      break;
      
    case CPY_CHAR_ARY:
      instrs.push_back(new StackInstr(line_num, CPY_CHAR_ARY));
      break;
      
    case CPY_INT_ARY:
      instrs.push_back(new StackInstr(line_num, CPY_INT_ARY));
      break;

    case CPY_FLOAT_ARY:
      instrs.push_back(new StackInstr(line_num, CPY_FLOAT_ARY));
      break;

    case FLOR_FLOAT:
      instrs.push_back(new StackInstr(line_num, FLOR_FLOAT));
      break;

    case SIN_FLOAT:
      instrs.push_back(new StackInstr(line_num, SIN_FLOAT));
      break;

    case COS_FLOAT:
      instrs.push_back(new StackInstr(line_num, COS_FLOAT));
      break;

    case TAN_FLOAT:
      instrs.push_back(new StackInstr(line_num, TAN_FLOAT));
      break;

    case ASIN_FLOAT:
      instrs.push_back(new StackInstr(line_num, ASIN_FLOAT));
      break;

    case ACOS_FLOAT:
      instrs.push_back(new StackInstr(line_num, ACOS_FLOAT));
      break;

    case ATAN_FLOAT:
      instrs.push_back(new StackInstr(line_num, ATAN_FLOAT));
      break;

    case LOG_FLOAT:
      instrs.push_back(new StackInstr(line_num, LOG_FLOAT));
      break;

    case POW_FLOAT:
      instrs.push_back(new StackInstr(line_num, POW_FLOAT));
      break;

    case SQRT_FLOAT:
      instrs.push_back(new StackInstr(line_num, SQRT_FLOAT));
      break;

    case RAND_FLOAT:
      instrs.push_back(new StackInstr(line_num, RAND_FLOAT));
      break;

    case F2I:
      instrs.push_back(new StackInstr(line_num, F2I));
      break;

    case I2F:
      instrs.push_back(new StackInstr(line_num, I2F));
      break;

    case SWAP_INT:
      instrs.push_back(new StackInstr(line_num, SWAP_INT));
      break;

    case POP_INT:
      instrs.push_back(new StackInstr(line_num, POP_INT));
      break;

    case POP_FLOAT:
      instrs.push_back(new StackInstr(line_num, POP_FLOAT));
      break;

    case LOAD_CLS_MEM:
      instrs.push_back(new StackInstr(line_num, LOAD_CLS_MEM));
      break;

    case LOAD_INST_MEM:
      instrs.push_back(new StackInstr(line_num, LOAD_INST_MEM));
      break;

    case LOAD_ARY_SIZE:
      instrs.push_back(new StackInstr(line_num, LOAD_ARY_SIZE));
      break;

    case SUB_INT:
      instrs.push_back(new StackInstr(line_num, SUB_INT));
      break;

    case MUL_INT:
      instrs.push_back(new StackInstr(line_num, MUL_INT));
      break;

    case DIV_INT:
      instrs.push_back(new StackInstr(line_num, DIV_INT));
      break;

    case MOD_INT:
      instrs.push_back(new StackInstr(line_num, MOD_INT));
      break;

    case BIT_AND_INT:
      instrs.push_back(new StackInstr(line_num, BIT_AND_INT));
      break;

    case BIT_OR_INT:
      instrs.push_back(new StackInstr(line_num, BIT_OR_INT));
      break;

    case BIT_XOR_INT:
      instrs.push_back(new StackInstr(line_num, BIT_XOR_INT));
      break;

    case EQL_INT:
      instrs.push_back(new StackInstr(line_num, EQL_INT));
      break;

    case NEQL_INT:
      instrs.push_back(new StackInstr(line_num, NEQL_INT));
      break;

    case LES_INT:
      instrs.push_back(new StackInstr(line_num, LES_INT));
      break;

    case GTR_INT:
      instrs.push_back(new StackInstr(line_num, GTR_INT));
      break;

    case LES_EQL_INT:
      instrs.push_back(new StackInstr(line_num, LES_EQL_INT));
      break;

    case LES_EQL_FLOAT:
      instrs.push_back(new StackInstr(line_num, LES_EQL_FLOAT));
      break;

    case GTR_EQL_INT:
      instrs.push_back(new StackInstr(line_num, GTR_EQL_INT));
      break;

    case GTR_EQL_FLOAT:
      instrs.push_back(new StackInstr(line_num, GTR_EQL_FLOAT));
      break;

    case ADD_FLOAT:
      instrs.push_back(new StackInstr(line_num, ADD_FLOAT));
      break;

    case SUB_FLOAT:
      instrs.push_back(new StackInstr(line_num, SUB_FLOAT));
      break;

    case MUL_FLOAT:
      instrs.push_back(new StackInstr(line_num, MUL_FLOAT));
      break;

    case DIV_FLOAT:
      instrs.push_back(new StackInstr(line_num, DIV_FLOAT));
      break;

    case EQL_FLOAT:
      instrs.push_back(new StackInstr(line_num, EQL_FLOAT));
      break;

    case NEQL_FLOAT:
      instrs.push_back(new StackInstr(line_num, NEQL_FLOAT));
      break;

    case LES_FLOAT:
      instrs.push_back(new StackInstr(line_num, LES_FLOAT));
      break;

    case GTR_FLOAT:
      instrs.push_back(new StackInstr(line_num, GTR_FLOAT));
      break;

    case LOAD_FLOAT_LIT:
      instrs.push_back(new StackInstr(line_num, LOAD_FLOAT_LIT,
				      ReadDouble()));
      break;

    case RTRN:
      if(is_debug) {
        instrs.push_back(new StackInstr(line_num + 1, RTRN));
      }
      else {
        instrs.push_back(new StackInstr(line_num, RTRN));
      }
      break;

    case DLL_LOAD:
      instrs.push_back(new StackInstr(line_num, DLL_LOAD));
      break;

    case DLL_UNLOAD:
      instrs.push_back(new StackInstr(line_num, DLL_UNLOAD));
      break;

    case DLL_FUNC_CALL:
      instrs.push_back(new StackInstr(line_num, DLL_FUNC_CALL));
      break;

    case THREAD_JOIN:
      instrs.push_back(new StackInstr(line_num, THREAD_JOIN));
      break;

    case THREAD_SLEEP:
      instrs.push_back(new StackInstr(line_num, THREAD_SLEEP));
      break;

    case THREAD_MUTEX:
      instrs.push_back(new StackInstr(line_num, THREAD_MUTEX));
      break;

    case CRITICAL_START:
      instrs.push_back(new StackInstr(line_num, CRITICAL_START));
      break;

    case CRITICAL_END:
      instrs.push_back(new StackInstr(line_num, CRITICAL_END));
      break;

    case TRAP: {
      long args = ReadInt();
      instrs.push_back(new StackInstr(line_num, TRAP, args));
    }
      break;

    case TRAP_RTRN: {
      long args = ReadInt();
      instrs.push_back(new StackInstr(line_num, TRAP_RTRN, args));
    }
      break;

    default: {
#ifdef _DEBUG
      InstructionType instr = (InstructionType)type;
      wcout << L">>> unknown instruction: id=" << instr << L" <<<" << endl;
#endif
      exit(1);
    }
      break;

    }
    // update
    type = ReadByte();
    index++;
  }

  // copy and set instructions
  StackInstr** mthd_instrs = new StackInstr*[instrs.size()];
  copy(instrs.begin(), instrs.end(), mthd_instrs);
  method->SetInstructions(mthd_instrs, instrs.size());
}
