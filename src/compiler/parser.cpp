/***************************************************************************
 * Language parser.
 *
 * Copyright (c) 2008-2013, Randy Hollines
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met
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

#include "parser.h"

/****************************
 * Loads parsing error codes.
 ****************************/
void Parser::LoadErrorCodes()
{
  error_msgs[TOKEN_NATIVE_ID] = L"Expected 'native'";
  error_msgs[TOKEN_AS_ID] = L"Expected 'As'";
  error_msgs[TOKEN_IDENT] = L"Expected identifier";
  error_msgs[TOKEN_OPEN_PAREN] = L"Expected '('";
  error_msgs[TOKEN_CLOSED_PAREN] = L"Expected ')'";
  error_msgs[TOKEN_OPEN_BRACKET] = L"Expected '['";
  error_msgs[TOKEN_CLOSED_BRACKET] = L"Expected ']'";
  error_msgs[TOKEN_OPEN_BRACE] = L"Expected '{'";
  error_msgs[TOKEN_CLOSED_BRACE] = L"Expected '}'";
  error_msgs[TOKEN_COLON] = L"Expected ':'";
  error_msgs[TOKEN_COMMA] = L"Expected ','";
  error_msgs[TOKEN_ASSIGN] = L"Expected ':='";
  error_msgs[TOKEN_SEMI_COLON] = L"Expected ';'";
  error_msgs[TOKEN_ASSESSOR] = L"Expected '->'";
  error_msgs[TOKEN_TILDE] = L"Expected '~'";
}

/****************************
 * Emits parsing error.
 ****************************/
void Parser::ProcessError(ScannerTokenType type)
{
  wstring msg = error_msgs[type];
#ifdef _DEBUG
  wcout << L"\tError: " << GetFileName() << L":" << GetLineNumber() << L": "
        << msg << endl;
#endif

  const wstring &str_line_num = ToString(GetLineNumber());
  errors.insert(pair<int, wstring>(GetLineNumber(), GetFileName() + L":" + str_line_num + L": " + msg));
}

/****************************
 * Emits parsing error.
 ****************************/
void Parser::ProcessError(const wstring &msg)
{
#ifdef _DEBUG
  wcout << L"\tError: " << GetFileName() << L":" << GetLineNumber() << L": "
        << msg << endl;
#endif

  const wstring &str_line_num = ToString(GetLineNumber());
  errors.insert(pair<int, wstring>(GetLineNumber(), GetFileName() + L":" + 
                                   str_line_num + L": " + msg));
}

/****************************
 * Emits parsing error.
 ****************************/
void Parser::ProcessError(const wstring &msg, ScannerTokenType sync)
{
#ifdef _DEBUG
  wcout << L"\tError: " << GetFileName() << L":" << GetLineNumber() << L": "
        << msg << endl;
#endif

  const wstring &str_line_num = ToString(GetLineNumber());
  errors.insert(pair<int, wstring>(GetLineNumber(),
                                   GetFileName() + L":" + str_line_num +
                                   L": " + msg));
  ScannerTokenType token = GetToken();
  while(token != sync && token != TOKEN_END_OF_STREAM) {
    NextToken();
    token = GetToken();
  }
}

/****************************
 * Emits parsing error.
 ****************************/
void Parser::ProcessError(const wstring &msg, ParseNode* node)
{
#ifdef _DEBUG
  wcout << L"\tError: " << node->GetFileName() << L":" << node->GetLineNumber()
        << L": " << msg << endl;
#endif

  const wstring &str_line_num = ToString(node->GetLineNumber());
  errors.insert(pair<int, wstring>(node->GetLineNumber(), node->GetFileName() +
                                   L":" + str_line_num + L": " + msg));
}

/****************************
 * Checks for parsing errors.
 ****************************/
bool Parser::CheckErrors()
{
  // check and process errors
  if(errors.size()) {
    map<int, wstring>::iterator error;
    for(error = errors.begin(); error != errors.end(); ++error) {
      wcerr << error->second << endl;
    }

    // clean up
    delete program;
    program = NULL;

    return false;
  }

  return true;
}

/****************************
 * Starts the parsing process.
 ****************************/
bool Parser::Parse()
{
#ifdef _DEBUG
  wcout << "\n---------- Scanning/Parsing ---------" << endl;
#endif

  // parses source path
  if(src_path.size() > 0) {
    size_t offset = 0;
    size_t index = src_path.find(',');
    while(index != wstring::npos) {
      const wstring &file_name = src_path.substr(offset, index - offset);
      ParseFile(file_name);
      // update
      offset = index + 1;
      index = src_path.find(',', offset);
    }
    const wstring &file_name = src_path.substr(offset, src_path.size());
    ParseFile(file_name);
  }
  else if(run_prgm.size() > 0) {
    ParseProgram();
  }

  return CheckErrors();
}

/****************************
 * Parses a file.
 ****************************/
void Parser::ParseFile(const wstring &file_name)
{
  scanner = new Scanner(file_name);
  NextToken();
  ParseBundle(0);
  // clean up
  delete scanner;
  scanner = NULL;
}

/****************************
 * Parses a file.
 ****************************/
void Parser::ParseProgram()
{
  scanner = new Scanner(run_prgm, true);
  NextToken();
  ParseBundle(0);
  // clean up
  delete scanner;
  scanner = NULL;
}

/****************************
 * Parses a bundle.
 ****************************/
void Parser::ParseBundle(int depth)
{
  // uses
  vector<wstring> uses;
  uses.push_back(L"System");
  uses.push_back(L"System.Introspection"); // always include the default system bundles
  while(Match(TOKEN_USE_ID) && !Match(TOKEN_END_OF_STREAM)) {
    NextToken();
    const wstring &ident = ParseBundleName();
    uses.push_back(ident);
    if(!TOKEN_SEMI_COLON) {
      ProcessError(L"Expected ';'", TOKEN_SEMI_COLON);
    }
    NextToken();
#ifdef _DEBUG
    Show(L"search: " + ident, depth);
#endif
  }

  // parse bundle
  if(Match(TOKEN_BUNDLE_ID)) {
    while(Match(TOKEN_BUNDLE_ID) && !Match(TOKEN_END_OF_STREAM)) {
      NextToken();

      wstring bundle_name = ParseBundleName();
      if(bundle_name == DEFAULT_BUNDLE_NAME) {
        bundle_name = L"";
      }
      else {
        uses.push_back(bundle_name);
      }
      symbol_table = new SymbolTableManager;
      ParsedBundle* bundle = new ParsedBundle(bundle_name, symbol_table);
      if(!TOKEN_OPEN_BRACE) {
        ProcessError(L"Expected '{'", TOKEN_OPEN_BRACE);
      }
      NextToken();

      current_bundle = bundle;
#ifdef _DEBUG
      Show(L"bundle: '" + current_bundle->GetName() + L"'", depth);
#endif

      // parse classes, interfaces and enums
      while(!Match(TOKEN_CLOSED_BRACE) && !Match(TOKEN_END_OF_STREAM)) {
        if(Match(TOKEN_ENUM_ID)) {
          bundle->AddEnum(ParseEnum(depth + 1));
        } 
        else if(Match(TOKEN_CLASS_ID)) {
          bundle->AddClass(ParseClass(bundle_name, depth + 1));
        }
        else if(Match(TOKEN_INTERFACE_ID)) {
          bundle->AddClass(ParseInterface(bundle_name, depth + 1));
        }
        else {
          ProcessError(L"Expected 'class', 'interface' or 'enum'", TOKEN_SEMI_COLON);
          NextToken();
        }
      }

      if(!Match(TOKEN_CLOSED_BRACE)) {
        ProcessError(L"Expected '}'", TOKEN_CLOSED_BRACE);
      }
      NextToken();

      program->AddBundle(bundle);
    }

    // detect stray characters
    if(!Match(TOKEN_END_OF_STREAM)) {
      ProcessError(L"Unexpected tokens (likely related to other errors)");
    }
    program->AddUses(uses);
  }
  // parse class
  else if(Match(TOKEN_CLASS_ID) || Match(TOKEN_ENUM_ID) || Match(TOKEN_INTERFACE_ID)) {
    wstring bundle_name = L"";
    symbol_table = new SymbolTableManager;
    ParsedBundle* bundle = new ParsedBundle(bundle_name, symbol_table);

    current_bundle = bundle;    
#ifdef _DEBUG
    Show(L"bundle: '" + current_bundle->GetName() + L"'", depth);
#endif

    // parse classes, interfaces and enums
    while(!Match(TOKEN_CLOSED_BRACE) && !Match(TOKEN_END_OF_STREAM)) {
      if(Match(TOKEN_ENUM_ID)) {
        bundle->AddEnum(ParseEnum(depth + 1));
      } 
      else if(Match(TOKEN_CLASS_ID)) {
        bundle->AddClass(ParseClass(bundle_name, depth + 1));
      }
      else if(Match(TOKEN_INTERFACE_ID)) {
        bundle->AddClass(ParseInterface(bundle_name, depth + 1));
      }
      else {
        ProcessError(L"Expected 'class', 'interface' or 'enum'", TOKEN_SEMI_COLON);
        NextToken();
      }
    }
    program->AddBundle(bundle);

    // detect stray characters
    if(!Match(TOKEN_END_OF_STREAM)) {
      ProcessError(L"Unexpected tokens (likely related to other errors)");
    }
    program->AddUses(uses);
  }
  // error
  else {
    ProcessError(L"Expected 'use', 'bundle', 'class, 'enum', or 'interface'");
  }
}

/****************************
 * Parses an enum
 ****************************/
Enum* Parser::ParseEnum(int depth)
{
  const int line_num = GetLineNumber();
  const wstring &file_name = GetFileName();

#ifdef _DEBUG
  Show(L"Enum", depth);
#endif

  NextToken();
  if(!Match(TOKEN_IDENT)) {
    ProcessError(TOKEN_IDENT);
  }
  // identifier
  wstring enum_name = scanner->GetToken()->GetIdentifier();
  if(current_bundle->GetClass(enum_name) || current_bundle->GetEnum(enum_name)) {
    ProcessError(L"Class, interface or enum already defined in this bundle");
  }
  NextToken();

  int offset = 0;
  if(Match(TOKEN_ASSIGN)) {
    NextToken();
    Expression* label = ParseSimpleExpression(depth + 1);
    if(label) {
      if(label->GetExpressionType() != INT_LIT_EXPR) {
        ProcessError(L"Expected integer", TOKEN_CLOSED_PAREN);
      }
      offset = static_cast<IntegerLiteral*>(label)->GetValue();
    }
  }

  if(!Match(TOKEN_OPEN_BRACE)) {
    ProcessError(L"Expected '{'", TOKEN_OPEN_BRACE);
  }
  NextToken();

  Enum* eenum = TreeFactory::Instance()->MakeEnum(file_name, line_num, enum_name, offset);
  while(!Match(TOKEN_CLOSED_BRACE) && !Match(TOKEN_END_OF_STREAM)) {
    if(!Match(TOKEN_IDENT)) {
      ProcessError(TOKEN_IDENT);
    }
    // identifier
    wstring label_name = scanner->GetToken()->GetIdentifier();
    NextToken();
    eenum->AddItem(TreeFactory::Instance()->MakeEnumItem(file_name, line_num, label_name, eenum));

    if(Match(TOKEN_COMMA)) {
      NextToken();
      if(!Match(TOKEN_IDENT)) {
        ProcessError(TOKEN_IDENT);
      }
    } 
    else if(!Match(TOKEN_CLOSED_BRACE)) {
      ProcessError(L"Expected ',' or ')'", TOKEN_CLOSED_BRACE);
      NextToken();
    }
  }
  if(!Match(TOKEN_CLOSED_BRACE)) {
    ProcessError(L"Expected '}'", TOKEN_CLOSED_BRACE);
  }
  NextToken();

  return eenum;
}

/****************************
 * Parses a class.
 ****************************/
