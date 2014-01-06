/***************************************************************************
 * Language parser.
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

#ifndef __PARSER_H__
#define __PARSER_H__

#include "scanner.h"

using namespace frontend;

#define SECOND_INDEX 1
#define THIRD_INDEX 2
#define DEFAULT_BUNDLE_NAME L"Default"

/****************************
 * Parsers source files.
 ****************************/
class Parser {
  ParsedProgram* program;
  ParsedBundle* current_bundle;
  Class* current_class;
  Method* current_method;
  Method* prev_method;
  Scanner* scanner;
  SymbolTableManager* symbol_table;
  map<ScannerTokenType, wstring> error_msgs;
  map<int, wstring> errors;
  wstring src_path;
  wstring run_prgm;
  unsigned int anonymous_class_id;
  
  inline void NextToken() {
    scanner->NextToken();
  }

  inline bool Match(ScannerTokenType type, int index = 0) {
    return scanner->GetToken(index)->GetType() == type;
  }

  inline bool IsBasicType(ScannerTokenType type) {
    switch(GetToken()) {
    case TOKEN_BOOLEAN_ID:
    case TOKEN_BYTE_ID:
    case TOKEN_INT_ID:
    case TOKEN_FLOAT_ID:
    case TOKEN_CHAR_ID:
      return true;

    default:
      break;
    }

    return false;
  }

  inline ScannerTokenType GetToken(int index = 0) {
    return scanner->GetToken(index)->GetType();
  }

  inline int GetLineNumber() {
    return scanner->GetToken()->GetLineNumber();
  }

  inline const wstring GetFileName() {
    return scanner->GetToken()->GetFileName();
  }

  inline const wstring GetScopeName(const wstring &ident) {
    wstring scope_name;
    if(current_method) {
      scope_name = current_method->GetName() + L":" + ident;
    } else {
      scope_name = current_class->GetName() + L":" + ident;
    }

    return scope_name;
  }

  void Show(const wstring &msg, int depth) {
    wcout << setw(4) << GetLineNumber() << L": ";
    for(int i = 0; i < depth; i++) {
      wcout << L"  ";
    }
    wcout << msg << endl;
  }

  inline wstring ToString(int v) {
    wostringstream str;
    str << v;
    return str.str();
  }

  inline wstring ParseBundleName() {
    wstring name;
    if(Match(TOKEN_IDENT)) {
      while(Match(TOKEN_IDENT) && !Match(TOKEN_END_OF_STREAM)) {
        name += scanner->GetToken()->GetIdentifier();
        NextToken();
        if(Match(TOKEN_PERIOD)) {
          name += L'.';
          NextToken();
        }
	else if(Match(TOKEN_IDENT)) {
	  ProcessError(L"Expected period", TOKEN_SEMI_COLON);
	  NextToken();
	}
      }
    } 
    else {
      ProcessError(TOKEN_IDENT);
    }
    
    return name;
  }

  // error processing
  void LoadErrorCodes();
  void ProcessError(const ScannerTokenType type);
  void ProcessError(const wstring &msg);
  void ProcessError(const wstring &msg, ParseNode* node);
  void ProcessError(const wstring &msg, const ScannerTokenType sync);
  bool CheckErrors();

  // parsing operations
  void ParseFile(const wstring& file_name);
  void ParseProgram();
  void ParseBundle(int depth);
  wstring ParseBundleName(int depth);
  Class* ParseClass(const wstring &bundle_id, int depth);
  Class* ParseInterface(const wstring &bundle_id, int depth);
  Method* ParseMethod(bool is_function, bool virtual_required, int depth);
  Variable* ParseVariable(const wstring &ident, int depth);
  MethodCall* ParseMethodCall(int depth);
  MethodCall* ParseMethodCall(const wstring &ident, int depth);
  void ParseMethodCall(Expression* expression, int depth);
  MethodCall* ParseMethodCall(Variable* variable, int depth);
  void ParseAnonymousClass(MethodCall* method_call, int depth);
  StatementList* ParseStatementList(int depth);
  Statement* ParseStatement(int depth);
  Assignment* ParseAssignment(Variable* variable, int depth);
  StaticArray* ParseStaticArray(int depth);
  If* ParseIf(int depth);
  DoWhile* ParseDoWhile(int depth);
  While* ParseWhile(int depth);
  Select* ParseSelect(int depth);
  Enum* ParseEnum(int depth);
  For* ParseFor(int depth);
  For* ParseEach(int depth);
  CriticalSection* ParseCritical(int depth);
  Return* ParseReturn(int depth);
  Declaration* ParseDeclaration(const wstring &ident, int depth);
  DeclarationList* ParseDecelerationList(int depth);
  ExpressionList* ParseExpressionList(int depth, ScannerTokenType open = TOKEN_OPEN_PAREN,
                                      ScannerTokenType close = TOKEN_CLOSED_PAREN);
  ExpressionList* ParseIndices(int depth);
  void ParseCastTypeOf(Expression* expression, int depth);
  Type* ParseType(int depth);
  Expression* ParseExpression(int depth);
  Expression* ParseLogic(int depth);
  Expression* ParseMathLogic(int depth);
  Expression* ParseTerm(int depth);
  Expression* ParseFactor(int depth);
  Expression* ParseSimpleExpression(int depth);

 public:
  Parser(const wstring &p, const wstring &r) {
    src_path = p;
    run_prgm = wstring(r.begin(), r.end());
    program = new ParsedProgram;
    LoadErrorCodes();
    current_class = NULL;
    current_method = prev_method = NULL;
    anonymous_class_id = 0;
  }

  ~Parser() {
  }

  bool Parse();

  ParsedProgram* GetProgram() {
    return program;
  }

  SymbolTableManager* GetSymbolTable() {
    return symbol_table;
  }
};

#endif
