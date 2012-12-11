#include <cassert>
#include <cstdio>
#include <stack>

#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

static const uint64_t DATA_SIZE = 30000;
static const char *DATA_NAME = "data";
static const char *PTR_NAME = "ptr";

struct Loop {
	BasicBlock *Entry, *Body, *Exit;
	PHINode *DataPtrBody, *DataPtrExit;
};

int main() {
  // LLVM context.
  LLVMContext &Context = getGlobalContext();
  Module MainModule("brainfuck program", Context);
  IRBuilder<> Builder(Context);

  // Some useful constants.
  const IntegerType *CellType = Type::getInt8Ty(Context);
  Constant *One = ConstantInt::get(CellType, 1);

  // Function prototypes for the shim.
  FunctionType *PutFunctionType = FunctionType::get(
      Type::getVoidTy(Context) /* return type */,
      std::vector<const Type *>(1, CellType) /* argument types */,
      false /* var args */);
  Function *PutFunction = Function::Create(PutFunctionType,
      Function::ExternalLinkage, "brainfuck_put", &MainModule);
  FunctionType *GetFunctionType = FunctionType::get(
      CellType /* return type */,
      std::vector<const Type *>() /* argument types */,
      false /* var args */);
  Function *GetFunction = Function::Create(GetFunctionType,
      Function::ExternalLinkage, "brainfuck_get", &MainModule);

  // Global data for a brainfuck program.
  ArrayType *DataType = ArrayType::get(CellType, DATA_SIZE);
  GlobalVariable *Data = new GlobalVariable(
      MainModule, DataType, false /* constant */,
      GlobalVariable::InternalLinkage, Constant::getNullValue(DataType),
      DATA_NAME);
  Value *DataPtr = Builder.CreateConstInBoundsGEP2_32(Data, 0, 0, PTR_NAME);

  // Main function definition.
  FunctionType *MainFunctionType = FunctionType::get(
      Type::getVoidTy(Context) /* return type */,
      std::vector<const Type *>() /* argument types */,
      false /* var args */);
  Function *MainFunction = Function::Create(MainFunctionType,
      Function::ExternalLinkage, "brainfuck_main", &MainModule);
  BasicBlock *MainBlock = BasicBlock::Create(Context, "entry", MainFunction);
  Builder.SetInsertPoint(MainBlock);

  // Code generation.
  int c;
  Value *Value;
  std::stack<Loop> Loops;
  Loop ThisLoop;
  while ((c = getchar()) != EOF) {
    switch (c) {
      case '>':
        DataPtr = Builder.CreateConstGEP1_32(DataPtr, 1, PTR_NAME);
        break;
      case '<':
        DataPtr = Builder.CreateConstGEP1_32(DataPtr, -1, PTR_NAME);
        break;
      case '+':
        Value = Builder.CreateLoad(DataPtr);
        Value = Builder.CreateAdd(Value, One);
        Builder.CreateStore(Value, DataPtr);
        break;
      case '-':
        Value = Builder.CreateLoad(DataPtr);
        Value = Builder.CreateSub(Value, One);
        Builder.CreateStore(Value, DataPtr);
        break;
      case '.':
        Value = Builder.CreateLoad(DataPtr);
        Builder.CreateCall(PutFunction, Value);
        break;
      case ',':
        Value = Builder.CreateCall(GetFunction);
        Builder.CreateStore(Value, DataPtr);
        break;

      case '[':
        // Prepare data for the stack.
        ThisLoop.Entry = Builder.GetInsertBlock();
        ThisLoop.Body = BasicBlock::Create(Context, "loop", MainFunction);
        ThisLoop.Exit = BasicBlock::Create(Context, "exit", MainFunction);

        // Emit the beginning conditional branch.
    	  Value = Builder.CreateLoad(DataPtr);
    	  Value = Builder.CreateIsNotNull(Value);
    	  Builder.CreateCondBr(Value, ThisLoop.Body, ThisLoop.Exit);

    	  // Define the pointer after the loop.
    	  Builder.SetInsertPoint(ThisLoop.Exit);
    	  ThisLoop.DataPtrExit = Builder.CreatePHI(DataPtr->getType(), PTR_NAME);
    	  ThisLoop.DataPtrExit->addIncoming(DataPtr, ThisLoop.Entry);

    	  // Define the pointer within the loop.
    	  Builder.SetInsertPoint(ThisLoop.Body);
    	  ThisLoop.DataPtrBody = Builder.CreatePHI(DataPtr->getType(), PTR_NAME);
    	  ThisLoop.DataPtrBody->addIncoming(DataPtr, ThisLoop.Entry);

    	  // Continue generating code within the loop.
    	  DataPtr = ThisLoop.DataPtrBody;
    	  Loops.push(ThisLoop);
    	  break;

    	case ']':
        // Pop data off the stack.
    	  if (Loops.empty()) {
        	fputs("] requires matching [\n", stderr);
        	abort();
        }
    	  ThisLoop = Loops.top();
    	  Loops.pop();

    	  // Finish off the phi nodes.
    	  ThisLoop.DataPtrBody->addIncoming(DataPtr, Builder.GetInsertBlock());
    	  ThisLoop.DataPtrExit->addIncoming(DataPtr, Builder.GetInsertBlock());

    	  // Emit the ending conditional branch.
    	  Value = Builder.CreateLoad(DataPtr);
    	  Value = Builder.CreateIsNotNull(Value);
    	  Builder.CreateCondBr(Value, ThisLoop.Body, ThisLoop.Exit);

    	  // Move insertion after the loop.
    	  ThisLoop.Exit->moveAfter(Builder.GetInsertBlock());
    	  DataPtr = ThisLoop.DataPtrExit;
    	  Builder.SetInsertPoint(ThisLoop.Exit);
    	  break;
    }
  }

  // Ensure all loops have been closed.
  if (!Loops.empty()) {
  	fputs("[ requires matching ]\n", stderr);
  	abort();
  }

  // Finish off brainfuck_main and dump.
  Builder.CreateRetVoid();
  MainModule.print(outs(), NULL /* assembly annotation writer */);
}