Class* Parser::ParseClass(const wstring &bundle_name, int depth)
{
  const int line_num = GetLineNumber();
  const wstring &file_name = GetFileName();

  NextToken();
  if(!Match(TOKEN_IDENT)) {
    ProcessError(TOKEN_IDENT);
  }
  // identifier
  wstring cls_name = scanner->GetToken()->GetIdentifier();
  if(current_bundle->GetClass(cls_name) || current_bundle->GetEnum(cls_name)) {
    ProcessError(L"Class, interface or enum already defined in this bundle");
  }
  NextToken();

#ifdef _DEBUG
  Show(L"[Class: name='" + cls_name + L"']", depth);
#endif

  // from id
  wstring parent_cls_name;
  if(Match(TOKEN_FROM_ID)) {
    NextToken();
    if(!Match(TOKEN_IDENT)) {
      ProcessError(TOKEN_IDENT);
    }
    // identifier
    parent_cls_name = ParseBundleName();
  }

  // implements ids
  vector<wstring> interface_names;
  if(Match(TOKEN_IMPLEMENTS_ID)) {
    NextToken();
    while(!Match(TOKEN_OPEN_BRACE) && !Match(TOKEN_END_OF_STREAM)) {
      if(!Match(TOKEN_IDENT)) {
        ProcessError(TOKEN_IDENT);
      }
      // identifier
      const wstring& ident = ParseBundleName();
      interface_names.push_back(ident);      
      if(Match(TOKEN_COMMA)) {
        NextToken();
        if(!Match(TOKEN_IDENT)) {
          ProcessError(TOKEN_IDENT);
        }
      } 
      else if(!Match(TOKEN_OPEN_BRACE)) {
        ProcessError(L"Expected ',' or '{'", TOKEN_OPEN_BRACE);
        NextToken();
      }
    }
  }
  if(!Match(TOKEN_OPEN_BRACE)) {
    ProcessError(L"Expected '{'", TOKEN_OPEN_BRACE);
  }

  symbol_table->NewParseScope();
  NextToken();

  if(bundle_name.size() > 0) {
    cls_name.insert(0, L".");
    cls_name.insert(0, bundle_name);
  }

  if(current_bundle->GetClass(cls_name)) {
    ProcessError(L"Class has already been defined");
  }

  Class* klass = TreeFactory::Instance()->MakeClass(file_name, line_num, cls_name, parent_cls_name, 
                                                    interface_names, false);
  current_class = klass;

  // add '@self' entry
  SymbolEntry* entry = TreeFactory::Instance()->MakeSymbolEntry(file_name, line_num, GetScopeName(SELF_ID),
                                                                TypeFactory::Instance()->MakeType(CLASS_TYPE, cls_name), 
                                                                false, false, true);
  symbol_table->CurrentParseScope()->AddEntry(entry);

  // add '@parent' entry
  entry = TreeFactory::Instance()->MakeSymbolEntry(file_name, line_num, GetScopeName(PARENT_ID),
                                                   TypeFactory::Instance()->MakeType(CLASS_TYPE, cls_name), 
                                                   false, false, true);
  symbol_table->CurrentParseScope()->AddEntry(entry);

  while(!Match(TOKEN_CLOSED_BRACE) && !Match(TOKEN_END_OF_STREAM)) {
    // parse 'method | function | declaration'
    if(Match(TOKEN_FUNCTION_ID)) {
      Method* method = ParseMethod(true, false, depth + 1);
      bool was_added = klass->AddMethod(method);
      if(!was_added) {
        ProcessError(L"Method or function already defined or overloaded '" + method->GetName() + L"'", method);
      }
    } 
    else if(Match(TOKEN_METHOD_ID) || Match(TOKEN_NEW_ID)) {
      Method* method = ParseMethod(false, false, depth + 1);
      bool was_added = klass->AddMethod(method);
      if(!was_added) {
        ProcessError(L"Method or function already defined or overloaded '" + method->GetName() + L"'", method);
      }
    } 
    else if(Match(TOKEN_IDENT)) {
      const wstring &ident = scanner->GetToken()->GetIdentifier();
      NextToken();

      klass->AddStatement(ParseDeclaration(ident, depth + 1));
      if(!Match(TOKEN_SEMI_COLON)) {
        ProcessError(L"Expected ';'", TOKEN_SEMI_COLON);
      }
      NextToken();
    } 
    else {
      ProcessError(L"Expected declaration", TOKEN_SEMI_COLON);
      NextToken();
    }
  }

  if(!Match(TOKEN_CLOSED_BRACE)) {
    ProcessError(L"Expected '}'", TOKEN_CLOSED_BRACE);
  }
  NextToken();

  symbol_table->PreviousParseScope(current_class->GetName());
  current_class = NULL;
  return klass;
}

/****************************
 * Parses an interface
 ****************************/
Class* Parser::ParseInterface(const wstring &bundle_name, int depth)
{
  const int line_num = GetLineNumber();
  const wstring &file_name = GetFileName();

  NextToken();
  if(!Match(TOKEN_IDENT)) {
    ProcessError(TOKEN_IDENT);
  }
  // identifier
  wstring cls_name = scanner->GetToken()->GetIdentifier();
  if(current_bundle->GetClass(cls_name) || current_bundle->GetEnum(cls_name)) {
    ProcessError(L"Class, interface or enum already defined in this bundle");
  }
  NextToken();

#ifdef _DEBUG
  Show(L"[Interface: name='" + cls_name + L"']", depth);
#endif

  if(!Match(TOKEN_OPEN_BRACE)) {
    ProcessError(L"Expected '{'", TOKEN_OPEN_BRACE);
  }
  symbol_table->NewParseScope();
  NextToken();

  if(bundle_name.size() > 0) {
    cls_name.insert(0, L".");
    cls_name.insert(0, bundle_name);
  }

  if(current_bundle->GetClass(cls_name)) {
    ProcessError(L"Class has already been defined");
  }

  vector<wstring> interface_strings;
  Class* klass = TreeFactory::Instance()->MakeClass(file_name, line_num, cls_name, L"", 
                                                    interface_strings, true);
  current_class = klass;

  while(!Match(TOKEN_CLOSED_BRACE) && !Match(TOKEN_END_OF_STREAM)) {
    // parse 'method | function | declaration'
    if(Match(TOKEN_FUNCTION_ID)) {
      Method* method = ParseMethod(true, true, depth + 1);
      bool was_added = klass->AddMethod(method);
      if(!was_added) {
        ProcessError(L"Method or function already defined or overloaded '" + method->GetName() + L"'", method);
      }
    } 
    else if(Match(TOKEN_METHOD_ID)) {
      Method* method = ParseMethod(false, true, depth + 1);
      bool was_added = klass->AddMethod(method);
      if(!was_added) {
        ProcessError(L"Method or function already defined or overloaded '" + method->GetName() + L"'", method);
      }
    } 
    else {
      ProcessError(L"Expected declaration", TOKEN_SEMI_COLON);
      NextToken();
    }
  }

  if(!Match(TOKEN_CLOSED_BRACE)) {
    ProcessError(L"Expected '}'", TOKEN_CLOSED_BRACE);
  }
  NextToken();

  symbol_table->PreviousParseScope(current_class->GetName());
  current_class = NULL;
  return klass;
}

/****************************
 * Parses a method.
 ****************************/
Method* Parser::ParseMethod(bool is_function, bool virtual_requried, int depth)
{
  const int line_num = GetLineNumber();
  const wstring &file_name = GetFileName();

  MethodType method_type;
  wstring method_name;
  bool is_native = false;
  bool is_virtual = false;
  if(Match(TOKEN_NEW_ID)) {
    NextToken();

    if(Match(TOKEN_COLON)) {
      NextToken();
      if(!Match(TOKEN_PRIVATE_ID)) {
        ProcessError(L"Expected 'private'", TOKEN_SEMI_COLON);
      }
      NextToken();
      method_type = NEW_PRIVATE_METHOD;
    } else {
      method_type = NEW_PUBLIC_METHOD;
    }

    method_name = current_class->GetName() + L":New";
  } 
  else {
    NextToken();

    if(!Match(TOKEN_COLON)) {
      ProcessError(L"Expected ';'", TOKEN_COLON);
    }
    NextToken();

    // virtual method
    if(Match(TOKEN_VIRTUAL_ID)) {
      is_virtual = true;
      NextToken();

      if(!Match(TOKEN_COLON)) {
        ProcessError(L"Expected ';'", TOKEN_COLON);
      }
      NextToken();
      current_class->SetVirtual(true);
    }

    // public/private methods
    if(Match(TOKEN_PUBLIC_ID)) {
      method_type = PUBLIC_METHOD;
      NextToken();

      if(!Match(TOKEN_COLON)) {
        ProcessError(L"Expected ';'", TOKEN_COLON);
      }
      NextToken();
    } 
    else if(Match(TOKEN_PRIVATE_ID)) {
      method_type = PRIVATE_METHOD;
      NextToken();

      if(!Match(TOKEN_COLON)) {
        ProcessError(L"Expected ';'", TOKEN_COLON);
      }
      NextToken();
    } 
    else {
      method_type = PRIVATE_METHOD;
    }

    if(Match(TOKEN_NATIVE_ID)) {
      NextToken();
      is_native = true;
      if(!Match(TOKEN_COLON)) {
        ProcessError(L"Expected ';'", TOKEN_COLON);
      }
      NextToken();
    }

    // virtual method
    if(!is_virtual && Match(TOKEN_VIRTUAL_ID)) {
      is_virtual = true;
      NextToken();

      if(!Match(TOKEN_COLON)) {
        ProcessError(L"Expected ';'", TOKEN_COLON);
      }
      NextToken();
      current_class->SetVirtual(true);
    }
    else if(is_virtual && Match(TOKEN_VIRTUAL_ID)) {
      ProcessError(L"The 'virtual' attribute has already been specified", TOKEN_IDENT);
    }

    if(!Match(TOKEN_IDENT)) {
      ProcessError(TOKEN_IDENT);
    }
    // identifer
    wstring ident = scanner->GetToken()->GetIdentifier();
    method_name = current_class->GetName() + L":" + ident;
    NextToken();
  }

#ifdef _DEBUG
  Show(L"(Method/Function/New: name='" + method_name + L"')", depth);
#endif

  Method* method = TreeFactory::Instance()->MakeMethod(file_name, line_num, method_name,
                                                       method_type, is_function, is_native);
  current_method = method;

  // declarations
  symbol_table->NewParseScope();
  method->SetDeclarations(ParseDecelerationList(depth + 1));

  // return type
  Type* return_type;
  if(method_type != NEW_PUBLIC_METHOD && method_type != NEW_PRIVATE_METHOD) {
    if(!Match(TOKEN_TILDE)) {
      ProcessError(L"Expected '~'", TOKEN_TILDE);
    }
    NextToken();
    return_type = ParseType(depth + 1);
  } 
  else {
    return_type = TypeFactory::Instance()->MakeType(CLASS_TYPE, current_class->GetName());
  }
  method->SetReturn(return_type);

  // statements
  if(is_virtual) {
    method->SetStatements(NULL);
    // virtual function/method ending
    if(!Match(TOKEN_SEMI_COLON)) {
      ProcessError(L"Expected ';'", TOKEN_SEMI_COLON);
    }
    NextToken();
  } 
  else if(virtual_requried && !is_virtual) {
    ProcessError(L"Method or function must be declared as virtual", TOKEN_SEMI_COLON);
  }
  else {
    method->SetStatements(ParseStatementList(depth + 1));
  }

  symbol_table->PreviousParseScope(method->GetParsedName());
  current_method = NULL;

  return method;
}

/****************************
 * Parses a statement list.
 ****************************/
StatementList* Parser::ParseStatementList(int depth)
{
#ifdef _DEBUG
  Show(L"Statement List", depth);
#endif

  // statement list
  if(!Match(TOKEN_OPEN_BRACE)) {
    ProcessError(L"Expected '{'", TOKEN_OPEN_BRACE);
  }
  NextToken();

  StatementList* statements = TreeFactory::Instance()->MakeStatementList();
  while(!Match(TOKEN_CLOSED_BRACE) && !Match(TOKEN_END_OF_STREAM)) {
    statements->AddStatement(ParseStatement(depth + 1));
  }
  if(!Match(TOKEN_CLOSED_BRACE)) {
    ProcessError(L"Expected '}'", TOKEN_CLOSED_BRACE);
  }
  NextToken();

  return statements;
}

/****************************
 * Parses a statement.
 ****************************/
