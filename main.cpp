#include <cstdio>

#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

static const uint64_t DATA_SIZE = 30000;
static const char *DATA_NAME = "data";
static const char *PTR_NAME = "ptr";

int main() {
  // LLVM context.
  LLVMContext &Context = getGlobalContext();
  Module MainModule("brainfuck program", Context);
  IRBuilder<> Builder(Context);

  // Useful constants.
  const IntegerType *CellType = Type::getInt8Ty(Context);
  Constant *One = ConstantInt::get(CellType, 1);

  // Called by brainfuck code.
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

  // Function signature: void brainfuck_main().
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
    }
  }

  // Finish off brainfuck_main and dump.
  Builder.CreateRetVoid();
  MainModule.print(outs(), NULL /* assembly annotation writer */);
}
