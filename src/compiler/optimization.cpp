/***************************************************************************
 * Platform independent language optimizer.
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

#include "optimization.h"

using namespace backend;

void ItermediateOptimizer::Optimize()
{
#ifdef _DEBUG
  wcout << L"\n--------- Optimizing Code ---------" << endl;
#endif

  // classes...
  vector<IntermediateClass*> klasses = program->GetClasses();
  for(size_t i = 0; i < klasses.size(); ++i) {
    // methods...
    vector<IntermediateMethod*> methods = klasses[i]->GetMethods();
    for(size_t j = 0; j < methods.size(); j++) {
      current_method = methods[j];
#ifdef _DEBUG
      wcout << L"Optimizing method, pass 1: name='" << current_method->GetName() << "'" << endl;
#endif
      current_method->SetBlocks(OptimizeMethod(current_method->GetBlocks()));
    }
    
    for(size_t j = 0; j < methods.size(); j++) {
      current_method = methods[j];
#ifdef _DEBUG
      wcout << L"Optimizing method, pass 2: name='" << current_method->GetName() << "'" << endl;
#endif
      current_method->SetBlocks(InlineMethod(current_method->GetBlocks()));
    }
  }
}

vector<IntermediateBlock*> ItermediateOptimizer::InlineMethod(vector<IntermediateBlock*> inputs)
{
  if(optimization_level > 2) {
    // inline methods
#ifdef _DEBUG
    wcout << L"  Method inlining..." << endl;
#endif
    vector<IntermediateBlock*> outputs;
    while(!inputs.empty()) {
      IntermediateBlock* tmp = inputs.front();
      outputs.push_back(InlineMethod(tmp));
      // delete old block
      inputs.erase(inputs.begin());
      delete tmp;
      tmp = NULL;
    }

    return outputs;
  }
  else {
    return inputs;
  }
}

vector<IntermediateBlock*> ItermediateOptimizer::OptimizeMethod(vector<IntermediateBlock*> inputs)
{
  // clean up jump addresses
#ifdef _DEBUG
  wcout << L"  Clean up jumps..." << endl;
#endif

  vector<IntermediateBlock*> jump_blocks;
  // clean up jumps
  while(!inputs.empty()) {
    IntermediateBlock* tmp = inputs.front();
    jump_blocks.push_back(CleanJumps(tmp));
    // delete old block
    inputs.erase(inputs.begin());
    delete tmp;
    tmp = NULL;
  }
  
  vector<IntermediateBlock*> useless_instrs_blocks;
  // remove useless instructions
#ifdef _DEBUG
  wcout << L"  Clean up Instructions..." << endl;
#endif
  while(!jump_blocks.empty()) {
    IntermediateBlock* tmp = jump_blocks.front();
    useless_instrs_blocks.push_back(RemoveUselessInstructions(tmp));
    // delete old block
    jump_blocks.erase(jump_blocks.begin());
    delete tmp;
    tmp = NULL;
  }
  
  vector<IntermediateBlock*> folded_float_blocks;
  if(optimization_level > 0) {
    vector<IntermediateBlock*> getter_setter_blocks;
    // getter/setter inlining
#ifdef _DEBUG
    wcout << L"  Getter/setter inlining..." << endl;
#endif
    while(!useless_instrs_blocks.empty()) {
      IntermediateBlock* tmp = useless_instrs_blocks.front();
      getter_setter_blocks.push_back(InlineSettersGetters(tmp));
      // delete old block
      useless_instrs_blocks.erase(useless_instrs_blocks.begin());
      delete tmp;
      tmp = NULL;
    }
        
    // fold integers
#ifdef _DEBUG
    wcout << L"  Folding integers..." << endl;
#endif
    vector<IntermediateBlock*> folded_int_blocks;
    while(!getter_setter_blocks.empty()) {
      IntermediateBlock* tmp = getter_setter_blocks.front();
      folded_int_blocks.push_back(FoldIntConstants(tmp));
      // delete old block
      getter_setter_blocks.erase(getter_setter_blocks.begin());
      delete tmp;
      tmp = NULL;
    }
    
    // fold floats
#ifdef _DEBUG
    wcout << L"  Folding floats..." << endl;
#endif
    while(!folded_int_blocks.empty()) {
      IntermediateBlock* tmp = folded_int_blocks.front();
      folded_float_blocks.push_back(FoldFloatConstants(tmp));
      // delete old block
      folded_int_blocks.erase(folded_int_blocks.begin());
      delete tmp;
      tmp = NULL;
    }
  } 
  else {
    return useless_instrs_blocks;
  }

  vector<IntermediateBlock*> strength_reduced_blocks;
  if(optimization_level > 1) {
    // reduce strength
#ifdef _DEBUG
    wcout << L"  Strength reduction..." << endl;
#endif
    while(!folded_float_blocks.empty()) {
      IntermediateBlock* tmp = folded_float_blocks.front();
      strength_reduced_blocks.push_back(StrengthReduction(tmp));
      // delete old block
      folded_float_blocks.erase(folded_float_blocks.begin());
      delete tmp;
      tmp = NULL;
    }
  } 
  else {
    return folded_float_blocks;
  }

  vector<IntermediateBlock*> instruction_replaced_blocks;
  if(optimization_level > 2) {
    // instruction replacement
#ifdef _DEBUG
    wcout << L"  Instruction replacement..." << endl;
#endif
    while(!strength_reduced_blocks.empty()) {
      IntermediateBlock* tmp = strength_reduced_blocks.front();
      instruction_replaced_blocks.push_back(InstructionReplacement(tmp));
      // delete old block
      strength_reduced_blocks.erase(strength_reduced_blocks.begin());
      delete tmp;
      tmp = NULL;
    }
  } 
  else {
    return strength_reduced_blocks;
  }
  
  return instruction_replaced_blocks;
}

IntermediateBlock* ItermediateOptimizer::RemoveUselessInstructions(IntermediateBlock* inputs)
{
  IntermediateBlock* outputs = new IntermediateBlock;
  deque<IntermediateInstruction*> working_stack;
  
  vector<IntermediateInstruction*> input_instrs = inputs->GetInstructions();
  for(size_t i = 0; i < input_instrs.size(); ++i) {
    IntermediateInstruction* instr = input_instrs[i];

    switch(instr->GetType()) {
    case LOAD_INT_VAR:
      if(instr->GetOperand2() == LOCL) {
	working_stack.push_front(instr);
      }
      else {
	while(!working_stack.empty()) {
	  outputs->AddInstruction(working_stack.back());
	  working_stack.pop_back();
	}
	outputs->AddInstruction(instr);
      }
      break;
      
    case STOR_INT_VAR:
      if(instr->GetOperand2() == LOCL && !working_stack.empty() && 
	 working_stack.front()->GetType() == LOAD_INT_VAR) {
	if(instr->GetOperand() == working_stack.front()->GetOperand()) {
	  working_stack.pop_front();
	}
	else {
	  while(!working_stack.empty()) {
	    outputs->AddInstruction(working_stack.back());
	    working_stack.pop_back();
	  }
	  outputs->AddInstruction(instr);
	}
      }
      else {
	while(!working_stack.empty()) {
	  outputs->AddInstruction(working_stack.back());
	  working_stack.pop_back();
	}
	outputs->AddInstruction(instr);
      }
      break;
     
    default:
      while(!working_stack.empty()) {
	outputs->AddInstruction(working_stack.back());
	working_stack.pop_back();
      }
      outputs->AddInstruction(instr);
      break;
    }
  }
  
  return outputs; 
}

IntermediateBlock* ItermediateOptimizer::CleanJumps(IntermediateBlock* inputs)
{
  IntermediateBlock* outputs = new IntermediateBlock;
  deque<IntermediateInstruction*> working_stack;

  vector<IntermediateInstruction*> input_instrs = inputs->GetInstructions();
  for(size_t i = 0; i < input_instrs.size(); ++i) {
    IntermediateInstruction* instr = input_instrs[i];

    switch(instr->GetType()) {
    case JMP:
      working_stack.push_front(instr);
      break;

    case LBL:
      // ignore jump to next instruction
      if(!working_stack.empty() && working_stack.front()->GetType() == JMP && 
	 working_stack.front()->GetOperand() == instr->GetOperand()) {
        working_stack.pop_front();
      }
      // add back in reverse order
      while(!working_stack.empty()) {
        outputs->AddInstruction(working_stack.back());
        working_stack.pop_back();
      }
      outputs->AddInstruction(instr);
      break;

    default:
      // add back in reverse order
      while(!working_stack.empty()) {
        outputs->AddInstruction(working_stack.back());
        working_stack.pop_back();
      }
      outputs->AddInstruction(instr);
      break;
    }
  }
  // order matters...
  while(!working_stack.empty()) {
    outputs->AddInstruction(working_stack.back());
    working_stack.pop_back();
  }

  return outputs;
}

IntermediateBlock* ItermediateOptimizer::InlineSettersGetters(IntermediateBlock* inputs)
{
  IntermediateBlock* outputs = new IntermediateBlock;
  
  vector<IntermediateInstruction*> input_instrs = inputs->GetInstructions();
  for(size_t i = 0; i < input_instrs.size(); ++i) {
    IntermediateInstruction* instr = input_instrs[i];
    if(instr->GetType() == MTHD_CALL) {
      IntermediateMethod* mthd_called = program->GetClass(instr->GetOperand())->GetMethod(instr->GetOperand2());
      int status = CanInlineSetterGetter(mthd_called);
      //  getter instance pattern
      if(status == 0) {
	vector<IntermediateBlock*> blocks = mthd_called->GetBlocks();
	vector<IntermediateInstruction*> instrs = blocks[0]->GetInstructions();
	outputs->AddInstruction(instrs[1]);
      }
      // getter instance pattern
      else if(status == 1) {
	vector<IntermediateBlock*> blocks = mthd_called->GetBlocks();
	vector<IntermediateInstruction*> instrs = blocks[0]->GetInstructions();
	outputs->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, POP_INT));
	outputs->AddInstruction(instrs[0]);
      }
      // character print pattern
      else if(status == 2) {
	vector<IntermediateBlock*> blocks = mthd_called->GetBlocks();
	vector<IntermediateInstruction*> instrs = blocks[0]->GetInstructions();
	outputs->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, POP_INT));
	outputs->AddInstruction(instrs[2]);
	outputs->AddInstruction(instrs[3]);
      }
      else if(status == 3) {
	vector<IntermediateBlock*> blocks = mthd_called->GetBlocks();
	vector<IntermediateInstruction*> instrs = blocks[0]->GetInstructions();
	outputs->AddInstruction(instrs[3]);
      }
      else {
	outputs->AddInstruction(instr);
      }
    }
    else {
      outputs->AddInstruction(instr);
    }
  }
  
  return outputs;
}


IntermediateBlock* ItermediateOptimizer::InstructionReplacement(IntermediateBlock* inputs)
{
  IntermediateBlock* outputs = new IntermediateBlock;
  deque<IntermediateInstruction*> working_stack;

  size_t i = 0;
  vector<IntermediateInstruction*> input_instrs = inputs->GetInstructions();
  while(i < input_instrs.size() && (input_instrs[i]->GetType() == STOR_INT_VAR ||
                                    input_instrs[i]->GetType() == STOR_FLOAT_VAR)) {
    outputs->AddInstruction(input_instrs[i++]);
  }

  for(; i < input_instrs.size(); ++i) {
    IntermediateInstruction* instr = input_instrs[i];

    switch(instr->GetType()) {
    case LOAD_INT_VAR:
    case LOAD_FLOAT_VAR:
      ReplacementInstruction(instr, working_stack, outputs);
      break;

    case STOR_INT_VAR:
    case STOR_FLOAT_VAR:
      if(!working_stack.empty() && working_stack.front()->GetType() == STOR_INT_VAR) {
	// order matters...
	while(!working_stack.empty()) {
	  outputs->AddInstruction(working_stack.back());
	  working_stack.pop_back();
	}
	outputs->AddInstruction(instr);
      }
      else {
	working_stack.push_front(instr);
      }
      break;
      
    default:
      // order matters...
      while(!working_stack.empty()) {
        outputs->AddInstruction(working_stack.back());
        working_stack.pop_back();
      }
      outputs->AddInstruction(instr);
      break;
    }
  }
  // order matters...
  while(!working_stack.empty()) {
    outputs->AddInstruction(working_stack.back());
    working_stack.pop_back();
  }

  return outputs;
}

void ItermediateOptimizer::ReplacementInstruction(IntermediateInstruction* instr,
						  deque<IntermediateInstruction*> &working_stack,
						  IntermediateBlock* outputs)
{
  if(!working_stack.empty()) {
    IntermediateInstruction* top_instr = working_stack.front();
    if(top_instr->GetType() == STOR_INT_VAR && instr->GetType() == LOAD_INT_VAR &&
       instr->GetOperand() == top_instr->GetOperand() &&
       instr->GetOperand2() == top_instr->GetOperand2()) {
      outputs->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, COPY_INT_VAR, top_instr->GetOperand(), top_instr->GetOperand2()));
      working_stack.pop_front();
    } 
    else if(top_instr->GetType() == STOR_FLOAT_VAR && instr->GetType() == LOAD_FLOAT_VAR &&
	    instr->GetOperand() == top_instr->GetOperand() &&
	    instr->GetOperand2() == top_instr->GetOperand2()) {
      outputs->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, COPY_FLOAT_VAR, top_instr->GetOperand(), top_instr->GetOperand2()));
      working_stack.pop_front();
    } 
    else {
      // order matters...
      while(!working_stack.empty()) {
        outputs->AddInstruction(working_stack.back());
        working_stack.pop_back();
      }
      outputs->AddInstruction(instr);
    }
  } 
  else {
    outputs->AddInstruction(instr);
  }
}

IntermediateBlock* ItermediateOptimizer::StrengthReduction(IntermediateBlock* inputs)
{
  IntermediateBlock* outputs = new IntermediateBlock;
  deque<IntermediateInstruction*> working_stack;

  vector<IntermediateInstruction*> input_instrs = inputs->GetInstructions();
  for(size_t i = 0; i < input_instrs.size(); ++i) {
    IntermediateInstruction* instr = input_instrs[i];
    switch(instr->GetType()) {
    case LOAD_INT_LIT:
      working_stack.push_front(instr);
      break;

    case LOAD_INT_VAR:
      working_stack.push_front(instr);
      break;

    case MUL_INT:
    case DIV_INT:
      CalculateReduction(instr, working_stack, outputs);
      break;

    default:
      // order matters...
      while(!working_stack.empty()) {
        outputs->AddInstruction(working_stack.back());
        working_stack.pop_back();
      }
      outputs->AddInstruction(instr);
      break;
    }
  }
  // order matters...
  while(!working_stack.empty()) {
    outputs->AddInstruction(working_stack.back());
    working_stack.pop_back();
  }

  return outputs;
}

void ItermediateOptimizer::CalculateReduction(IntermediateInstruction* instr,
					      deque<IntermediateInstruction*> &working_stack,
					      IntermediateBlock* outputs)
{
  if(working_stack.size() > 1) {
    IntermediateInstruction* top_instr = working_stack.front();
    working_stack.pop_front();
    if(top_instr->GetType() == LOAD_INT_LIT) {
      if(working_stack.front()->GetType() == LOAD_INT_VAR) {
        ApplyReduction(top_instr, instr, top_instr, working_stack, outputs);
      } 
      else {
        AddBackReduction(instr, top_instr, working_stack, outputs);
      }
    } 
    else if(working_stack.front()->GetType() == LOAD_INT_LIT) {
      if(top_instr->GetType() == LOAD_INT_VAR) {
        ApplyReduction(working_stack.front(), instr, top_instr, working_stack, outputs);
      } 
      else {
        AddBackReduction(instr, top_instr, working_stack, outputs);
      }
    } 
    else {
      AddBackReduction(instr, top_instr, working_stack, outputs);
    }
  } 
  else {
    // order matters...
    while(!working_stack.empty()) {
      outputs->AddInstruction(working_stack.back());
      working_stack.pop_back();
    }
    outputs->AddInstruction(instr);
  }
}

void ItermediateOptimizer::ApplyReduction(IntermediateInstruction* test, 
					  IntermediateInstruction* instr, 
					  IntermediateInstruction* top_instr,
					  deque<IntermediateInstruction*> &working_stack, 
					  IntermediateBlock* outputs)
{
  int shift = 0;
  switch(test->GetOperand()) {
  case 2:
    shift = 1;
    break;

  case 4:
    shift = 2;
    break;

  case 8:
    shift = 3;
    break;

  case 16:
    shift = 4;
    break;

  case 32:
    shift = 5;
    break;

  case 64:
    shift = 6;
    break;

  case 128:
    shift = 7;
    break;

  case 256:
    shift = 8;
    break;

  default:
    AddBackReduction(instr, top_instr, working_stack, outputs);
    break;
  }

  deque<IntermediateInstruction*> rewrite_instrs;
  if(shift) {
    rewrite_instrs.push_back(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, shift));
    // exclude literal
    if(working_stack.front()->GetType() != LOAD_INT_LIT) {
      rewrite_instrs.push_back(working_stack.front());
    }
    if(top_instr->GetType() != LOAD_INT_LIT) {
      rewrite_instrs.push_back(top_instr);
    }
    working_stack.pop_front();
    // shift left or right
    if(instr->GetType() == MUL_INT) {
      rewrite_instrs.push_back(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, SHL_INT, shift));
    } 
    else {
      rewrite_instrs.push_back(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, SHR_INT, shift));
    }
  }
  
  // add original instructions
  while(!working_stack.empty()) {
    outputs->AddInstruction(working_stack.back());
    working_stack.pop_back();
  }
  
  // add rewritten instructions
  for(size_t i = 0; i < rewrite_instrs.size(); ++i) {
    outputs->AddInstruction(rewrite_instrs[i]);
  }
}

void ItermediateOptimizer::AddBackReduction(IntermediateInstruction* instr, IntermediateInstruction* top_instr,
					    deque<IntermediateInstruction*> &working_stack, IntermediateBlock* outputs)
{
  // order matters...
  while(!working_stack.empty()) {
    outputs->AddInstruction(working_stack.back());
    working_stack.pop_back();
  }
  outputs->AddInstruction(top_instr);
  outputs->AddInstruction(instr);
}

IntermediateBlock* ItermediateOptimizer::InlineMethod(IntermediateBlock* inputs)
{
  set<IntermediateMethod*> inlined_mthds;
  IntermediateBlock* outputs = new IntermediateBlock;
  vector<IntermediateInstruction*> input_instrs = inputs->GetInstructions();

  set<int> lbl_jmp_offsets;
  for(size_t i = 0; i < input_instrs.size(); ++i) {
    IntermediateInstruction* instr = input_instrs[i];
    switch(instr->GetType()) {
    case LBL:
    case JMP:
      lbl_jmp_offsets.insert(instr->GetOperand());
      break;

    default:
      break;
    }
  }
  lbl_jmp_offsets.insert(unconditional_label + 1);
  
  for(size_t i = 0; i < input_instrs.size(); ++i) {
    IntermediateInstruction* instr = input_instrs[i];

    if(instr->GetType() == MTHD_CALL) {
      IntermediateMethod* mthd_called = program->GetClass(instr->GetOperand())->GetMethod(instr->GetOperand2());
      // checked called method to determine if it can be inlined
      if(CanInlineMethod(mthd_called, inlined_mthds, lbl_jmp_offsets)) {
	// calculate local offset, +2 in case last variable is a double 
	int local_instr_offset = GetLastLocalOffset(current_method) + 2;
	
	// ajust local space
	current_method->SetSpace(current_method->GetSpace() + sizeof(INT_VALUE) * 2 + mthd_called->GetSpace());
	
	// fetch inline instructions for called method
	// vector<IntermediateBlock*> mthd_called_blocks = OptimizeMethod(mthd_called->GetBlocks());
	vector<IntermediateBlock*> mthd_called_blocks = mthd_called->GetBlocks();
	vector<IntermediateInstruction*> mthd_called_instrs = mthd_called_blocks[0]->GetInstructions();
	
	// handle the storing of local instance
	// TODO: might be a better way to do this given the constrains
	outputs->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, STOR_INT_VAR,
										 local_instr_offset, LOCL));
	
	// inline instructions
	// TODO: inline methods with multiple return values
	// bool found_rtrn = false;
	// int end = ++unconditional_label;
	for(size_t j = 0; j < mthd_called_instrs.size() - 1; j++) {
	  IntermediateInstruction* mthd_called_instr = mthd_called_instrs[j];
	  switch(mthd_called_instr->GetType()) {
	  case LOAD_INT_VAR:
	  case STOR_INT_VAR:
	  case COPY_INT_VAR:
	  case LOAD_FLOAT_VAR:
	  case STOR_FLOAT_VAR:
	  case COPY_FLOAT_VAR:
	    if(mthd_called_instr->GetOperand2() == LOCL) {
	      outputs->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, mthd_called_instr->GetType(),
										       mthd_called_instr->GetOperand() + local_instr_offset + 1,  
										       LOCL));
	    }
	    else if(mthd_called_instr->GetOperand2() == INST) {
	      outputs->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, mthd_called_instr->GetType(),
										       mthd_called_instr->GetOperand(), INST));	      
	    }
	    else {
	      outputs->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, mthd_called_instr->GetType(),
										       mthd_called_instr->GetOperand(), CLS));
	    }
	    break;

	  case LOAD_INST_MEM:
	    outputs->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, local_instr_offset, LOCL));
	    break;
	    
	    /*
	      case RTRN:
	      outputs->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, JMP, end, -1));
	      found_rtrn = true;
	      break;
	    */

	  default:
	    outputs->AddInstruction(mthd_called_instr);
	    break;
	  }
	}
	/*
	  if(found_rtrn) {
	  outputs->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LBL, end));
	  }
	*/
	inlined_mthds.insert(mthd_called);
      }	
      else {
	outputs->AddInstruction(instr);
      }
    }
    else {
      outputs->AddInstruction(instr);
    }
  }
  
  return outputs;
}