Statement* Parser::ParseStatement(int depth)
{
  const int line_num = GetLineNumber();
  const wstring &file_name = GetFileName();

#ifdef _DEBUG
  Show(L"Statement", depth);
#endif

  Statement* statement = NULL;

  // identifier
  if(Match(TOKEN_IDENT)) {
    const wstring ident = ParseBundleName();

    switch(GetToken()) {
    case TOKEN_COLON:
      statement = ParseDeclaration(ident, depth + 1);
      break;

    case TOKEN_ASSESSOR:
    case TOKEN_OPEN_PAREN:
      statement = ParseMethodCall(ident, depth + 1);
      break;

    case TOKEN_OPEN_BRACKET: {
      Variable* variable = ParseVariable(ident, depth + 1);

      switch(GetToken()) {
      case TOKEN_ASSIGN:
        statement = ParseAssignment(variable, depth + 1);
        break;

      case TOKEN_ADD_ASSIGN:
        NextToken();
        statement = TreeFactory::Instance()->MakeOperationAssignment(file_name, line_num, variable,
                                                                     ParseLogic(depth + 1), 
                                                                     ADD_ASSIGN_STMT);
        break;

      case TOKEN_SUB_ASSIGN:
        NextToken();
        statement = TreeFactory::Instance()->MakeOperationAssignment(file_name, line_num, variable,
                                                                     ParseLogic(depth + 1), 
                                                                     SUB_ASSIGN_STMT);
        break;

      case TOKEN_MUL_ASSIGN:
        NextToken();
        statement = TreeFactory::Instance()->MakeOperationAssignment(file_name, line_num, variable,
                                                                     ParseLogic(depth + 1), 
                                                                     MUL_ASSIGN_STMT);
        break;

      case TOKEN_DIV_ASSIGN:
        NextToken();
        statement = TreeFactory::Instance()->MakeOperationAssignment(file_name, line_num, variable,
                                                                     ParseLogic(depth + 1), 
                                                                     DIV_ASSIGN_STMT);
        break;

      case TOKEN_ASSESSOR:
        // subsequent method
        if(Match(TOKEN_ASSESSOR) && !Match(TOKEN_AS_ID, SECOND_INDEX) && 
           !Match(TOKEN_TYPE_OF_ID, SECOND_INDEX)) {
          statement = ParseMethodCall(variable, depth + 1);
        }
        // type cast
        else {
          ParseCastTypeOf(variable, depth + 1);
          statement = TreeFactory::Instance()->MakeSimpleStatement(file_name, line_num, variable);
        }
        break;

      default:
        ProcessError(L"Expected statement", TOKEN_SEMI_COLON);
        break;
      }
    }
      break;

    case TOKEN_ASSIGN:
      statement = ParseAssignment(ParseVariable(ident, depth + 1), depth + 1);
      break;

    case TOKEN_ADD_ASSIGN:
      NextToken();
      statement = TreeFactory::Instance()->MakeOperationAssignment(file_name, line_num, 
                                                                   ParseVariable(ident, depth + 1),
                                                                   ParseLogic(depth + 1), 
                                                                   ADD_ASSIGN_STMT);
      break;

    case TOKEN_SUB_ASSIGN:
      NextToken();
      statement = TreeFactory::Instance()->MakeOperationAssignment(file_name, line_num, 
                                                                   ParseVariable(ident, depth + 1),
                                                                   ParseLogic(depth + 1), 
                                                                   SUB_ASSIGN_STMT);
      break;

    case TOKEN_MUL_ASSIGN:
      NextToken();
      statement = TreeFactory::Instance()->MakeOperationAssignment(file_name, line_num, 
                                                                   ParseVariable(ident, depth + 1),
                                                                   ParseLogic(depth + 1), 
                                                                   MUL_ASSIGN_STMT);
      break;

    case TOKEN_DIV_ASSIGN:
      NextToken();
      statement = TreeFactory::Instance()->MakeOperationAssignment(file_name, line_num, 
                                                                   ParseVariable(ident, depth + 1),
                                                                   ParseLogic(depth + 1), 
                                                                   DIV_ASSIGN_STMT);
      break;

    default:
      ProcessError(L"Expected statement", TOKEN_SEMI_COLON);
      break;
    }
  }
  // other
  else {
    switch(GetToken()) { 
    case TOKEN_SEMI_COLON:
      statement = TreeFactory::Instance()->MakeEmptyStatement(file_name, line_num);
      break;
      
    case TOKEN_PARENT_ID:
      statement = ParseMethodCall(depth + 1);
      break;

    case TOKEN_RETURN_ID:
      statement = ParseReturn(depth + 1);
      break;

    case TOKEN_IF_ID:
      statement = ParseIf(depth + 1);
      break;

    case TOKEN_DO_ID:
      statement = ParseDoWhile(depth + 1);
      break;

    case TOKEN_WHILE_ID:
      statement = ParseWhile(depth + 1);
      break;

    case TOKEN_BREAK_ID:
      statement = TreeFactory::Instance()->MakeBreak(file_name, line_num);
      NextToken();
      break;

    case TOKEN_SELECT_ID:
      statement = ParseSelect(depth + 1);
      break;

    case TOKEN_FOR_ID:
      statement = ParseFor(depth + 1);
      break;

    case TOKEN_EACH_ID:
      statement = ParseEach(depth + 1);
      break;

    case TOKEN_CRITICAL_ID:
      statement = ParseCritical(depth + 1);
      break;
      
#ifdef _SYSTEM
    case ASYNC_MTHD_CALL:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::ASYNC_MTHD_CALL);
      NextToken();      
      break;

    case DLL_LOAD:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::DLL_LOAD);
      NextToken();
      break;

    case DLL_UNLOAD:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::DLL_UNLOAD);
      NextToken();
      break;

    case DLL_FUNC_CALL:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::DLL_FUNC_CALL);
      NextToken();
      break;

    case THREAD_MUTEX:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::THREAD_MUTEX);
      NextToken();
      break;

    case THREAD_SLEEP:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::THREAD_SLEEP);
      NextToken();
      break;

    case THREAD_JOIN:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::THREAD_JOIN);
      NextToken();
      break;

    case SYS_TIME:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SYS_TIME);
      NextToken();
      break;
      
    case BYTES_TO_UNICODE:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::BYTES_TO_UNICODE);
      NextToken();
      break;
      
    case UNICODE_TO_BYTES:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::UNICODE_TO_BYTES);
      NextToken();
      break;
      
    case GMT_TIME:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::GMT_TIME);
      NextToken();
      break;

    case FILE_CREATE_TIME:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::FILE_CREATE_TIME);
      NextToken();
      break;

    case FILE_MODIFIED_TIME:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::FILE_MODIFIED_TIME);
      NextToken();
      break;

    case FILE_ACCESSED_TIME:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::FILE_ACCESSED_TIME);
      NextToken();
      break;

    case DATE_TIME_SET_1:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::DATE_TIME_SET_1);
      NextToken();
      break;

    case DATE_TIME_SET_2:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::DATE_TIME_SET_2);
      NextToken();
      break;

    case DATE_TIME_SET_3:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::DATE_TIME_SET_3);
      NextToken();
      break;

    case DATE_TIME_ADD_DAYS:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::DATE_TIME_ADD_DAYS);
      NextToken();
      break;

    case DATE_TIME_ADD_HOURS:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::DATE_TIME_ADD_HOURS);
      NextToken();
      break;

    case DATE_TIME_ADD_MINS:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::DATE_TIME_ADD_MINS);
      NextToken();
      break;

    case DATE_TIME_ADD_SECS:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::DATE_TIME_ADD_SECS);
      NextToken();
      break;

    case PLTFRM:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::PLTFRM);
      NextToken();
      break;

    case GET_SYS_PROP:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::GET_SYS_PROP);
      NextToken();
      break;

    case SET_SYS_PROP:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SET_SYS_PROP);
      NextToken();
      break;

    case EXIT:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::EXIT);
      NextToken();
      break;

    case TIMER_START:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::TIMER_START);
      NextToken();
      break;

    case TIMER_END:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::TIMER_END);
      NextToken();
      break;

    case FLOR_FLOAT:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::FLOR_FLOAT);
      NextToken();
      break;

    case CPY_BYTE_ARY:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::CPY_BYTE_ARY);
      NextToken();
      break;
      
    case LOAD_ARY_SIZE:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::LOAD_ARY_SIZE);
      NextToken();
      break;
      
    case CPY_CHAR_ARY:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::CPY_CHAR_ARY);
      NextToken();
      break;
      
    case CPY_INT_ARY:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::CPY_INT_ARY);
      NextToken();
      break;

    case CPY_FLOAT_ARY:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::CPY_FLOAT_ARY);
      NextToken();
      break;

    case CEIL_FLOAT:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::CEIL_FLOAT);
      NextToken();
      break;

    case SIN_FLOAT:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SIN_FLOAT);
      NextToken();
      break;

    case COS_FLOAT:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::COS_FLOAT);
      NextToken();
      break;

    case TAN_FLOAT:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::TAN_FLOAT);
      NextToken();
      break;

    case ASIN_FLOAT:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::ASIN_FLOAT);
      NextToken();
      break;

    case ACOS_FLOAT:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::ACOS_FLOAT);
      NextToken();
      break;

    case ATAN_FLOAT:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::ATAN_FLOAT);
      NextToken();
      break;

    case LOG_FLOAT:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::LOG_FLOAT);
      NextToken();
      break;

    case POW_FLOAT:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::POW_FLOAT);
      NextToken();
      break;

    case SQRT_FLOAT:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SQRT_FLOAT);
      NextToken();
      break;

    case RAND_FLOAT:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::RAND_FLOAT);
      NextToken();
      break;

    case STD_OUT_BOOL:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::STD_OUT_BOOL);
      NextToken();
      break;

    case STD_OUT_BYTE:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::STD_OUT_BYTE);
      NextToken();
      break;

    case STD_OUT_CHAR:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::STD_OUT_CHAR);
      NextToken();
      break;

    case STD_OUT_INT:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::STD_OUT_INT);
      NextToken();
      break;

    case STD_OUT_FLOAT:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::STD_OUT_FLOAT);
      NextToken();
      break;

    case STD_OUT_CHAR_ARY:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::STD_OUT_CHAR_ARY);
      NextToken();
      break;

    case STD_OUT_BYTE_ARY_LEN:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::STD_OUT_BYTE_ARY_LEN);
      NextToken();
      break;
      
    case STD_OUT_CHAR_ARY_LEN:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::STD_OUT_CHAR_ARY_LEN);
      NextToken();
      break;
      
    case STD_ERR_BOOL:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::STD_ERR_BOOL);
      NextToken();
      break;

    case STD_ERR_BYTE:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::STD_ERR_BYTE);
      NextToken();
      break;

    case STD_ERR_CHAR:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::STD_ERR_CHAR);
      NextToken();
      break;

    case STD_ERR_INT:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::STD_ERR_INT);
      NextToken();
      break;

    case STD_ERR_FLOAT:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::STD_ERR_FLOAT);
      NextToken();
      break;

    case STD_ERR_CHAR_ARY:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::STD_ERR_CHAR_ARY);
      NextToken();
      break;

    case STD_ERR_BYTE_ARY:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::STD_ERR_BYTE_ARY);
      NextToken();
      break;
      
    case LOAD_MULTI_ARY_SIZE:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::LOAD_MULTI_ARY_SIZE);
      NextToken();
      break;

    case LOAD_CLS_INST_ID:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::LOAD_CLS_INST_ID);
      NextToken();
      break;

    case LOAD_CLS_BY_INST:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::LOAD_CLS_BY_INST);
      NextToken();
      break;

    case LOAD_NEW_OBJ_INST:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::LOAD_NEW_OBJ_INST);
      NextToken();
      break;

    case LOAD_INST_UID:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::LOAD_INST_UID);
      NextToken();
      break;

    case STD_IN_STRING:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::STD_IN_STRING);
      NextToken();
      break;

    case FILE_OPEN_READ:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::FILE_OPEN_READ);
      NextToken();
      break;

    case FILE_CLOSE:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::FILE_CLOSE);
      NextToken();
      break;

    case FILE_FLUSH:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::FILE_FLUSH);
      NextToken();
      break;

    case FILE_IN_BYTE:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::FILE_IN_BYTE);
      NextToken();
      break;

    case FILE_IN_BYTE_ARY:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::FILE_IN_BYTE_ARY);
      NextToken();
      break;

    case FILE_IN_CHAR_ARY:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::FILE_IN_CHAR_ARY);
      NextToken();
      break;

    case FILE_IN_STRING:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::FILE_IN_STRING);
      NextToken();
      break;

    case FILE_OPEN_WRITE:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::FILE_OPEN_WRITE);
      NextToken();
      break;

    case FILE_OPEN_READ_WRITE:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::FILE_OPEN_READ_WRITE);
      NextToken();
      break;
      
    case FILE_OUT_BYTE:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::FILE_OUT_BYTE);
      NextToken();
      break;

    case FILE_OUT_BYTE_ARY:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::FILE_OUT_BYTE_ARY);
      NextToken();
      break;

    case FILE_OUT_STRING:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::FILE_OUT_STRING);
      NextToken();
      break;

    case FILE_EXISTS:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::FILE_EXISTS);
      NextToken();
      break;

    case FILE_SIZE:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::FILE_SIZE);
      NextToken();
      break;

    case FILE_SEEK:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::FILE_SEEK);
      NextToken();
      break;

    case FILE_EOF:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::FILE_EOF);
      NextToken();
      break;

    case FILE_DELETE:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::FILE_DELETE);
      NextToken();
      break;

    case FILE_RENAME:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::FILE_RENAME);
      NextToken();
      break;

    case DIR_CREATE:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::DIR_CREATE);
      NextToken();
      break;

    case DIR_EXISTS:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::DIR_EXISTS);
      NextToken();
      break;

    case DIR_LIST:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::DIR_LIST);
      NextToken();
      break;

    case FILE_REWIND:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::FILE_REWIND);
      NextToken();
      break;

    case FILE_IS_OPEN:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::FILE_IS_OPEN);
      NextToken();
      break;

    case SOCK_TCP_CONNECT:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SOCK_TCP_CONNECT);
      NextToken();
      break;

    case SOCK_TCP_BIND:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SOCK_TCP_BIND);
      NextToken();
      break;

    case SOCK_TCP_LISTEN:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SOCK_TCP_LISTEN);
      NextToken();
      break;

    case SOCK_TCP_ACCEPT:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SOCK_TCP_ACCEPT);
      NextToken();
      break;      

    case SOCK_TCP_IS_CONNECTED:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SOCK_TCP_IS_CONNECTED);
      NextToken();
      break;

    case SOCK_TCP_CLOSE:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SOCK_TCP_CLOSE);
      NextToken();
      break;

    case SOCK_TCP_IN_BYTE:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SOCK_TCP_IN_BYTE);
      NextToken();
      break;

    case SOCK_TCP_IN_BYTE_ARY:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SOCK_TCP_IN_BYTE_ARY);
      NextToken();
      break;

    case SOCK_TCP_OUT_STRING:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SOCK_TCP_OUT_STRING);
      NextToken();
      break;

    case SOCK_TCP_IN_STRING:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SOCK_TCP_IN_STRING);
      NextToken();
      break;

    case SOCK_TCP_OUT_BYTE:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SOCK_TCP_OUT_BYTE);
      NextToken();
      break;

    case SOCK_TCP_OUT_BYTE_ARY:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SOCK_TCP_OUT_BYTE_ARY);
      NextToken();
      break;   

    case SOCK_TCP_HOST_NAME:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SOCK_TCP_HOST_NAME);
      NextToken();
      break;

    case SOCK_TCP_SSL_CONNECT:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SOCK_TCP_SSL_CONNECT);
      NextToken();
      break;

    case SOCK_TCP_SSL_BIND:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SOCK_TCP_SSL_BIND);
      NextToken();
      break;

    case SOCK_TCP_SSL_LISTEN:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SOCK_TCP_SSL_LISTEN);
      NextToken();
      break;

    case SOCK_TCP_SSL_ACCEPT:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SOCK_TCP_SSL_ACCEPT);
      NextToken();
      break;      

    case SOCK_TCP_SSL_IS_CONNECTED:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SOCK_TCP_SSL_IS_CONNECTED);
      NextToken();
      break;

    case SOCK_TCP_SSL_CLOSE:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SOCK_TCP_SSL_CLOSE);
      NextToken();
      break;

    case SOCK_TCP_SSL_IN_BYTE:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SOCK_TCP_SSL_IN_BYTE);
      NextToken();
      break;

    case SOCK_TCP_SSL_IN_BYTE_ARY:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SOCK_TCP_SSL_IN_BYTE_ARY);
      NextToken();
      break;

    case SOCK_TCP_SSL_OUT_STRING:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SOCK_TCP_SSL_OUT_STRING);
      NextToken();
      break;

    case SOCK_TCP_SSL_IN_STRING:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SOCK_TCP_SSL_IN_STRING);
      NextToken();
      break;

    case SOCK_TCP_SSL_OUT_BYTE:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SOCK_TCP_SSL_OUT_BYTE);
      NextToken();
      break;

    case SOCK_TCP_SSL_OUT_BYTE_ARY:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SOCK_TCP_SSL_OUT_BYTE_ARY);
      NextToken();
      break;   

    case SERL_CHAR:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SERL_CHAR);
      NextToken();
      break;
      
    case SERL_INT:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SERL_INT);
      NextToken();
      break;

    case SERL_FLOAT:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SERL_FLOAT);
      NextToken();
      break;

    case SERL_OBJ_INST:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SERL_OBJ_INST);
      NextToken();
      break;

    case SERL_BYTE_ARY:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SERL_BYTE_ARY);
      NextToken();
      break;

    case SERL_CHAR_ARY:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SERL_CHAR_ARY);
      NextToken();
      break;

    case SERL_INT_ARY:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SERL_INT_ARY);
      NextToken();
      break;

    case SERL_FLOAT_ARY:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::SERL_FLOAT_ARY);
      NextToken();
      break;

    case DESERL_CHAR:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::DESERL_CHAR);
      NextToken();
      break;
      
    case DESERL_INT:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::DESERL_INT);
      NextToken();
      break;

    case DESERL_FLOAT:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::DESERL_FLOAT);
      NextToken();
      break;

    case DESERL_OBJ_INST:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::DESERL_OBJ_INST);
      NextToken();
      break;

    case DESERL_BYTE_ARY:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::DESERL_BYTE_ARY);
      NextToken();
      break;

    case DESERL_CHAR_ARY:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::DESERL_CHAR_ARY);
      NextToken();
      break;

    case DESERL_INT_ARY:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::DESERL_INT_ARY);
      NextToken();
      break;

    case DESERL_FLOAT_ARY:
      statement = TreeFactory::Instance()->MakeSystemStatement(file_name, line_num,
                                                               instructions::DESERL_FLOAT_ARY);
      NextToken();
      break;
