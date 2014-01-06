/***************************************************************************
* Defines how the intermediate code is written to output files
*
* Copyright (c) 2008-2013, Randy Hollines
* All rights reserved.wstring
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* - Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
* - Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in
* the documentation and/or other materials provided with the distribution.
* - Neither the name of the Objeck team nor the names of its
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

#ifndef __TARGET_H__
#define __TARGET_H__

#include "types.h"
#include "linker.h"
#include "tree.h"
#include "../shared/instrs.h"
#include "../shared/version.h"

using namespace std;
using namespace instructions;

namespace backend {
  class IntermediateClass;

  /****************************
  * Intermediate class
  ****************************/
  class Intermediate {

  public:
    Intermediate() {
    }

    virtual ~Intermediate() {
    }

  protected:
    void WriteString(const wstring &in, ofstream* file_out) {
      string out;
      if(!UnicodeToBytes(in, out)) {
        wcerr << L">>> Unable to write unicode string <<<" << endl;
        exit(1);
      }
      WriteInt(out.size(), file_out);
      file_out->write(out.c_str(), out.size());
    }

    void WriteByte(char value, ofstream* file_out) {
      file_out->write(&value, sizeof(value));
    }

    void WriteInt(int value, ofstream* file_out) {
      file_out->write((char*)&value, sizeof(value));
    }

    void WriteChar(wchar_t value, ofstream* file_out) {
      string buffer;
      if(!CharacterToBytes(value, buffer)) {
        wcerr << L">>> Unable to write character <<<" << endl;
        exit(1);
      }

      // write bytes
      if(buffer.size()) {
        WriteInt(buffer.size(), file_out);
        file_out->write(buffer.c_str(), buffer.size());
      }
      else {
        WriteInt(0, file_out);
      }
    }

    void WriteDouble(FLOAT_VALUE value, ofstream* file_out) {
      file_out->write((char*)&value, sizeof(value));
    }
  };

  /****************************
  * IntermediateInstruction
  * class
  ****************************/
  class IntermediateInstruction : public Intermediate {
    friend class IntermediateFactory;
    InstructionType type;
    int operand;
    int operand2;
    int operand3;
    FLOAT_VALUE operand4;
    wstring operand5;
    wstring operand6;
    int line_num;

    IntermediateInstruction(int l, InstructionType t) {
      line_num = l;
      type = t;
    }

    IntermediateInstruction(int l, InstructionType t, int o1) {
      line_num = l;
      type = t;
      operand = o1;
    }

    IntermediateInstruction(int l, InstructionType t, int o1, int o2) {
      line_num = l;
      type = t;
      operand = o1;
      operand2 = o2;
    }

    IntermediateInstruction(int l, InstructionType t, int o1, int o2, int o3) {
      line_num = l;
      type = t;
      operand = o1;
      operand2 = o2;
      operand3 = o3;
    }

    IntermediateInstruction(int l, InstructionType t, FLOAT_VALUE o4) {
      line_num = l;
      type = t;
      operand4 = o4;
    }

    IntermediateInstruction(int l, InstructionType t, wstring o5) {
      line_num = l;
      type = t;
      operand5 = o5;
    }

    IntermediateInstruction(int l, InstructionType t, int o3, wstring o5, wstring o6) {
      line_num = l;
      type = t;
      operand3 = o3;
      operand5 = o5;
      operand6 = o6;
    }

    IntermediateInstruction(LibraryInstr* lib_instr) {
      type = lib_instr->GetType();
      line_num = lib_instr->GetLineNumber();
      operand = lib_instr->GetOperand();
      operand2 = lib_instr->GetOperand2();
      operand3 = lib_instr->GetOperand3();
      operand4 = lib_instr->GetOperand4();
      operand5 = lib_instr->GetOperand5();
      operand6 = lib_instr->GetOperand6();
    }

    ~IntermediateInstruction() {
    }

  public:
    InstructionType GetType() {
      return type;
    }

    int GetOperand() {
      return operand;
    }

    int GetOperand2() {
      return operand2;
    }

    FLOAT_VALUE GetOperand4() {
      return operand4;
    }

    void SetOperand3(int o3) {
      operand3 = o3;
    }

    void Write(bool is_debug, ofstream* file_out) {
      WriteByte((int)type, file_out);
      if(is_debug) {
        WriteInt(line_num, file_out);
      }
      switch(type) {
      case LOAD_INT_LIT:

      case NEW_FLOAT_ARY:
      case NEW_INT_ARY:
      case NEW_BYTE_ARY:
      case NEW_CHAR_ARY:
      case NEW_OBJ_INST:
      case OBJ_INST_CAST:
      case OBJ_TYPE_OF:
      case TRAP:
      case TRAP_RTRN:
        WriteInt(operand, file_out);
        break;

      case LOAD_CHAR_LIT:
        WriteChar(operand, file_out);
        break;

      case instructions::ASYNC_MTHD_CALL:
      case MTHD_CALL:
        WriteInt(operand, file_out);
        WriteInt(operand2, file_out);
        WriteInt(operand3, file_out);
        break;

      case LIB_NEW_OBJ_INST:
      case LIB_OBJ_INST_CAST:
        WriteString(operand5, file_out);
        break;

      case LIB_MTHD_CALL:
        WriteInt(operand3, file_out);
        WriteString(operand5, file_out);
        WriteString(operand6, file_out);
        break;

      case LIB_FUNC_DEF:
        WriteString(operand5, file_out);
        WriteString(operand6, file_out);
        break;

      case JMP:
      case DYN_MTHD_CALL:
      case LOAD_INT_VAR:
      case LOAD_FLOAT_VAR:
      case LOAD_FUNC_VAR:
      case STOR_INT_VAR:
      case STOR_FLOAT_VAR:
      case STOR_FUNC_VAR:
      case COPY_INT_VAR:
      case COPY_FLOAT_VAR:
      case COPY_FUNC_VAR:
      case LOAD_BYTE_ARY_ELM:
      case LOAD_CHAR_ARY_ELM:
      case LOAD_INT_ARY_ELM:
      case LOAD_FLOAT_ARY_ELM:
      case STOR_BYTE_ARY_ELM:
      case STOR_CHAR_ARY_ELM:
      case STOR_INT_ARY_ELM:
      case STOR_FLOAT_ARY_ELM:
        WriteInt(operand, file_out);
        WriteInt(operand2, file_out);
        break;

      case LOAD_FLOAT_LIT:
        WriteDouble(operand4, file_out);
        break;

      case LBL:
        WriteInt(operand, file_out);
        break;

      default:
        break;
      }
    }

    void Debug() {
      switch(type) {
      case SWAP_INT:
        wcout << L"SWAP_INT" << endl;
        break;

      case POP_INT:
        wcout << L"POP_INT" << endl;
        break;

      case POP_FLOAT:
        wcout << L"POP_FLOAT" << endl;
        break;

      case LOAD_INT_LIT:
        wcout << L"LOAD_INT_LIT: value=" << operand << endl;
        break;

      case LOAD_CHAR_LIT:
        wcout << L"LOAD_CHAR_LIT value=" << (wchar_t)operand << endl;
        break;

      case DYN_MTHD_CALL:
        wcout << L"DYN_MTHD_CALL num_params=" << operand 
          << L", rtrn_type=" << operand2 << endl;
        break;

      case SHL_INT:
        wcout << L"SHL_INT" << endl;
        break;

      case SHR_INT:
        wcout << L"SHR_INT" << endl;
        break;

      case LOAD_FLOAT_LIT:
        wcout << L"LOAD_FLOAT_LIT: value=" << operand4 << endl;
        break;

      case LOAD_FUNC_VAR:
        wcout << L"LOAD_FUNC_VAR: id=" << operand << L", local="
          << (operand2 == LOCL ? "true" : "false") << endl;
        break;

      case LOAD_INT_VAR:
        wcout << L"LOAD_INT_VAR: id=" << operand << L", local="
          << (operand2 == LOCL ? "true" : "false") << endl;
        break;

      case LOAD_FLOAT_VAR:
        wcout << L"LOAD_FLOAT_VAR: id=" << operand << L", local="
          << (operand2 == LOCL ? "true" : "false") << endl;
        break;

      case LOAD_BYTE_ARY_ELM:
        wcout << L"LOAD_BYTE_ARY_ELM: dimension=" << operand
          << L", local=" << (operand2 == LOCL ? "true" : "false") << endl;
        break;

      case LOAD_CHAR_ARY_ELM:
        wcout << L"LOAD_CHAR_ARY_ELM: dimension=" << operand
          << L", local=" << (operand2 == LOCL ? "true" : "false") << endl;
        break;

      case LOAD_INT_ARY_ELM:
        wcout << L"LOAD_INT_ARY_ELM: dimension=" << operand
          << L", local=" << (operand2 == LOCL ? "true" : "false") << endl;
        break;

      case LOAD_FLOAT_ARY_ELM:
        wcout << L"LOAD_FLOAT_ARY_ELM: dimension=" << operand
          << L", local=" << (operand2 == LOCL ? "true" : "false") << endl;
        break;

      case LOAD_CLS_MEM:
        wcout << L"LOAD_CLS_MEM" << endl;
        break;

      case LOAD_INST_MEM:
        wcout << L"LOAD_INST_MEM" << endl;
        break;

      case STOR_FUNC_VAR:
        wcout << L"STOR_FUNC_VAR: id=" << operand << L", local="
          << (operand2 == LOCL ? "true" : "false") << endl;
        break;

      case STOR_INT_VAR:
        wcout << L"STOR_INT_VAR: id=" << operand << L", local="
          << (operand2 == LOCL ? "true" : "false") << endl;
        break;

      case STOR_FLOAT_VAR:
        wcout << L"STOR_FLOAT_VAR: id=" << operand << L", local="
          << (operand2 == LOCL ? "true" : "false") << endl;
        break;

      case COPY_FUNC_VAR:
        wcout << L"COPY_FUNC_VAR: id=" << operand << L", local="
          << (operand2 == LOCL ? "true" : "false") << endl;
        break;

      case COPY_INT_VAR:
        wcout << L"COPY_INT_VAR: id=" << operand << L", local="
          << (operand2 == LOCL ? "true" : "false") << endl;
        break;

      case COPY_FLOAT_VAR:
        wcout << L"COPY_FLOAT_VAR: id=" << operand << L", local="
          << (operand2 == LOCL ? "true" : "false") << endl;
        break;

      case STOR_BYTE_ARY_ELM:
        wcout << L"STOR_BYTE_ARY_ELM: dimension=" << operand
          << L", local=" << (operand2 == LOCL ? "true" : "false") << endl;
        break;

      case STOR_CHAR_ARY_ELM:
        wcout << L"STOR_CHAR_ARY_ELM: dimension=" << operand
          << L", local=" << (operand2 == LOCL ? "true" : "false") << endl;
        break;

      case STOR_INT_ARY_ELM:
        wcout << L"STOR_INT_ARY_ELM: dimension=" << operand
          << L", local=" << (operand2 == LOCL ? "true" : "false") << endl;
        break;

      case STOR_FLOAT_ARY_ELM:
        wcout << L"STOR_FLOAT_ARY_ELM: dimension=" << operand
          << L", local=" << (operand2 == LOCL ? "true" : "false") << endl;
        break;

      case instructions::ASYNC_MTHD_CALL:
        wcout << L"ASYNC_MTHD_CALL: class=" << operand << L", method="
          << operand2 << L"; native=" << (operand3 ? "true" : "false") << endl;
        break;

      case instructions::DLL_LOAD:
        wcout << L"DLL_LOAD" << endl;
        break;

      case instructions::DLL_UNLOAD:
        wcout << L"DLL_UNLOAD" << endl;
        break;

      case instructions::DLL_FUNC_CALL:
        wcout << L"DLL_FUNC_CALL" << endl;
        break;

      case instructions::THREAD_JOIN:
        wcout << L"THREAD_JOIN" << endl;
        break;

      case instructions::THREAD_SLEEP:
        wcout << L"THREAD_SLEEP" << endl;
        break;

      case instructions::THREAD_MUTEX:
        wcout << L"THREAD_MUTEX" << endl;
        break;

      case CRITICAL_START:
        wcout << L"CRITICAL_START" << endl;
        break;

      case CRITICAL_END:
        wcout << L"CRITICAL_END" << endl;
        break;

      case AND_INT:
        wcout << L"AND_INT" << endl;
        break;

      case OR_INT:
        wcout << L"OR_INT" << endl;
        break;

      case ADD_INT:
        wcout << L"ADD_INT" << endl;
        break;

      case SUB_INT:
        wcout << L"SUB_INT" << endl;
        break;

      case MUL_INT:
        wcout << L"MUL_INT" << endl;
        break;

      case DIV_INT:
        wcout << L"DIV_INT" << endl;
        break;

      case MOD_INT:
        wcout << L"MOD_INT" << endl;
        break;

      case BIT_AND_INT:
        wcout << L"BIT_AND_INT" << endl;
        break;

      case BIT_OR_INT:
        wcout << L"BIT_OR_INT" << endl;
        break;

      case BIT_XOR_INT:
        wcout << L"BIT_XOR_INT" << endl;
        break;

      case EQL_INT:
        wcout << L"EQL_INT" << endl;
        break;

      case NEQL_INT:
        wcout << L"NEQL_INT" << endl;
        break;

      case LES_INT:
        wcout << L"LES_INT" << endl;
        break;

      case GTR_INT:
        wcout << L"GTR_INT" << endl;
        break;

      case LES_EQL_INT:
        wcout << L"LES_EQL_INT" << endl;
        break;

      case GTR_EQL_INT:
        wcout << L"GTR_EQL_INT" << endl;
        break;

      case ADD_FLOAT:
        wcout << L"ADD_FLOAT" << endl;
        break;

      case SUB_FLOAT:
        wcout << L"SUB_FLOAT" << endl;
        break;

      case MUL_FLOAT:
        wcout << L"MUL_FLOAT" << endl;
        break;

      case DIV_FLOAT:
        wcout << L"DIV_FLOAT" << endl;
        break;

      case EQL_FLOAT:
        wcout << L"EQL_FLOAT" << endl;
        break;

      case NEQL_FLOAT:
        wcout << L"NEQL_FLOAT" << endl;
        break;

      case LES_EQL_FLOAT:
        wcout << L"LES_EQL_FLOAT" << endl;
        break;

      case LES_FLOAT:
        wcout << L"LES_FLOAT" << endl;
        break;

      case GTR_FLOAT:
        wcout << L"GTR_FLOAT" << endl;
        break;

      case GTR_EQL_FLOAT:
        wcout << L"LES_EQL_FLOAT" << endl;
        break;

      case instructions::FLOR_FLOAT:
        wcout << L"FLOR_FLOAT" << endl;
        break;

      case instructions::LOAD_ARY_SIZE:
        wcout << L"LOAD_ARY_SIZE" << endl;
        break;

      case instructions::CPY_BYTE_ARY:
        wcout << L"CPY_BYTE_ARY" << endl;
        break;

      case instructions::CPY_CHAR_ARY:
        wcout << L"CPY_CHAR_ARY" << endl;
        break;

      case instructions::CPY_INT_ARY:
        wcout << L"CPY_INT_ARY" << endl;
        break;

      case instructions::CPY_FLOAT_ARY:
        wcout << L"CPY_FLOAT_ARY" << endl;
        break;

      case instructions::CEIL_FLOAT:
        wcout << L"CEIL_FLOAT" << endl;
        break;

      case instructions::RAND_FLOAT:
        wcout << L"RAND_FLOAT" << endl;
        break;

      case instructions::SIN_FLOAT:
        wcout << L"SIN_FLOAT" << endl;
        break;

      case instructions::COS_FLOAT:
        wcout << L"COS_FLOAT" << endl;
        break;

      case instructions::TAN_FLOAT:
        wcout << L"TAN_FLOAT" << endl;
        break;

      case instructions::ASIN_FLOAT:
        wcout << L"ASIN_FLOAT" << endl;
        break;

      case instructions::ACOS_FLOAT:
        wcout << L"ACOS_FLOAT" << endl;
        break;

      case instructions::ATAN_FLOAT:
        wcout << L"ATAN_FLOAT" << endl;
        break;

      case instructions::LOG_FLOAT:
        wcout << L"LOG_FLOAT" << endl;
        break;

      case instructions::POW_FLOAT:
        wcout << L"POW_FLOAT" << endl;
        break;

      case instructions::SQRT_FLOAT:
        wcout << L"SQRT_FLOAT" << endl;
        break;

      case I2F:
        wcout << L"I2F" << endl;
        break;

      case F2I:
        wcout << L"F2I" << endl;
        break;

      case RTRN:
        wcout << L"RTRN" << endl;
        break;

      case MTHD_CALL:
        wcout << L"MTHD_CALL: class=" << operand << L", method="
          << operand2 << L"; native=" << (operand3 ? "true" : "false") << endl;
        break;

      case LIB_NEW_OBJ_INST:
        wcout << L"LIB_NEW_OBJ_INST: class='" << operand5 << L"'" << endl;
        break;

      case LIB_OBJ_INST_CAST:
        wcout << L"LIB_OBJ_INST_CAST: to_class='" << operand5 << L"'" << endl;
        break;

      case LIB_MTHD_CALL:
        wcout << L"LIB_MTHD_CALL: class='" << operand5 << L"', method='"
          << operand6 << L"'; native=" << (operand3 ? "true" : "false") << endl;
        break;

      case LIB_FUNC_DEF:
        wcout << L"LIB_FUNC_DEF: class='" << operand5 << L"', method='" 
          << operand6 << L"'" << endl;
        break;

      case LBL:
        wcout << L"LBL: id=" << operand << endl;
        break;

      case JMP:
        if(operand2 == -1) {
          wcout << L"JMP: id=" << operand << endl;
        } else {
          wcout << L"JMP: id=" << operand << L" conditional="
            << (operand2 ? "true" : "false") << endl;
        }
        break;

      case OBJ_INST_CAST:
        wcout << L"OBJ_INST_CAST: to=" << operand << endl;
        break;

      case OBJ_TYPE_OF:
        wcout << L"OBJ_TYPE_OF: check=" << operand << endl;
        break;

      case NEW_FLOAT_ARY:
        wcout << L"NEW_FLOAT_ARY: dimension=" << operand << endl;
        break;

      case NEW_INT_ARY:
        wcout << L"NEW_INT_ARY: dimension=" << operand << endl;
        break;

      case NEW_BYTE_ARY:
        wcout << L"NEW_BYTE_ARY: dimension=" << operand << endl;
        break;

      case NEW_CHAR_ARY:
        wcout << L"NEW_CHAR_ARY: dimension=" << operand << endl;
        break;

      case NEW_OBJ_INST:
        wcout << L"NEW_OBJ_INST: class=" << operand << endl;
        break;

      case TRAP:
        wcout << L"TRAP: args=" << operand << endl;
        break;

      case TRAP_RTRN:
        wcout << L"TRAP_RTRN: args=" << operand << endl;
        break;

      default:
        break;
      }
    }
  };

  /****************************
  * IntermediateFactory
  * class
  ****************************/
  class IntermediateFactory {
    static IntermediateFactory* instance;
    vector<IntermediateInstruction*> instructions;

  public:
    static IntermediateFactory* Instance();

    void Clear() {
      while(!instructions.empty()) {
        IntermediateInstruction* tmp = instructions.front();
        instructions.erase(instructions.begin());
        // delete
        delete tmp;
        tmp = NULL;
      }

      delete instance;
      instance = NULL;
    }

    IntermediateInstruction* MakeInstruction(int l, InstructionType t) {
      IntermediateInstruction* tmp = new IntermediateInstruction(l, t);
      instructions.push_back(tmp);
      return tmp;
    }

    IntermediateInstruction* MakeInstruction(int l, InstructionType t, int o1) {
      IntermediateInstruction* tmp = new IntermediateInstruction(l, t, o1);
      instructions.push_back(tmp);
      return tmp;
    }

    IntermediateInstruction* MakeInstruction(int l, InstructionType t, int o1, int o2) {
      IntermediateInstruction* tmp = new IntermediateInstruction(l, t, o1, o2);
      instructions.push_back(tmp);
      return tmp;
    }

    IntermediateInstruction* MakeInstruction(int l, InstructionType t, int o1, int o2, int o3) {
      IntermediateInstruction* tmp = new IntermediateInstruction(l, t, o1, o2, o3);
      instructions.push_back(tmp);
      return tmp;
    }

    IntermediateInstruction* MakeInstruction(int l, InstructionType t, FLOAT_VALUE o4) {
      IntermediateInstruction* tmp = new IntermediateInstruction(l, t, o4);
      instructions.push_back(tmp);
      return tmp;
    }

    IntermediateInstruction* MakeInstruction(int l, InstructionType t, wstring o5) {
      IntermediateInstruction* tmp = new IntermediateInstruction(l, t, o5);
      instructions.push_back(tmp);
      return tmp;
    }

    IntermediateInstruction* MakeInstruction(int l, InstructionType t, int o3, wstring o5, wstring o6) {
      IntermediateInstruction* tmp = new IntermediateInstruction(l, t, o3, o5, o6);
      instructions.push_back(tmp);
      return tmp;
    }

    IntermediateInstruction* MakeInstruction(LibraryInstr* lib_instr) {
      IntermediateInstruction* tmp = new IntermediateInstruction(lib_instr);
      instructions.push_back(tmp);
      return tmp;
    }
  };

  /****************************
  * IntermediateBlock class
  ****************************/
  class IntermediateBlock : public Intermediate {
    vector<IntermediateInstruction*> instructions;

  public:
    IntermediateBlock() {
    }

    ~IntermediateBlock() {
    }

    void AddInstruction(IntermediateInstruction* i) {
      instructions.push_back(i);
    }

    vector<IntermediateInstruction*> GetInstructions() {
      return instructions;
    }

    int GetSize() {
      return instructions.size();
    }

    void Clear() {
      instructions.clear();
    }

    bool IsEmpty() {
      return instructions.size() == 0;
    }

    void Write(bool is_debug, ofstream* file_out) {
      for(size_t i = 0; i < instructions.size(); ++i) {
        instructions[i]->Write(is_debug, file_out);
      }
    }

    void Debug() {
      if(instructions.size() > 0) {
        for(size_t i = 0; i < instructions.size(); ++i) {
          instructions[i]->Debug();
        }
        wcout << L"--" << endl;
      }
    }
  };

  /****************************
  * IntermediateMethod class
  ****************************/
  class IntermediateMethod : public Intermediate {
    int id;
    wstring name;
    wstring rtrn_name;
    int space;
    int params;
    frontend::MethodType type;
    bool is_native;
    bool is_function;
    bool is_lib;
    bool is_virtual;
    bool has_and_or;
    int instr_count;
    vector<IntermediateBlock*> blocks;
    IntermediateDeclarations* entries;
    IntermediateClass* klass;
    map<IntermediateMethod*, int> registered_inlined_mthds; // TODO: remove

  public:
    IntermediateMethod(int i, const wstring &n, bool v, bool h, const wstring &r,
      frontend::MethodType t, bool nt, bool f, int c, int p,
      IntermediateDeclarations* e, IntermediateClass* k) {
        id = i;
        name = n;
        is_virtual = v;
        has_and_or = h;
        rtrn_name = r;
        type = t;
        is_native = nt;
        is_function = f;
        space = c;
        params = p;
        entries = e;
        is_lib = false;
        klass = k;
        instr_count = 0;
    }

    IntermediateMethod(LibraryMethod* lib_method, IntermediateClass* k) {
      // set attributes
      id = lib_method->GetId();
      name = lib_method->GetName();
      is_virtual = lib_method->IsVirtual();
      has_and_or = lib_method->HasAndOr();
      rtrn_name = lib_method->GetEncodedReturn();
      type = lib_method->GetMethodType();
      is_native = lib_method->IsNative();
      is_function = lib_method->IsStatic();
      space = lib_method->GetSpace();
      params = lib_method->GetNumParams();
      entries = lib_method->GetEntries();
      is_lib = true;
      instr_count = 0;
      klass = k;
      // process instructions
      IntermediateBlock* block = new IntermediateBlock;
      vector<LibraryInstr*> lib_instructions = lib_method->GetInstructions();
      for(size_t i = 0; i < lib_instructions.size(); ++i) {
        block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(lib_instructions[i]));
      }
      AddBlock(block);
    }

    ~IntermediateMethod() {
      // clean up
      while(!blocks.empty()) {
        IntermediateBlock* tmp = blocks.front();
        blocks.erase(blocks.begin());
        // delete
        delete tmp;
        tmp = NULL;
      }

      if(entries) {
        delete entries;
        entries = NULL;
      }
    }

    int GetId() {
      return id;
    }

    IntermediateClass* GetClass() {
      return klass;
    }

    int GetSpace() {
      return space;
    }

    void SetSpace(int s) {
      space = s;
    }

    wstring GetName() {
      return name;
    }

    bool IsVirtual() {
      return is_virtual;
    }

    bool IsLibrary() {
      return is_lib;
    }

    void AddBlock(IntermediateBlock* b) {
      instr_count += b->GetSize();
      blocks.push_back(b);
    }

    int GetInstructionCount() {
      return instr_count;
    }

    int GetNumParams() {
      return params;
    }

    vector<IntermediateBlock*> GetBlocks() {
      return blocks;
    }

    void SetBlocks(vector<IntermediateBlock*> b) {
      blocks = b;
    }

    void Write(bool is_debug, ofstream* file_out) {
      // write attributes
      WriteInt(id, file_out);
      WriteInt(type, file_out);
      WriteInt(is_virtual, file_out);
      WriteInt(has_and_or, file_out);
      WriteInt(is_native, file_out);
      WriteInt(is_function, file_out);
      WriteString(name, file_out);
      WriteString(rtrn_name, file_out);

      // write local space size
      WriteInt(params, file_out);
      WriteInt(space, file_out);
      entries->Write(is_debug, file_out);

      // write statements
      for(size_t i = 0; i < blocks.size(); ++i) {
        blocks[i]->Write(is_debug, file_out);
      }
      WriteByte(END_STMTS, file_out);
    }

    void Debug() {
      wcout << L"---------------------------------------------------------" << endl;
      wcout << L"Method: id=" << id << L"; name='" << name << L"'; return='" << rtrn_name
        << L"';\n  blocks=" << blocks.size() << L"; is_function=" << is_function << L"; num_params="
        << params << L"; mem_size=" << space << endl;
      wcout << L"---------------------------------------------------------" << endl;
      entries->Debug();
      wcout << L"---------------------------------------------------------" << endl;
      for(size_t i = 0; i < blocks.size(); ++i) {
        blocks[i]->Debug();
      }
    }
  };

  /****************************
  * IntermediateClass class
  ****************************/
  class IntermediateClass : public Intermediate {
    int id;
    wstring name;
    int pid;
    vector<int> interface_ids;
    wstring parent_name;
    vector<wstring> interface_names;
    int cls_space;
    int inst_space;
    vector<IntermediateBlock*> blocks;
    vector<IntermediateMethod*> methods;
    map<int, IntermediateMethod*> method_map;
    IntermediateDeclarations* cls_entries;
    IntermediateDeclarations* inst_entries;
    bool is_lib;
    bool is_interface;
    bool is_virtual;
    bool is_debug;
    wstring file_name;
    
  public:
    IntermediateClass(int i, const wstring &n, int pi, const wstring &p, 
		      vector<int> infs, vector<wstring> in, bool is_inf, 
		      bool is_vrtl, int cs, int is, IntermediateDeclarations* ce, 
		      IntermediateDeclarations* ie, const wstring &fn, bool d) {
        id = i;
        name = n;
        pid = pi;
        parent_name = p;
        interface_ids = infs;
        interface_names = in;
        is_interface = is_inf;
        is_virtual = is_vrtl;
        cls_space = cs;
        inst_space = is;
        cls_entries = ce;
        inst_entries = ie;
        is_lib = false;
        is_debug = d;
        file_name = fn;
    }

    IntermediateClass(LibraryClass* lib_klass) {
      // set attributes
      id = lib_klass->GetId();
      name = lib_klass->GetName();
      pid = -1;
      interface_ids = lib_klass->GetInterfaceIds();
      parent_name = lib_klass->GetParentName();
      interface_names = lib_klass->GetInterfaceNames();
      is_interface = lib_klass->IsInterface();
      is_virtual = lib_klass->IsVirtual();
      is_debug = lib_klass->IsDebug();
      cls_space = lib_klass->GetClassSpace();
      inst_space = lib_klass->GetInstanceSpace();
      cls_entries = lib_klass->GetClassEntries();
      inst_entries = lib_klass->GetInstanceEntries();

      // process methods
      map<const wstring, LibraryMethod*> lib_methods = lib_klass->GetMethods();
      map<const wstring, LibraryMethod*>::iterator mthd_iter;
      for(mthd_iter = lib_methods.begin(); mthd_iter != lib_methods.end(); ++mthd_iter) {
        LibraryMethod* lib_method = mthd_iter->second;
        IntermediateMethod* imm_method = new IntermediateMethod(lib_method, this);
        AddMethod(imm_method);
      }

      file_name = lib_klass->GetFileName();
      is_lib = true;
    }

    ~IntermediateClass() {
      // clean up
      while(!blocks.empty()) {
        IntermediateBlock* tmp = blocks.front();
        blocks.erase(blocks.begin());
        // delete
        delete tmp;
        tmp = NULL;
      }
      // clean up
      while(!methods.empty()) {
        IntermediateMethod* tmp = methods.front();
        methods.erase(methods.begin());
        // delete
        delete tmp;
        tmp = NULL;
      }

      // clean up
      if(cls_entries) {
        delete cls_entries;
        cls_entries = NULL;
      }

      if(inst_entries) {
        delete inst_entries;
        inst_entries = NULL;
      }
    }

    int GetId() {
      return id;
    }

    const wstring& GetName() {
      return name;
    }

    bool IsLibrary() {
      return is_lib;
    }

    int GetInstanceSpace() {
      return inst_space;
    }

    void SetInstanceSpace(int s) {
      inst_space = s;
    }

    int GetClassSpace() {
      return cls_space;
    }

    void SetClassSpace(int s) {
      cls_space = s;
    }

    void AddMethod(IntermediateMethod* m) {
      methods.push_back(m);
      method_map.insert(pair<int, IntermediateMethod*>(m->GetId(), m));
    }

    void AddBlock(IntermediateBlock* b) {
      blocks.push_back(b);
    }

    IntermediateMethod* GetMethod(int id) {
      map<int, IntermediateMethod*>::iterator result = method_map.find(id);
#ifdef _DEBUG
      assert(result != method_map.end());
#endif
      return result->second;
    }

    vector<IntermediateMethod*> GetMethods() {
      return methods;
    }

    void Write(ofstream* file_out) {
      // write id and name
      WriteInt(id, file_out);
      WriteString(name, file_out);
      WriteInt(pid, file_out);
      WriteString(parent_name, file_out);

      // interface ids
      WriteInt(interface_ids.size(), file_out);
      for(size_t i = 0; i < interface_ids.size(); ++i) {
        WriteInt(interface_ids[i], file_out);
      }

      // interface names
      WriteInt(interface_names.size(), file_out);
      for(size_t i = 0; i < interface_names.size(); ++i) {
        WriteString(interface_names[i], file_out);
      }

      WriteInt(is_interface, file_out);
      WriteInt(is_virtual, file_out);
      WriteInt(is_debug, file_out);
      if(is_debug) {
        WriteString(file_name, file_out);
      }

      // write local space size
      WriteInt(cls_space, file_out);
      WriteInt(inst_space, file_out);
      cls_entries->Write(is_debug, file_out);
      inst_entries->Write(is_debug, file_out);

      // write methods
      WriteInt((int)methods.size(), file_out);
      for(size_t i = 0; i < methods.size(); ++i) {
        methods[i]->Write(is_debug, file_out);
      }
    }

    void Debug() {
      wcout << L"=========================================================" << endl;
      wcout << L"Class: id=" << id << L"; name='" << name << L"'; parent='" << parent_name
            << L"'; pid=" << pid << L";\n interface=" << (is_interface ? L"true" : L"false") 
            << L"; virtual=" << is_virtual << L"; num_methods=" << methods.size() 
            << L"; class_mem_size=" << cls_space << L";\n instance_mem_size=" 
            << inst_space << L"; is_debug=" << (is_debug  ? L"true" : L"false") << endl;      
      wcout << endl << "Interfaces:" << endl;
      for(size_t i = 0; i < interface_names.size(); ++i) {
        wcout << L"\t" << interface_names[i] << endl;
      }      
      wcout << L"=========================================================" << endl;
      cls_entries->Debug();
      wcout << L"---------------------------------------------------------" << endl;
      inst_entries->Debug();
      wcout << L"=========================================================" << endl;
      for(size_t i = 0; i < blocks.size(); ++i) {
        blocks[i]->Debug();
      }

      for(size_t i = 0; i < methods.size(); ++i) {
        methods[i]->Debug();
      }
    }
  };

  /****************************
  * IntermediateEnumItem class
  ****************************/
  class IntermediateEnumItem : public Intermediate {
    wstring name;
    INT_VALUE id;

  public:
    IntermediateEnumItem(const wstring &n, const INT_VALUE i) {
      name = n;
      id = i;
    }

    IntermediateEnumItem(LibraryEnumItem* i) {
      name = i->GetName();
      id = i->GetId();
    }

    void Write(ofstream* file_out) {
      WriteString(name, file_out);
      WriteInt(id, file_out);
    }

    void Debug() {
      wcout << L"Item: name='" << name << L"'; id='" << id << endl;
    }
  };

  /****************************
  * IntermediateEnum class
  ****************************/
  class IntermediateEnum : public Intermediate {
    wstring name;
    INT_VALUE offset;
    vector<IntermediateEnumItem*> items;

  public:
    IntermediateEnum(const wstring &n, const INT_VALUE o) {
      name = n;
      offset = o;
    }

    IntermediateEnum(LibraryEnum* e) {
      name = e->GetName();
      offset = e->GetOffset();
      // write items
      map<const wstring, LibraryEnumItem*> items = e->GetItems();
      map<const wstring, LibraryEnumItem*>::iterator iter;
      for(iter = items.begin(); iter != items.end(); ++iter) {
        LibraryEnumItem* lib_enum_item = iter->second;
        IntermediateEnumItem* imm_enum_item = new IntermediateEnumItem(lib_enum_item);
        AddItem(imm_enum_item);
      }
    }

    ~IntermediateEnum() {
      while(!items.empty()) {
        IntermediateEnumItem* tmp = items.front();
        items.erase(items.begin());
        // delete
        delete tmp;
        tmp = NULL;
      }
    }

    void AddItem(IntermediateEnumItem* i) {
      items.push_back(i);
    }

    void Write(ofstream* file_out) {
      WriteString(name, file_out);
      WriteInt(offset, file_out);
      // write items
      WriteInt((int)items.size(), file_out);
      for(size_t i = 0; i < items.size(); ++i) {
        items[i]->Write(file_out);
      }
    }

    void Debug() {
      wcout << L"=========================================================" << endl;
      wcout << L"Enum: name='" << name << L"'; items=" << items.size() << endl;
      wcout << L"=========================================================" << endl;

      for(size_t i = 0; i < items.size(); ++i) {
        items[i]->Debug();
      }
    }
  };

  /****************************
  * IntermediateProgram class
  ****************************/
  class IntermediateProgram : public Intermediate {
    int class_id;
    int method_id;
    vector<IntermediateEnum*> enums;
    vector<IntermediateClass*> classes;
    map<int, IntermediateClass*> class_map;
    vector<wstring> char_strings;
    vector<frontend::IntStringHolder*> int_strings;
    vector<frontend::FloatStringHolder*> float_strings;
    vector<wstring> bundle_names;
    int num_src_classes;
    int num_lib_classes;
    int string_cls_id;

  public:
    IntermediateProgram() {
      num_src_classes = num_lib_classes = 0;
      string_cls_id = -1;
    }

    ~IntermediateProgram() {
      // clean up
      while(!enums.empty()) {
        IntermediateEnum* tmp = enums.front();
        enums.erase(enums.begin());
        // delete
        delete tmp;
        tmp = NULL;
      }

      while(!classes.empty()) {
        IntermediateClass* tmp = classes.front();
        classes.erase(classes.begin());
        // delete
        delete tmp;
        tmp = NULL;
      }

      while(!int_strings.empty()) {
        frontend::IntStringHolder* tmp = int_strings.front();
        delete[] tmp->value;
        tmp->value = NULL;
        int_strings.erase(int_strings.begin());
        // delete
        delete tmp;
        tmp = NULL;
      }

      while(!float_strings.empty()) {
        frontend::FloatStringHolder* tmp = float_strings.front();
        delete[] tmp->value;
        tmp->value = NULL;
        float_strings.erase(float_strings.begin());
        // delete
        delete tmp;
        tmp = NULL;
      }

      IntermediateFactory::Instance()->Clear();
    }

    void AddClass(IntermediateClass* c) {
      classes.push_back(c);
      class_map.insert(pair<int, IntermediateClass*>(c->GetId(), c));
    }

    IntermediateClass* GetClass(int id) {
      map<int, IntermediateClass*>::iterator result = class_map.find(id);
#ifdef _DEBUG
      assert(result != class_map.end());
#endif
      return result->second;
    }

    void AddEnum(IntermediateEnum* e) {
      enums.push_back(e);
    }

    vector<IntermediateClass*> GetClasses() {
      return classes;
    }

    void SetCharStrings(vector<wstring> s) {
      char_strings = s;
    }

    void SetIntStrings(vector<frontend::IntStringHolder*> s) {
      int_strings = s;
    }

    void SetFloatStrings(vector<frontend::FloatStringHolder*> s) {
      float_strings = s;
    }

    void SetStartIds(int c, int m) {
      class_id = c;
      method_id = m;
    }

    int GetStartClassId() {
      return class_id;
    }

    int GetStartMethodId() {
      return method_id;
    }

    void SetBundleNames(vector<wstring> n) {
      bundle_names = n;
    }

    void SetStringClassId(int i) {
      string_cls_id = i;
    }

    void Write(ofstream* file_out, bool is_lib, bool is_debug, bool is_web) {
      // version
      WriteInt(VER_NUM, file_out);

      // magic number
      if(is_lib) {
        WriteInt(MAGIC_NUM_LIB, file_out);
      } 
      else if(is_web) {
        WriteInt(MAGIC_NUM_WEB, file_out);
      } 
      else {
        WriteInt(MAGIC_NUM_EXE, file_out);
      }    

      // write wstring id
      if(!is_lib) {
#ifdef _DEBUG
        assert(string_cls_id > 0);
#endif
        WriteInt(string_cls_id, file_out);
      }

      // write float strings
      WriteInt((int)float_strings.size(), file_out);
      for(size_t i = 0; i < float_strings.size(); ++i) {
        frontend::FloatStringHolder* holder = float_strings[i];
        WriteInt(holder->length, file_out);
        for(int j = 0; j < holder->length; j++) {
          WriteDouble(holder->value[j], file_out);
        }
      }
      // write int strings
      WriteInt((int)int_strings.size(), file_out);
      for(size_t i = 0; i < int_strings.size(); ++i) {
        frontend::IntStringHolder* holder = int_strings[i];
        WriteInt(holder->length, file_out);
        for(int j = 0; j < holder->length; j++) {
          WriteInt(holder->value[j], file_out);
        }
      }
      // write char strings
      WriteInt((int)char_strings.size(), file_out);
      for(size_t i = 0; i < char_strings.size(); ++i) {
        WriteString(char_strings[i], file_out);
      }

      // write bundle names
      if(is_lib) {
        WriteInt((int)bundle_names.size(), file_out);
        for(size_t i = 0; i < bundle_names.size(); ++i) {
          WriteString(bundle_names[i], file_out);
        }
      }

      // program start
      if(!is_lib) {
        WriteInt(class_id, file_out);
        WriteInt(method_id, file_out);
      }
      // program enums
      WriteInt((int)enums.size(), file_out);
      for(size_t i = 0; i < enums.size(); ++i) {
        enums[i]->Write(file_out);
      }
      // program classes
      WriteInt((int)classes.size(), file_out);
      for(size_t i = 0; i < classes.size(); ++i) {
        if(classes[i]->IsLibrary()) {
          num_lib_classes++;
        } else {
          num_src_classes++;
        }
        classes[i]->Write(file_out);
      }

      wcout << L"Compiled " << num_src_classes
        << (num_src_classes > 1 ? " source classes" : " source class");
      if(is_debug) {
        wcout << " with debug symbols";
      }
      wcout << L'.' << endl;

      wcout << L"Linked " << num_lib_classes
        << (num_lib_classes > 1 ? " library classes." : " library class.")  << endl;
    }

    void Debug() {
      wcout << L"Strings:" << endl;
      for(size_t i = 0; i < char_strings.size(); ++i) {
        wcout << L"wstring id=" << i << L", size='" << ToString(char_strings[i].size()) << L"'" << endl;
      }
      wcout << endl;

      wcout << L"Program: enums=" << enums.size() << L", classes="
        << classes.size() << L"; start=" << class_id << L"," << method_id << endl;
      // enums
      for(size_t i = 0; i < enums.size(); ++i) {
        enums[i]->Debug();
      }
      // classes
      for(size_t i = 0; i < classes.size(); ++i) {
        classes[i]->Debug();
      }
    }

    inline wstring ToString(int v) {
      wostringstream str;
      str << v;
      return str.str();
    }
  };

  /****************************
  * TargetEmitter class
  ****************************/
  class TargetEmitter {
    IntermediateProgram* program;
    wstring file_name;
    bool is_lib;
    bool is_debug;
    bool is_web;

    bool EndsWith(wstring const &str, wstring const &ending) {
      if(str.length() >= ending.length()) {
        return str.compare(str.length() - ending.length(), ending.length(), ending) == 0;
      } 

      return false;
    }

  public:
    TargetEmitter(IntermediateProgram* p, bool l, bool d, bool w, const wstring &n) {
      program = p;
      is_lib = l;
      is_debug = d;
      is_web = w;
      file_name = n;
    }

    ~TargetEmitter() {
      delete program;
      program = NULL;
    }

    void Emit();
  };
}

#endif