IntermediateBlock* ItermediateOptimizer::FoldIntConstants(IntermediateBlock* inputs)
{
  IntermediateBlock* outputs = new IntermediateBlock;
  deque<IntermediateInstruction*> working_stack;

  vector<IntermediateInstruction*> input_instrs = inputs->GetInstructions();
  for(size_t i = 0; i < input_instrs.size(); ++i) {
    IntermediateInstruction* instr = input_instrs[i];

    switch(instr->GetType()) {
    case LOAD_INT_LIT:
      working_stack.push_front(instr);
      break;

    case ADD_INT:
    case SUB_INT:
    case MUL_INT:
    case DIV_INT:
    case MOD_INT:
    case BIT_AND_INT:
    case BIT_OR_INT:
    case BIT_XOR_INT:
      CalculateIntFold(instr, working_stack, outputs);
      break;

    default:
      // add back in reverse order
      while(!working_stack.empty()) {
        outputs->AddInstruction(working_stack.back());
        working_stack.pop_back();
      }
      outputs->AddInstruction(instr);
      break;
    }
  }
  // order matters...
  while(!working_stack.empty()) {
    outputs->AddInstruction(working_stack.back());
    working_stack.pop_back();
  }

  return outputs;
}

void ItermediateOptimizer::CalculateIntFold(IntermediateInstruction* instr,
					    deque<IntermediateInstruction*> &working_stack,
					    IntermediateBlock* outputs)
{
  if(working_stack.size() == 1) {
    outputs->AddInstruction(working_stack.front());
    working_stack.pop_front();
    outputs->AddInstruction(instr);
  } else if(working_stack.size() > 1) {
    IntermediateInstruction* left = working_stack.front();
    working_stack.pop_front();

    IntermediateInstruction* right = working_stack.front();
    working_stack.pop_front();

    switch(instr->GetType()) {
    case ADD_INT: {
      INT_VALUE value = left->GetOperand() + right->GetOperand();
      working_stack.push_front(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, value));
    }
      break;

    case SUB_INT: {
      INT_VALUE value = left->GetOperand() - right->GetOperand();
      working_stack.push_front(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, value));
    }
      break;

    case MUL_INT: {
      INT_VALUE value = left->GetOperand() * right->GetOperand();
      working_stack.push_front(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, value));
    }
      break;

    case DIV_INT: {
      INT_VALUE value = left->GetOperand() / right->GetOperand();
      working_stack.push_front(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, value));
    }
      break;

    case MOD_INT: {
      INT_VALUE value = left->GetOperand() % right->GetOperand();
      working_stack.push_front(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, value));
    }
      break;
      
    case BIT_AND_INT: {
      INT_VALUE value = left->GetOperand() & right->GetOperand();
      working_stack.push_front(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, value));
    }
      break;

    case BIT_OR_INT: {
      INT_VALUE value = left->GetOperand() | right->GetOperand();
      working_stack.push_front(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, value));
    }
      break;

    case BIT_XOR_INT: {
      INT_VALUE value = left->GetOperand() ^ right->GetOperand();
      working_stack.push_front(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, value));
    }
      break;

    default:
      break;
    }
  } 
  else {
    outputs->AddInstruction(instr);
  }
}