#endif

    default:
      statement = TreeFactory::Instance()->MakeSimpleStatement(file_name, line_num,
                                                               ParseSimpleExpression(depth + 1));
      break;
    }
  }

  if(!Match(TOKEN_SEMI_COLON)) {
    ProcessError(L"Expected ';'", TOKEN_SEMI_COLON);
  }
  NextToken();

  return statement;
}

/****************************
 * Parses a static array
 ****************************/
StaticArray* Parser::ParseStaticArray(int depth) {
  const int line_num = GetLineNumber();
  const wstring &file_name = GetFileName();

#ifdef _DEBUG
  Show(L"Static Array", depth);
#endif

  NextToken();
  ExpressionList* expressions = TreeFactory::Instance()->MakeExpressionList();

  // array dimension
  if(Match(TOKEN_OPEN_BRACKET)) {
    while(!Match(TOKEN_CLOSED_BRACKET) && !Match(TOKEN_END_OF_STREAM)) {
      expressions->AddExpression(ParseStaticArray(depth + 1));
    }

    if(!Match(TOKEN_CLOSED_BRACKET)) {
      ProcessError(L"Expected ']'", TOKEN_CLOSED_BRACKET);
    }
    NextToken();
  }
  // array elements
  else {
    while(!Match(TOKEN_CLOSED_BRACKET) && !Match(TOKEN_END_OF_STREAM)) {
      Expression* expression = NULL;
      if(Match(TOKEN_SUB)) {
        NextToken();

        switch(GetToken()) {
        case TOKEN_INT_LIT:
          expression = TreeFactory::Instance()->MakeIntegerLiteral(file_name, line_num,
                                                                   -scanner->GetToken()->GetIntLit());
          NextToken();
          break;

        case TOKEN_FLOAT_LIT:
          expression = TreeFactory::Instance()->MakeFloatLiteral(file_name, line_num,
                                                                 -scanner->GetToken()->GetFloatLit());
          NextToken();
          break;

        default:
          ProcessError(L"Expected literal expression", TOKEN_SEMI_COLON);
          break;
        }
      }
      else {
        switch(GetToken()) {
        case TOKEN_INT_LIT:
          expression = TreeFactory::Instance()->MakeIntegerLiteral(file_name, line_num,
                                                                   scanner->GetToken()->GetIntLit());
          NextToken();
          break;

        case TOKEN_FLOAT_LIT:
          expression = TreeFactory::Instance()->MakeFloatLiteral(file_name, line_num,
                                                                 scanner->GetToken()->GetFloatLit());
          NextToken();
          break;

        case TOKEN_CHAR_LIT:
          expression = TreeFactory::Instance()->MakeCharacterLiteral(file_name, line_num,
                                                                     scanner->GetToken()->GetCharLit());
          NextToken();
          break;

        case TOKEN_CHAR_STRING_LIT: {
          const wstring &ident = scanner->GetToken()->GetIdentifier();
          expression = TreeFactory::Instance()->MakeCharacterString(file_name, line_num, ident);
          NextToken();
        }
          break;
          
        case TOKEN_BAD_CHAR_STRING_LIT:
          ProcessError(L"Invalid escaped string literal", TOKEN_SEMI_COLON);
          NextToken();
          break;

        default:
          ProcessError(L"Expected literal expression", TOKEN_SEMI_COLON);
          break;
        }
      }
      // add expression
      expressions->AddExpression(expression);

      // next expression
      if(Match(TOKEN_COMMA)) {
        NextToken();
      } 
      else if(!Match(TOKEN_CLOSED_BRACKET)) {
        ProcessError(L"Expected ';' or ']'", TOKEN_SEMI_COLON);
        NextToken();
      }
    }

    if(!Match(TOKEN_CLOSED_BRACKET)) {
      ProcessError(L"Expected ']'", TOKEN_CLOSED_BRACKET);
    }
    NextToken();
  }

  return TreeFactory::Instance()->MakeStaticArray(file_name, line_num, expressions);
}

/****************************
 * Parses a variable.
 ****************************/
Variable* Parser::ParseVariable(const wstring &ident, int depth)
{
  const int line_num = GetLineNumber();
  const wstring &file_name = GetFileName();

#ifdef _DEBUG
  Show(L"Variable", depth);
#endif

  Variable* variable = TreeFactory::Instance()->MakeVariable(file_name, line_num, ident);
  variable->SetIndices(ParseIndices(depth + 1));

  return variable;
}

/****************************
 * Parses a declaration.
 ****************************/
Declaration* Parser::ParseDeclaration(const wstring &ident, int depth)
{
  const int line_num = GetLineNumber();
  const wstring &file_name = GetFileName();

#ifdef _DEBUG
  Show(L"Declaration", depth);
#endif

  if(!Match(TOKEN_COLON)) {
    ProcessError(L"Expected ','", TOKEN_COLON);
  }
  NextToken();

  Declaration* declaration;
  if(Match(TOKEN_OPEN_PAREN)) {
    Type* type = ParseType(depth + 1);

    // add entry
    wstring scope_name = GetScopeName(ident);
    SymbolEntry* entry = TreeFactory::Instance()->MakeSymbolEntry(file_name, line_num,
                                                                  scope_name, type, false,
                                                                  current_method != NULL);

#ifdef _DEBUG
    Show(L"Adding variable: '" + scope_name + L"'", depth + 2);
#endif

    bool was_added = symbol_table->CurrentParseScope()->AddEntry(entry);
    if(!was_added) {
      ProcessError(L"Variable already defined in this scope: '" + ident + L"'");
    }

    if(Match(TOKEN_ASSIGN)) {
      Variable* variable = ParseVariable(ident, depth + 1);
      // FYI: can not specify array indices here
      declaration = TreeFactory::Instance()->MakeDeclaration(file_name, line_num, entry,
                                                             ParseAssignment(variable, depth + 1));
    }
    else {
      declaration = TreeFactory::Instance()->MakeDeclaration(file_name, line_num, entry);
    }
  }
  else {    
    // static
    bool is_static = false;
    if(Match(TOKEN_STATIC_ID)) {
      is_static = true;
      NextToken();

      if(!Match(TOKEN_COLON)) {
        ProcessError(L"Expected ','", TOKEN_COLON);
      }
      NextToken();
    }

    // type
    Type* type = ParseType(depth + 1);

    // add entry
    wstring scope_name = GetScopeName(ident);
    SymbolEntry* entry = TreeFactory::Instance()->MakeSymbolEntry(file_name, line_num,
                                                                  scope_name, type, is_static,
                                                                  current_method != NULL);

#ifdef _DEBUG
    Show(L"Adding variable: '" + scope_name + L"'", depth + 2);
#endif

    bool was_added = symbol_table->CurrentParseScope()->AddEntry(entry);
    if(!was_added) {
      ProcessError(L"Variable already defined in this scope: '" + ident + L"'");
    }

    if(Match(TOKEN_ASSIGN)) {
      Variable* variable = ParseVariable(ident, depth + 1);
      // FYI: can not specify array indices here
      declaration = TreeFactory::Instance()->MakeDeclaration(file_name, line_num, entry,
                                                             ParseAssignment(variable, depth + 1));
    } 
    else {
      declaration = TreeFactory::Instance()->MakeDeclaration(file_name, line_num, entry);
    }
  }

  return declaration;
}

/****************************
 * Parses a declaration list.
 ****************************/
DeclarationList* Parser::ParseDecelerationList(int depth)
{
#ifdef _DEBUG
  Show(L"Declaration Parameters", depth);
#endif

  if(!Match(TOKEN_OPEN_PAREN)) {
    ProcessError(L"Expected '('", TOKEN_OPEN_PAREN);
  }
  NextToken();

  DeclarationList* declarations = TreeFactory::Instance()->MakeDeclarationList();
  while(!Match(TOKEN_CLOSED_PAREN) && !Match(TOKEN_END_OF_STREAM)) {
    if(!Match(TOKEN_IDENT)) {
      ProcessError(TOKEN_IDENT);
    }
    // identifier
    const wstring& ident = scanner->GetToken()->GetIdentifier();
    NextToken();

    declarations->AddDeclaration(ParseDeclaration(ident, depth + 1));

    if(Match(TOKEN_COMMA)) {
      NextToken();
      if(!Match(TOKEN_IDENT)) {
        ProcessError(TOKEN_IDENT);
      }
    } 
    else if(!Match(TOKEN_CLOSED_PAREN)) {
      ProcessError(L"Expected ',' or ')'", TOKEN_CLOSED_BRACE);
      NextToken();
    }
  }
  if(!Match(TOKEN_CLOSED_PAREN)) {
    ProcessError(L"Expected ')'", TOKEN_CLOSED_PAREN);
  }
  NextToken();

  return declarations;
}

/****************************
 * Parses a expression list.
 ****************************/
ExpressionList* Parser::ParseExpressionList(int depth, ScannerTokenType open, ScannerTokenType closed)
{
#ifdef _DEBUG
  Show(L"Calling Parameters", depth);
#endif

  ExpressionList* expressions = TreeFactory::Instance()->MakeExpressionList();
  if(!Match(open)) {
    ProcessError(open);
  }
  NextToken();

  while(!Match(closed) && !Match(TOKEN_END_OF_STREAM)) {
    // expression
    expressions->AddExpression(ParseExpression(depth + 1));

    if(Match(TOKEN_COMMA)) {
      NextToken();
    } 
    else if(!Match(closed)) {
      ProcessError(L"Invalid token", closed);
      NextToken();
    }
  }

  if(!Match(closed)) {
    ProcessError(closed);
  }
  NextToken();

  return expressions;
}

/****************************
 * Parses array indices.
 ****************************/
ExpressionList* Parser::ParseIndices(int depth)
{
  const int line_num = GetLineNumber();
  const wstring &file_name = GetFileName();

  ExpressionList* expressions = NULL;
  if(Match(TOKEN_OPEN_BRACKET)) {
    expressions = TreeFactory::Instance()->MakeExpressionList();
    NextToken();

    if(Match(TOKEN_CLOSED_BRACKET)) {
      expressions->AddExpression(TreeFactory::Instance()->MakeVariable(file_name, line_num, L"#"));
      NextToken();
    }
    else {
      if(Match(TOKEN_COMMA)) {
        expressions->AddExpression(TreeFactory::Instance()->MakeVariable(file_name, line_num, L"#"));      
        while(Match(TOKEN_COMMA) && !Match(TOKEN_END_OF_STREAM)) {
          expressions->AddExpression(TreeFactory::Instance()->MakeVariable(file_name, line_num, L"#"));      
          NextToken();
        }
        if(!Match(TOKEN_CLOSED_BRACKET)) {
          ProcessError(L"Expected ',' or ']'", TOKEN_SEMI_COLON);
          NextToken();
        }
        NextToken();
      }
      else {
        while(!Match(TOKEN_CLOSED_BRACKET) && !Match(TOKEN_END_OF_STREAM)) {
          // expression
          expressions->AddExpression(ParseExpression(depth + 1));

          if(Match(TOKEN_COMMA)) {
            NextToken();
          } 
          else if(!Match(TOKEN_CLOSED_BRACKET)) {
            ProcessError(L"Expected ',' or ']'", TOKEN_SEMI_COLON);
            NextToken();
          }
        }

        if(!Match(TOKEN_CLOSED_BRACKET)) {
          ProcessError(L"Expected ']'", TOKEN_CLOSED_BRACKET);
        }
        NextToken();
      }
    }
  }

  return expressions;
}

/****************************
 * Parses an expression.
 ****************************/
Expression* Parser::ParseExpression(int depth)
{
  const int line_num = GetLineNumber();
  const wstring &file_name = GetFileName();

#ifdef _DEBUG
  Show(L"Expression", depth);
#endif

  Expression* expression = ParseLogic(depth + 1);
  //
  // parses a ternary conditional
  //
  if(Match(TOKEN_QUESTION)) {
#ifdef _DEBUG
    Show(L"Ternary conditional", depth);
#endif   
    NextToken();
    Expression* if_expression = ParseLogic(depth + 1);
    if(!Match(TOKEN_COLON)) {
      ProcessError(L"Expected ':'", TOKEN_COLON);
    }
    NextToken();
    return TreeFactory::Instance()->MakeCond(file_name, line_num, expression, 
                                             if_expression, ParseLogic(depth + 1));    
  }

  return expression;
}

/****************************
 * Parses a logical expression.
 * This method delegates support
 * for other types of expressions.
 ****************************/
Expression* Parser::ParseLogic(int depth)
{
  const int line_num = GetLineNumber();
  const wstring &file_name = GetFileName();

#ifdef _DEBUG
  Show(L"Boolean logic", depth);
#endif
  
  Expression* left;
  if(Match(TOKEN_NOT)) {
    NextToken();
    CalculatedExpression* not_expr = static_cast<CalculatedExpression*>(TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, NEQL_EXPR));
    not_expr->SetLeft(ParseMathLogic(depth + 1));
    not_expr->SetRight(TreeFactory::Instance()->MakeBooleanLiteral(file_name, line_num, true));
    left = not_expr;
  }
  else {
    left = ParseMathLogic(depth + 1);
  }
  
  CalculatedExpression* expression = NULL;
  while((Match(TOKEN_AND) || Match(TOKEN_OR)) && !Match(TOKEN_END_OF_STREAM)) {
    if(expression) {
      left = expression;
    }

    switch(GetToken()) {
    case TOKEN_AND:
      expression = TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, AND_EXPR);
      break;
    case TOKEN_OR:
      expression = TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, OR_EXPR);
      break;

    default:
      break;
    }
    NextToken();

    Expression* right = ParseLogic(depth + 1);
    if(expression) {
      expression->SetLeft(right);
      expression->SetRight(left);
    }
  }

  if(expression) {
    return expression;
  }

  // pass-thru
  return left;
}

/****************************
 * Parses a mathematical expression.
 * This method delegates support
 * for other types of expressions.
 ****************************/
Expression* Parser::ParseMathLogic(int depth)
{
  const int line_num = GetLineNumber();
  const wstring &file_name = GetFileName();

#ifdef _DEBUG
  Show(L"Boolean math", depth);
#endif

  Expression* left = ParseTerm(depth + 1);

  if(Match(TOKEN_LES) || Match(TOKEN_GTR) ||
     Match(TOKEN_LEQL) || Match(TOKEN_GEQL) ||
     Match(TOKEN_EQL) || Match(TOKEN_NEQL)) {
    CalculatedExpression* expression = NULL;
    switch(GetToken()) {
    case TOKEN_LES:
      expression = TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, LES_EXPR);
      break;
    case TOKEN_GTR:
      expression = TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, GTR_EXPR);
      break;
    case TOKEN_LEQL:
      expression = TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, LES_EQL_EXPR);
      break;
    case TOKEN_GEQL:
      expression = TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, GTR_EQL_EXPR);
      break;
    case TOKEN_EQL:
      expression = TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, EQL_EXPR);
      break;
    case TOKEN_NEQL:
      expression = TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, NEQL_EXPR);
      break;

    default:
      break;
    }
    NextToken();

    if(expression) {
      Expression* right = ParseTerm(depth + 1);
      expression->SetLeft(left);
      expression->SetRight(right);
    }

    return expression;
  }

  // pass-thru
  return left;
}

/****************************
 * Parses a mathematical term.
 * This method delegates support
 * for other types of expressions.
 ****************************/
Expression* Parser::ParseTerm(int depth)
{
  const int line_num = GetLineNumber();
  const wstring &file_name = GetFileName();

#ifdef _DEBUG
  Show(L"Term", depth);
#endif

  Expression* left = ParseFactor(depth + 1);
  if(!left) {
    return NULL;
  }

  if(!Match(TOKEN_ADD) && !Match(TOKEN_SUB)) {
    return left;
  }

  CalculatedExpression* expression = NULL;
  while((Match(TOKEN_ADD) || Match(TOKEN_SUB)) && !Match(TOKEN_END_OF_STREAM)) {
    if(expression) {
      CalculatedExpression* right;
      if(Match(TOKEN_ADD)) {
        right = TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, ADD_EXPR);
      } else {
        right = TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, SUB_EXPR);
      }
      NextToken();

      Expression* temp = ParseFactor(depth + 1);

      right->SetRight(temp);
      right->SetLeft(expression);
      expression = right;
    }
    // first time in loop
    else {
      if(Match(TOKEN_ADD)) {
        expression = TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, ADD_EXPR);
      } else {
        expression = TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, SUB_EXPR);
      }
      NextToken();

      Expression* temp = ParseFactor(depth + 1);

      if(expression) {
        expression->SetRight(temp);
        expression->SetLeft(left);
      }
    }
  }

  return expression;
}

/****************************
 * Parses a mathematical factor.
 * This method delegates support
 * for other types of expressions.
 ****************************/