IntermediateBlock* ItermediateOptimizer::FoldFloatConstants(IntermediateBlock* inputs)
{
  IntermediateBlock* outputs = new IntermediateBlock;
  deque<IntermediateInstruction*> working_stack;

  vector<IntermediateInstruction*> input_instrs = inputs->GetInstructions();
  for(size_t i = 0; i < input_instrs.size(); ++i) {
    IntermediateInstruction* instr = input_instrs[i];

    switch(instr->GetType()) {
    case LOAD_FLOAT_LIT:
      working_stack.push_front(instr);
      break;

    case ADD_FLOAT:
    case SUB_FLOAT:
    case MUL_FLOAT:
    case DIV_FLOAT:
      CalculateFloatFold(instr, working_stack, outputs);
      break;

    default:
      // add back in reverse order
      while(!working_stack.empty()) {
        outputs->AddInstruction(working_stack.back());
        working_stack.pop_back();
      }
      outputs->AddInstruction(instr);
      break;
    }
  }
  // order matters...
  while(!working_stack.empty()) {
    outputs->AddInstruction(working_stack.back());
    working_stack.pop_back();
  }

  return outputs;
}

void ItermediateOptimizer::CalculateFloatFold(IntermediateInstruction* instr,
					      deque<IntermediateInstruction*> &working_stack,
					      IntermediateBlock* outputs)
{
  if(working_stack.size() == 1) {
    outputs->AddInstruction(working_stack.front());
    working_stack.pop_front();
    outputs->AddInstruction(instr);
  } 
  else if(working_stack.size() > 1) {
    IntermediateInstruction* left = working_stack.front();
    working_stack.pop_front();

    IntermediateInstruction* right = working_stack.front();
    working_stack.pop_front();

    switch(instr->GetType()) {
    case ADD_FLOAT: {
      FLOAT_VALUE value = left->GetOperand4() + right->GetOperand4();
      working_stack.push_front(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_FLOAT_LIT, value));
    }
      break;

    case SUB_FLOAT: {
      FLOAT_VALUE value = left->GetOperand4() - right->GetOperand4();
      working_stack.push_front(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_FLOAT_LIT, value));
    }
      break;

    case MUL_FLOAT: {
      FLOAT_VALUE value = left->GetOperand4() * right->GetOperand4();
      working_stack.push_front(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_FLOAT_LIT, value));
    }
      break;
    
    case DIV_FLOAT: {
      FLOAT_VALUE value = left->GetOperand4() / right->GetOperand4();
      working_stack.push_front(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_FLOAT_LIT, value));
    }
      break;
      
    default:
      break;
    }
  } 
  else {
    outputs->AddInstruction(instr);
  }
}