Expression* Parser::ParseFactor(int depth)
{
  const int line_num = GetLineNumber();
  const wstring &file_name = GetFileName();

#ifdef _DEBUG
  Show(L"Factor", depth);
#endif

  Expression* left = ParseSimpleExpression(depth + 1);
  if(!Match(TOKEN_MUL) && !Match(TOKEN_DIV) && !Match(TOKEN_MOD) && 
     !Match(TOKEN_SHL) && !Match(TOKEN_SHR) && !Match(TOKEN_AND_ID) && 
     !Match(TOKEN_OR_ID) && !Match(TOKEN_XOR_ID)) {
    return left;
  }

  CalculatedExpression* expression = NULL;
  while((Match(TOKEN_MUL) || Match(TOKEN_DIV) || Match(TOKEN_MOD) || 
         Match(TOKEN_SHL) || Match(TOKEN_SHR) || Match(TOKEN_AND_ID) || 
         Match(TOKEN_OR_ID) || Match(TOKEN_XOR_ID)) &&
        !Match(TOKEN_END_OF_STREAM)) {
    if(expression) {
      CalculatedExpression* right;
      if(Match(TOKEN_MUL)) {
        right = TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, MUL_EXPR);
      } 
      else if(Match(TOKEN_MOD)) {
        right = TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, MOD_EXPR);
      } 
      else if(Match(TOKEN_SHL)) {
        right = TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, SHL_EXPR);
      } 
      else if(Match(TOKEN_SHR)) {
        right = TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, SHR_EXPR);
      } 
      else if(Match(TOKEN_AND_ID)) {
        right = TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, BIT_AND_EXPR);
      }
      else if(Match(TOKEN_OR_ID)) {
        right = TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, BIT_OR_EXPR);
      }
      else if(Match(TOKEN_XOR_ID)) {
        right = TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, BIT_XOR_EXPR);
      }
      else {
        right = TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, DIV_EXPR);
      }
      NextToken();

      Expression* temp = ParseSimpleExpression(depth + 1);
      right->SetRight(temp);
      right->SetLeft(expression);
      expression = right;
    }
    // first time in loop
    else {
      if(Match(TOKEN_MUL)) {
        expression = TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, MUL_EXPR);
      } 
      else if(Match(TOKEN_MOD)) {
        expression = TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, MOD_EXPR);
      }
      else if(Match(TOKEN_SHL)) {
        expression = TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, SHL_EXPR);
      }
      else if(Match(TOKEN_SHR)) {
        expression = TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, SHR_EXPR);
      }
      else if(Match(TOKEN_AND_ID)) {
        expression = TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, BIT_AND_EXPR);
      }
      else if(Match(TOKEN_OR_ID)) {
        expression = TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, BIT_OR_EXPR);
      }
      else if(Match(TOKEN_XOR_ID)) {
        expression = TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, BIT_XOR_EXPR);
      }
      else {
        expression = TreeFactory::Instance()->MakeCalculatedExpression(file_name, line_num, DIV_EXPR);
      }
      NextToken();

      Expression* temp = ParseSimpleExpression(depth + 1);
      if(expression) {
        expression->SetRight(temp);
        expression->SetLeft(left);
      }
    }
  }

  return expression;
}

/****************************
 * Parses a simple expression.
 ****************************/
Expression* Parser::ParseSimpleExpression(int depth)
{
  const int line_num = GetLineNumber();
  const wstring &file_name = GetFileName();

#ifdef _DEBUG
  Show(L"Simple expression", depth);
#endif

  Expression* expression = NULL;
  if(Match(TOKEN_IDENT) || IsBasicType(GetToken())) {
    wstring ident;
    switch(GetToken()) {
    case TOKEN_BOOLEAN_ID:
      ident = BOOL_CLASS_ID;
      NextToken();
      break;

    case TOKEN_BYTE_ID:
      ident = BYTE_CLASS_ID;
      NextToken();
      break;

    case TOKEN_INT_ID:
      ident = INT_CLASS_ID;
      NextToken();
      break;

    case TOKEN_FLOAT_ID:
      ident = FLOAT_CLASS_ID;
      NextToken();
      break;

    case TOKEN_CHAR_ID:
      ident = CHAR_CLASS_ID;
      NextToken();
      break;

    default:
      ident = ParseBundleName();
      break;
    }

    switch(GetToken()) {
      // method call
    case TOKEN_ASSESSOR:
    case TOKEN_OPEN_PAREN:
      if(!Match(TOKEN_AS_ID, SECOND_INDEX) && !Match(TOKEN_TYPE_OF_ID, SECOND_INDEX)) {
        expression = ParseMethodCall(ident, depth + 1);
      } 
      else {
        expression = ParseVariable(ident, depth + 1);
        ParseCastTypeOf(expression, depth + 1);
      }
      break;

    default:
      // variable
      expression = ParseVariable(ident, depth + 1);
      break;
    }
  } 
  else if(Match(TOKEN_OPEN_PAREN)) {
    NextToken();
    expression = ParseExpression(depth + 1);
    if(!Match(TOKEN_CLOSED_PAREN)) {
      ProcessError(L"Expected ')'", TOKEN_CLOSED_PAREN);
    }
    NextToken();
  } 
  else if(Match(TOKEN_SUB)) {
    NextToken();

    switch(GetToken()) {
    case TOKEN_INT_LIT:
      expression = TreeFactory::Instance()->MakeIntegerLiteral(file_name, line_num,
                                                               -scanner->GetToken()->GetIntLit());
      NextToken();
      break;

    case TOKEN_FLOAT_LIT:
      expression = TreeFactory::Instance()->MakeFloatLiteral(file_name, line_num,
                                                             -scanner->GetToken()->GetFloatLit());
      NextToken();
      break;

    default:
      ProcessError(L"Expected expression", TOKEN_SEMI_COLON);
      break;
    }
  } 
  else {
    switch(GetToken()) {
    case TOKEN_CHAR_LIT:
      expression = TreeFactory::Instance()->MakeCharacterLiteral(file_name, line_num,
                                                                 scanner->GetToken()->GetCharLit());
      NextToken();
      break;

    case TOKEN_INT_LIT:
      expression = TreeFactory::Instance()->MakeIntegerLiteral(file_name, line_num,
                                                               scanner->GetToken()->GetIntLit());
      NextToken();
      break;

    case TOKEN_FLOAT_LIT:
      expression = TreeFactory::Instance()->MakeFloatLiteral(file_name, line_num,
                                                             scanner->GetToken()->GetFloatLit());
      NextToken();
      break;

    case TOKEN_CHAR_STRING_LIT: {
      const wstring &ident = scanner->GetToken()->GetIdentifier();
      expression = TreeFactory::Instance()->MakeCharacterString(file_name, line_num, ident);
      NextToken();
    }
      break;

    case TOKEN_BAD_CHAR_STRING_LIT:
      ProcessError(L"Invalid escaped string literal", TOKEN_SEMI_COLON);
      NextToken();
      break;

    case TOKEN_OPEN_BRACKET:
      expression = ParseStaticArray(depth + 1);
      break;

    case TOKEN_NIL_ID:
      expression = TreeFactory::Instance()->MakeNilLiteral(file_name, line_num);
      NextToken();
      break;

    case TOKEN_TRUE_ID:
      expression = TreeFactory::Instance()->MakeBooleanLiteral(file_name, line_num, true);
      NextToken();
      break;

    case TOKEN_FALSE_ID:
      expression = TreeFactory::Instance()->MakeBooleanLiteral(file_name, line_num, false);
      NextToken();
      break;

    default:
      ProcessError(L"Expected expression", TOKEN_SEMI_COLON);
      break;
    }
  }

  // subsequent method calls
  if(Match(TOKEN_ASSESSOR) && !Match(TOKEN_AS_ID, SECOND_INDEX) && 
     !Match(TOKEN_TYPE_OF_ID, SECOND_INDEX)) {
    if(expression && expression->GetExpressionType() == VAR_EXPR) {
      expression = ParseMethodCall(static_cast<Variable*>(expression), depth + 1);
    } else {
      ParseMethodCall(expression, depth + 1);
    }
  }
  // type cast
  else {
    ParseCastTypeOf(expression, depth + 1);
  }

  return expression;
}

/****************************
 * Parses an explicit type
 * cast or typeof
 ****************************/
void Parser::ParseCastTypeOf(Expression* expression, int depth)
{
  if(Match(TOKEN_ASSESSOR)) {
    NextToken();

    if(Match(TOKEN_AS_ID)) {  
      NextToken();

      if(!Match(TOKEN_OPEN_PAREN)) {
        ProcessError(L"Expected '('", TOKEN_OPEN_PAREN);
      }
      NextToken();

      if(expression) {
        expression->SetCastType(ParseType(depth + 1));
      }

      if(!Match(TOKEN_CLOSED_PAREN)) {
        ProcessError(L"Expected ')'", TOKEN_CLOSED_PAREN);
      }
      NextToken();
    }
    else if(Match(TOKEN_TYPE_OF_ID)) {
      NextToken();

      if(!Match(TOKEN_OPEN_PAREN)) {
        ProcessError(L"Expected '('", TOKEN_OPEN_PAREN);
      }
      NextToken();

      if(expression) {
        expression->SetTypeOf(ParseType(depth + 1));
      }

      if(!Match(TOKEN_CLOSED_PAREN)) {
        ProcessError(L"Expected ')'", TOKEN_CLOSED_PAREN);
      }
      NextToken();
    }
    else {
      ProcessError(L"Expected cast or typeof", TOKEN_SEMI_COLON);
    }

    // subsequent method calls
    if(Match(TOKEN_ASSESSOR)) {
      ParseMethodCall(expression, depth + 1);
    }
  }
}

/****************************
 * Parses a method call.
 ****************************/
MethodCall* Parser::ParseMethodCall(int depth)
{
  const int line_num = GetLineNumber();
  const wstring &file_name = GetFileName();

#ifdef _DEBUG
  Show(L"Parent call", depth);
#endif

  NextToken();
  return TreeFactory::Instance()->MakeMethodCall(file_name, line_num, PARENT_CALL, L"",
                                                 ParseExpressionList(depth + 1));
}

/****************************
 * Parses a method call.
 ****************************/
MethodCall* Parser::ParseMethodCall(const wstring &ident, int depth)
{
  const int line_num = GetLineNumber();
  const wstring &file_name = GetFileName();

#ifdef _DEBUG
  Show(L"Method call", depth);
#endif

  MethodCall* method_call = NULL;
  if(Match(TOKEN_ASSESSOR)) {
    NextToken();

    // method call
    if(Match(TOKEN_IDENT)) {
      const wstring &method_ident = scanner->GetToken()->GetIdentifier();
      NextToken();

      if(Match(TOKEN_OPEN_PAREN)) {
        method_call = TreeFactory::Instance()->MakeMethodCall(file_name, line_num, ident, method_ident,
                                                              ParseExpressionList(depth + 1));
        if(Match(TOKEN_TILDE)) {
          NextToken();
          Type* func_rtrn = ParseType(depth + 1);
          method_call->SetFunctionReturn(func_rtrn);
        }
      } 
      else {
        method_call = TreeFactory::Instance()->MakeMethodCall(file_name, line_num, ident, method_ident);
      }
    }
    // new call
    else if(Match(TOKEN_NEW_ID)) {
      NextToken();
      // new array
      if(Match(TOKEN_OPEN_BRACKET)) {
        ExpressionList* expressions = ParseExpressionList(depth + 1, TOKEN_OPEN_BRACKET,
                                                          TOKEN_CLOSED_BRACKET);
        method_call = TreeFactory::Instance()->MakeMethodCall(file_name, line_num, NEW_ARRAY_CALL,
                                                              ident, expressions);
      }
      // new object
      else {
        method_call = TreeFactory::Instance()->MakeMethodCall(file_name, line_num, NEW_INST_CALL, ident,
                                                              ParseExpressionList(depth + 1));
        if(Match(TOKEN_OPEN_BRACE) || Match(TOKEN_IMPLEMENTS_ID)) {
          ParseAnonymousClass(method_call, depth);
        } 
      }
    }
    else if(Match(TOKEN_AS_ID)) {
      Variable* variable = ParseVariable(ident, depth + 1);

      NextToken();
      if(!Match(TOKEN_OPEN_PAREN)) {
        ProcessError(L"Expected '('", TOKEN_OPEN_PAREN);
      }
      NextToken();

      if(variable) {
        variable->SetCastType(ParseType(depth + 1));
      }

      if(!Match(TOKEN_CLOSED_PAREN)) {
        ProcessError(L"Expected ')'", TOKEN_CLOSED_PAREN);
      }
      NextToken();

      // subsequent method calls
      if(Match(TOKEN_ASSESSOR)) {
        method_call = ParseMethodCall(variable, depth + 1);
        method_call->SetCastType(variable->GetCastType());
      }
    }
    else if(Match(TOKEN_TYPE_OF_ID)) {
      Variable* variable = ParseVariable(ident, depth + 1);

      NextToken();
      if(!Match(TOKEN_OPEN_PAREN)) {
        ProcessError(L"Expected '('", TOKEN_OPEN_PAREN);
      }
      NextToken();

      if(variable) {
        variable->SetTypeOf(ParseType(depth + 1));
      }

      if(!Match(TOKEN_CLOSED_PAREN)) {
        ProcessError(L"Expected ')'", TOKEN_CLOSED_PAREN);
      }
      NextToken();
    }
    else {
      ProcessError(L"Expected identifier", TOKEN_SEMI_COLON);
    }
  }
  // method call
  else if(Match(TOKEN_OPEN_PAREN)) {
    wstring klass_name = current_class->GetName();
    method_call = TreeFactory::Instance()->MakeMethodCall(file_name, line_num, klass_name,
                                                          ident, ParseExpressionList(depth + 1));
    if(Match(TOKEN_TILDE)) {
      NextToken();
      method_call->SetFunctionReturn(ParseType(depth + 1));
    }
  } 
  else {
    ProcessError(L"Expected identifier", TOKEN_SEMI_COLON);
  }

  // subsequent method calls
  if(Match(TOKEN_ASSESSOR) && !Match(TOKEN_AS_ID, SECOND_INDEX) && 
     !Match(TOKEN_TYPE_OF_ID, SECOND_INDEX)) {
    ParseMethodCall(method_call, depth + 1);
  }
  // type cast
  else {
    ParseCastTypeOf(method_call, depth + 1);
  }

  return method_call;
}

/****************************
 * Parses a method call. This
 * is either an expression method
 * or a call from a method return
 * value.
 ****************************/
void Parser::ParseMethodCall(Expression* expression, int depth)
{
#ifdef _DEBUG
  Show(L"Method call", depth);
#endif

  NextToken();

  if(!Match(TOKEN_IDENT)) {
    ProcessError(TOKEN_IDENT);
  }
  // identifier
  const wstring &ident = scanner->GetToken()->GetIdentifier();
  NextToken();

  if(expression) {
    expression->SetMethodCall(ParseMethodCall(ident, depth + 1));
    // subsequent method calls
    if(Match(TOKEN_ASSESSOR) && !Match(TOKEN_AS_ID, SECOND_INDEX) && 
       !Match(TOKEN_TYPE_OF_ID, SECOND_INDEX)) {
      ParseMethodCall(expression->GetMethodCall(), depth + 1);
    }
    // type cast
    else {
      ParseCastTypeOf(expression->GetMethodCall(), depth + 1);
    }
  }
}

/****************************
 * Parses a method call. This
 * is either an expression method
 * or a call from a method return
 * value.
 ****************************/
MethodCall* Parser::ParseMethodCall(Variable* variable, int depth)
{
  const int line_num = GetLineNumber();
  const wstring &file_name = GetFileName();

  NextToken();
  const wstring &method_ident = scanner->GetToken()->GetIdentifier();
  NextToken();

  MethodCall* call = TreeFactory::Instance()->MakeMethodCall(file_name, line_num, variable, method_ident,
                                                             ParseExpressionList(depth + 1));

  if(Match(TOKEN_ASSESSOR) && !Match(TOKEN_AS_ID, SECOND_INDEX) && 
     !Match(TOKEN_TYPE_OF_ID, SECOND_INDEX)) {
    call->SetMethodCall(ParseMethodCall(variable->GetName(), depth + 1));
  }

  return call;
}

/****************************
 * Parses an anonymous class
 ****************************/
void Parser::ParseAnonymousClass(MethodCall* method_call, int depth)
{
  const int line_num = GetLineNumber();
  const wstring &file_name = GetFileName();

  if(prev_method && current_method) {
    ProcessError(L"Invalid nested anonymous classes");
    return;
  }
  
  const wstring cls_name = method_call->GetVariableName() + L".#Anonymous." + 
    ToString(anonymous_class_id++) + L'#';
  
  vector<wstring> interface_names;
  if(Match(TOKEN_IMPLEMENTS_ID)) {
    NextToken();
    while(!Match(TOKEN_OPEN_BRACE) && !Match(TOKEN_END_OF_STREAM)) {
      if(!Match(TOKEN_IDENT)) {
        ProcessError(TOKEN_IDENT);
      }
      // identifier
      const wstring& ident = ParseBundleName();
      interface_names.push_back(ident);      
      if(Match(TOKEN_COMMA)) {
        NextToken();
        if(!Match(TOKEN_IDENT)) {
          ProcessError(TOKEN_IDENT);
        }
      } 
      else if(!Match(TOKEN_OPEN_BRACE)) {
        ProcessError(L"Expected ',' or '{'", TOKEN_OPEN_BRACE);
        NextToken();
      }
    }
  }

  // statement list
  if(!Match(TOKEN_OPEN_BRACE)) {
    ProcessError(L"Expected '{'", TOKEN_OPEN_BRACE);
  }
  NextToken();
  
  Class* klass = TreeFactory::Instance()->MakeClass(file_name, line_num, cls_name, 
                                                    method_call->GetVariableName(),
                                                    interface_names, true);
  
  Class* prev_class = current_class;
  prev_method = current_method;
  current_method = NULL;
  current_class = klass;
  symbol_table->NewParseScope();

  // add '@self' entry
  SymbolEntry* entry = TreeFactory::Instance()->MakeSymbolEntry(file_name, line_num, GetScopeName(SELF_ID),
                                                                TypeFactory::Instance()->MakeType(CLASS_TYPE, cls_name), 
                                                                false, false, true);

  symbol_table->CurrentParseScope()->AddEntry(entry);

  while(!Match(TOKEN_CLOSED_BRACE) && !Match(TOKEN_END_OF_STREAM)) {
    // parse 'method | function | declaration'
    if(Match(TOKEN_FUNCTION_ID)) {
      Method* method = ParseMethod(true, false, depth + 1);
      bool was_added = klass->AddMethod(method);
      if(!was_added) {
        ProcessError(L"Method or function already defined or overloaded '" + method->GetName() + L"'", method);
      }
    } 
    else if(Match(TOKEN_METHOD_ID) || Match(TOKEN_NEW_ID)) {
      Method* method = ParseMethod(false, false, depth + 1);
      bool was_added = klass->AddMethod(method);
      if(!was_added) {
        ProcessError(L"Method or function already defined or overloaded '" + method->GetName() + L"'", method);
      }
    } 
    else if(Match(TOKEN_IDENT)) {
      const wstring &ident = scanner->GetToken()->GetIdentifier();
      NextToken();

      klass->AddStatement(ParseDeclaration(ident, depth + 1));
      if(!Match(TOKEN_SEMI_COLON)) {
        ProcessError(L"Expected ';'", TOKEN_SEMI_COLON);
      }
      NextToken();
    } 
    else {
      ProcessError(L"Expected declaration", TOKEN_SEMI_COLON);
      NextToken();
    }
  }

  if(!Match(TOKEN_CLOSED_BRACE)) {
    ProcessError(L"Expected '}'", TOKEN_CLOSED_BRACE);
  }
  NextToken();
  
  symbol_table->PreviousParseScope(current_class->GetName());
  
  method_call->SetAnonymousClass(klass);
  method_call->SetVariableName(cls_name);
  klass->SetAnonymousCall(method_call);
  current_bundle->AddClass(klass);
  
  current_class = prev_class;
  current_method = prev_method;
  prev_method = NULL;
}

/****************************
 * Parses an 'if' statement
 ****************************/
If* Parser::ParseIf(int depth)
{
  const int line_num = GetLineNumber();
  const wstring &file_name = GetFileName();

#ifdef _DEBUG
  Show(L"If", depth);
#endif

  NextToken();
  if(!Match(TOKEN_OPEN_PAREN)) {
    ProcessError(L"Expected '('", TOKEN_OPEN_PAREN);
  }
  NextToken();

  Expression* expression = ParseExpression(depth + 1);
  if(!Match(TOKEN_CLOSED_PAREN)) {
    ProcessError(L"Expected ')'", TOKEN_CLOSED_PAREN);
  }
  NextToken();
  symbol_table->CurrentParseScope()->NewParseScope();
  StatementList* if_statements =  ParseStatementList(depth + 1);
  symbol_table->CurrentParseScope()->PreviousParseScope();

  If* if_stmt;
  if(Match(TOKEN_ELSE_ID) && Match(TOKEN_IF_ID, SECOND_INDEX)) {
    NextToken();
    If* next = ParseIf(depth + 1);
    if_stmt = TreeFactory::Instance()->MakeIf(file_name, line_num, expression, if_statements, next);
  } 
  else if(Match(TOKEN_ELSE_ID)) {
    NextToken();
    if_stmt = TreeFactory::Instance()->MakeIf(file_name, line_num, expression, if_statements);
    symbol_table->CurrentParseScope()->NewParseScope();
    if_stmt->SetElseStatements(ParseStatementList(depth + 1));
    symbol_table->CurrentParseScope()->PreviousParseScope();
  } 
  else {
    if_stmt = TreeFactory::Instance()->MakeIf(file_name, line_num, expression, if_statements);
  }

  return if_stmt;
}

/****************************
 * Parses a 'do/while' statement
 ****************************/
DoWhile* Parser::ParseDoWhile(int depth)
{
  const int line_num = GetLineNumber();
  const wstring &file_name = GetFileName();

#ifdef _DEBUG
  Show(L"Do/While", depth);
#endif

  NextToken();
  symbol_table->CurrentParseScope()->NewParseScope();
  StatementList* statements =  ParseStatementList(depth + 1);

  if(!Match(TOKEN_WHILE_ID)) {
    ProcessError(L"Expected 'while'", TOKEN_SEMI_COLON);
  }
  NextToken();

  if(!Match(TOKEN_OPEN_PAREN)) {
    ProcessError(L"Expected '('", TOKEN_OPEN_PAREN);
  }
  NextToken();

  Expression* expression = ParseExpression(depth + 1);
  if(!Match(TOKEN_CLOSED_PAREN)) {
    ProcessError(L"Expected ')'", TOKEN_CLOSED_PAREN);
  }
  NextToken();
  symbol_table->CurrentParseScope()->PreviousParseScope();

  return TreeFactory::Instance()->MakeDoWhile(file_name, line_num, expression, statements);
}

/****************************
 * Parses a 'while' statement
 ****************************/
While* Parser::ParseWhile(int depth)
{
  const int line_num = GetLineNumber();
  const wstring &file_name = GetFileName();

#ifdef _DEBUG
  Show(L"While", depth);
#endif

  NextToken();
  if(!Match(TOKEN_OPEN_PAREN)) {
    ProcessError(L"Expected '('", TOKEN_OPEN_PAREN);
  }
  NextToken();

  Expression* expression = ParseExpression(depth + 1);
  if(!Match(TOKEN_CLOSED_PAREN)) {
    ProcessError(L"Expected ')'", TOKEN_CLOSED_PAREN);
  }
  NextToken();
  symbol_table->CurrentParseScope()->NewParseScope();
  StatementList* statements =  ParseStatementList(depth + 1);
  symbol_table->CurrentParseScope()->PreviousParseScope();

  return TreeFactory::Instance()->MakeWhile(file_name, line_num, expression, statements);
}

/****************************
 * Parses a critical section
 ****************************/
CriticalSection* Parser::ParseCritical(int depth)
{
  const int line_num = GetLineNumber();
  const wstring &file_name = GetFileName();

#ifdef _DEBUG
  Show(L"Critical Section", depth);
#endif

  NextToken();
  if(!Match(TOKEN_OPEN_PAREN)) {
    ProcessError(L"Expected '('", TOKEN_OPEN_PAREN);
  }
  NextToken();

  // initialization statement
  if(!Match(TOKEN_IDENT)) {
    ProcessError(TOKEN_IDENT);
  }
  const wstring ident = scanner->GetToken()->GetIdentifier();
  Variable* variable = ParseVariable(ident, depth + 1);

  NextToken();
  if(!Match(TOKEN_CLOSED_PAREN)) {
    ProcessError(L"Expected ')'", TOKEN_CLOSED_PAREN);
  }
  NextToken();

  symbol_table->CurrentParseScope()->NewParseScope(); 
  StatementList* statements =  ParseStatementList(depth + 1);
  symbol_table->CurrentParseScope()->PreviousParseScope();

  return TreeFactory::Instance()->MakeCriticalSection(file_name, line_num, variable, statements);
}

/****************************
 * Parses an 'each' statement
 ****************************/
For* Parser::ParseEach(int depth)
{
  const int line_num = GetLineNumber();
  const wstring &file_name = GetFileName();

#ifdef _DEBUG
  Show(L"Each", depth);
#endif

  NextToken();
  symbol_table->CurrentParseScope()->NewParseScope();
  if(!Match(TOKEN_OPEN_PAREN)) {
    ProcessError(L"Expected ')'", TOKEN_OPEN_PAREN);
  }
  NextToken();

  // initialization statement
  if(!Match(TOKEN_IDENT)) {
    ProcessError(TOKEN_IDENT);
  }
  const wstring count_ident = scanner->GetToken()->GetIdentifier();
  NextToken();

  // add entry
  Type* type = TypeFactory::Instance()->MakeType(INT_TYPE);
  const wstring count_scope_name = GetScopeName(count_ident);
  SymbolEntry* entry = TreeFactory::Instance()->MakeSymbolEntry(file_name, line_num,
                                                                count_scope_name, type, false,
                                                                current_method != NULL);

#ifdef _DEBUG
  Show(L"Adding variable: '" + count_scope_name + L"'", depth + 2);
#endif

  bool was_added = symbol_table->CurrentParseScope()->AddEntry(entry);
  if(!was_added) {
    ProcessError(L"Variable already defined in this scope: '" + count_ident + L"'");
  }
  Variable* count_left = TreeFactory::Instance()->MakeVariable(file_name, line_num, count_ident);
  Expression* count_right = TreeFactory::Instance()->MakeIntegerLiteral(file_name, line_num, 0);
  Assignment* count_assign = TreeFactory::Instance()->MakeAssignment(file_name, line_num, 
                                                                     count_left, count_right);					  
  Statement* pre_stmt = TreeFactory::Instance()->MakeDeclaration(file_name, line_num, 
                                                                 entry, count_assign);

  if(!Match(TOKEN_COLON)) {
    ProcessError(L"Expected ':'", TOKEN_COLON);
  }
  NextToken();

  if(!Match(TOKEN_IDENT)) {
    ProcessError(TOKEN_IDENT);
  }
  const wstring list_ident = scanner->GetToken()->GetIdentifier();
  NextToken();

  // conditional expression
  Variable* list_left = TreeFactory::Instance()->MakeVariable(file_name, line_num, count_ident);
  ExpressionList* list_expressions = TreeFactory::Instance()->MakeExpressionList();
  Expression* list_right = TreeFactory::Instance()->MakeMethodCall(file_name, line_num, list_ident, 
                                                                   L"Size", list_expressions);  
  CalculatedExpression* cond_expr = TreeFactory::Instance()->MakeCalculatedExpression(file_name, 
                                                                                      line_num, 
                                                                                      LES_EXPR);
  cond_expr->SetLeft(list_left);
  cond_expr->SetRight(list_right);
  symbol_table->CurrentParseScope()->NewParseScope();

  // update statement
  Variable* update_left = TreeFactory::Instance()->MakeVariable(file_name, line_num, count_ident);
  Expression* update_right = TreeFactory::Instance()->MakeIntegerLiteral(file_name, line_num, 1);  
  Statement* update_stmt = TreeFactory::Instance()->MakeOperationAssignment(file_name, line_num, 
                                                                            update_left, update_right,
                                                                            ADD_ASSIGN_STMT);
  if(!Match(TOKEN_CLOSED_PAREN)) {
    ProcessError(L"Expected ')'", TOKEN_CLOSED_PAREN);
  }
  NextToken();

  // statement list
  StatementList* statements =  ParseStatementList(depth + 1);
  symbol_table->CurrentParseScope()->PreviousParseScope();
  symbol_table->CurrentParseScope()->PreviousParseScope();

  return TreeFactory::Instance()->MakeFor(file_name, line_num, pre_stmt, 
                                          cond_expr, update_stmt, statements);
}

/****************************
 * Parses a 'for' statement
 ****************************/
For* Parser::ParseFor(int depth)
{
  const int line_num = GetLineNumber();
  const wstring &file_name = GetFileName();

#ifdef _DEBUG
  Show(L"For", depth);
#endif

  NextToken();
  symbol_table->CurrentParseScope()->NewParseScope();
  if(!Match(TOKEN_OPEN_PAREN)) {
    ProcessError(L"Expected '('", TOKEN_OPEN_PAREN);
  }
  NextToken();
  // pre-statement
  Statement* pre_stmt = ParseStatement(depth + 1);
  // conditional
  Expression* cond_expr = ParseExpression(depth + 1);
  if(!Match(TOKEN_SEMI_COLON)) {
    ProcessError(L"Expected ';'", TOKEN_SEMI_COLON);
  }
  NextToken();
  symbol_table->CurrentParseScope()->NewParseScope();
  // update statement
  Statement* update_stmt = ParseStatement(depth + 1);
  if(!Match(TOKEN_CLOSED_PAREN)) {
    ProcessError(L"Expected ')'", TOKEN_CLOSED_PAREN);
  }
  NextToken();
  // statement list
  StatementList* statements =  ParseStatementList(depth + 1);
  symbol_table->CurrentParseScope()->PreviousParseScope();
  symbol_table->CurrentParseScope()->PreviousParseScope();

  return TreeFactory::Instance()->MakeFor(file_name, line_num, pre_stmt, cond_expr,
                                          update_stmt, statements);
}

/****************************
 * Parses a 'select' statement
 ****************************/
Select* Parser::ParseSelect(int depth)
{
  const int line_num = GetLineNumber();
  const wstring &file_name = GetFileName();

#ifdef _DEBUG
  Show(L"Select", depth);
#endif

  NextToken();
  if(!Match(TOKEN_OPEN_PAREN)) {
    ProcessError(L"Expected '('", TOKEN_OPEN_PAREN);
  }
  NextToken();

  Expression* eval_expression = ParseExpression(depth + 1);
  if(!Match(TOKEN_CLOSED_PAREN)) {
    ProcessError(L"Expected ')'", TOKEN_CLOSED_PAREN);
  }
  NextToken();

  if(!Match(TOKEN_OPEN_BRACE)) {
    ProcessError(L"Expected '('", TOKEN_OPEN_BRACE);
  }
  NextToken();


  StatementList* other = NULL;
  vector<StatementList*> statement_lists;
  map<ExpressionList*, StatementList*> statement_map;
  while((Match(TOKEN_LABEL_ID) || Match(TOKEN_OTHER_ID)) && !Match(TOKEN_END_OF_STREAM)) {
    bool is_other_label = false;
    ExpressionList* labels = TreeFactory::Instance()->MakeExpressionList();
    // parse labels
    while((Match(TOKEN_LABEL_ID) || Match(TOKEN_OTHER_ID)) && !Match(TOKEN_END_OF_STREAM)) {
      if(Match(TOKEN_LABEL_ID)) {
        NextToken();
        labels->AddExpression(ParseSimpleExpression(depth + 1));
        if(!Match(TOKEN_COLON)) {
          ProcessError(L"Expected ':'", TOKEN_COLON);
        }
        NextToken();
      } 
      else {
        if(is_other_label)  {
          ProcessError(L"Duplicate 'other' label", TOKEN_OTHER_ID);
        }
        is_other_label = true;
        NextToken();
        if(!Match(TOKEN_COLON)) {
          ProcessError(L"Expected ':'", TOKEN_COLON);
        }
        NextToken();
      }
    }

    // parse statements
    symbol_table->CurrentParseScope()->NewParseScope();
    StatementList* statements =  ParseStatementList(depth + 1);
    symbol_table->CurrentParseScope()->PreviousParseScope();

    // 'other' label
    if(is_other_label) {      
      if(!other) {
        other = statements;      
      }
      else {
        ProcessError(L"Duplicate 'other' label", TOKEN_CLOSED_BRACE);
      }
    } 
    // named label
    else {
      statement_map.insert(pair<ExpressionList*, StatementList*>(labels, statements));
    }

    // note: order matters during contextual analysis due 
    // to the way the nested symbol table manages scope
    statement_lists.push_back(statements);
  }

  if(!Match(TOKEN_CLOSED_BRACE)) {
    ProcessError(L"Expected '}'", TOKEN_CLOSED_BRACE);
  }
  NextToken();

  return TreeFactory::Instance()->MakeSelect(file_name, line_num, eval_expression,
                                             statement_map, statement_lists, other);
}

/****************************
 * Parses a return statement
 ****************************/
Return* Parser::ParseReturn(int depth)
{
  const int line_num = GetLineNumber();
  const wstring &file_name = GetFileName();
#ifdef _DEBUG
  Show(L"Return", depth);
#endif
  NextToken();

  Expression* expression = NULL;
  if(!Match(TOKEN_SEMI_COLON)) {
    expression = ParseExpression(depth + 1);
  }
  return TreeFactory::Instance()->MakeReturn(file_name, line_num, expression);
}

/****************************
 * Parses an assignment statement
 ****************************/
Assignment* Parser::ParseAssignment(Variable* variable, int depth)
{
  const int line_num = GetLineNumber();
  const wstring &file_name = GetFileName();

#ifdef _DEBUG
  Show(L"Assignment", depth);
#endif

  NextToken();
  Assignment* assignment = TreeFactory::Instance()->MakeAssignment(file_name, line_num, variable,
                                                                   ParseExpression(depth + 1));

  return assignment;
}

/****************************
 * Parses a data type identifier.
 ****************************/
Type* Parser::ParseType(int depth)
{
#ifdef _DEBUG
  Show(L"Data Type", depth);
#endif

  Type* type = NULL;
  switch(GetToken()) {
  case TOKEN_BYTE_ID:
    type = TypeFactory::Instance()->MakeType(BYTE_TYPE);
    NextToken();
    break;

  case TOKEN_INT_ID:
    type = TypeFactory::Instance()->MakeType(INT_TYPE);
    NextToken();
    break;

  case TOKEN_FLOAT_ID:
    type = TypeFactory::Instance()->MakeType(FLOAT_TYPE);
    NextToken();
    break;

  case TOKEN_CHAR_ID:
    type = TypeFactory::Instance()->MakeType(CHAR_TYPE);
    NextToken();
    break;

  case TOKEN_NIL_ID:
    type = TypeFactory::Instance()->MakeType(NIL_TYPE);
    NextToken();
    break;

  case TOKEN_BOOLEAN_ID:
    type = TypeFactory::Instance()->MakeType(BOOLEAN_TYPE);
    NextToken();
    break;

  case TOKEN_IDENT: {
    const wstring ident = ParseBundleName();
    type = TypeFactory::Instance()->MakeType(CLASS_TYPE, ident);
  }
    break;

  case TOKEN_OPEN_PAREN: {
    NextToken();

    vector<Type*> func_params;
    while(!Match(TOKEN_CLOSED_PAREN) && !Match(TOKEN_END_OF_STREAM)) {
      Type* param = ParseType(depth + 1);
      if(param) {
        func_params.push_back(param);
      }

      if(Match(TOKEN_COMMA)) {
        NextToken();
      } 
      else if(!Match(TOKEN_CLOSED_PAREN)) {
        ProcessError(L"Expected ',' or ')'", TOKEN_CLOSED_BRACE);
        NextToken();
      }
    }
    NextToken();

    if(!Match(TOKEN_TILDE)) {
      ProcessError(L"Expected '~'", TOKEN_TILDE);
    }
    NextToken();

    Type* func_rtrn = ParseType(depth + 1);

    return TypeFactory::Instance()->MakeType(func_params, func_rtrn);
  }
    break;

  default:
    ProcessError(L"Expected type", TOKEN_SEMI_COLON);
    break;
  }

  if(type) {
    int dimension = 0;

    if(Match(TOKEN_OPEN_BRACKET)) {
      NextToken();
      dimension++;
      while(!Match(TOKEN_CLOSED_BRACKET) && !Match(TOKEN_END_OF_STREAM)) {
        dimension++;
        if(Match(TOKEN_COMMA)) {
          NextToken();
        } 
        else if(!Match(TOKEN_CLOSED_BRACKET)) {
          ProcessError(L"Expected ',' or ';'", TOKEN_SEMI_COLON);
          NextToken();
        }
      }

      if(!Match(TOKEN_CLOSED_BRACKET)) {
        ProcessError(L"Expected ']'", TOKEN_CLOSED_BRACKET);
      }
      NextToken();
    }
    type->SetDimension(dimension);
  }

  return type;
}
