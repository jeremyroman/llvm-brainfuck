#include <cstdio>

#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Support/IRBuilder.h>

using namespace llvm;

static const uint64_t DATA_SIZE = 30000;
static const char *DATA_NAME = "data";
static const char *PTR_NAME = "ptr";

static LLVMContext &Context = getGlobalContext();
static Module MainModule("brainfuck program", Context);
static IRBuilder<> Builder(Context);

int main() {
  // Useful constants.
  const IntegerType *cellType = Type::getInt8Ty(Context);
  const PointerType *ptrType = cellType->getPointerTo();
  Constant *one = ConstantInt::get(cellType, 1);

  // Called by brainfuck code.
  FunctionType *PutFunctionType = FunctionType::get(
      Type::getVoidTy(Context) /* return type */,
      std::vector<const Type *>(1, cellType) /* argument types */,
      false /* var args */);
  Function *PutFunction = Function::Create(PutFunctionType,
      Function::ExternalLinkage, "brainfuck_put", &MainModule);
  FunctionType *GetFunctionType = FunctionType::get(
      cellType /* return type */,
      std::vector<const Type *>() /* argument types */,
      false /* var args */);
  Function *GetFunction = Function::Create(GetFunctionType,
      Function::ExternalLinkage, "brainfuck_get", &MainModule);

  // Global data for a brainfuck program.
  ArrayType *dataType = ArrayType::get(cellType, DATA_SIZE);
  GlobalVariable *data = new GlobalVariable(
      MainModule, dataType, false /* constant */,
      GlobalVariable::InternalLinkage, Constant::getNullValue(dataType),
      DATA_NAME);
  Value *dataPtr = Builder.CreateConstInBoundsGEP2_32(data, 0, 0, PTR_NAME);

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
  Value *value;
  while ((c = getchar()) != EOF) {
    switch (c) {
      case '>':
        dataPtr = Builder.CreateConstGEP1_32(dataPtr, 1, PTR_NAME);
        break;
      case '<':
        dataPtr = Builder.CreateConstGEP1_32(dataPtr, -1, PTR_NAME);
        break;
      case '+':
        value = Builder.CreateLoad(dataPtr);
        value = Builder.CreateAdd(value, one);
        Builder.CreateStore(value, dataPtr);
        break;
      case '-':
        value = Builder.CreateLoad(dataPtr);
        value = Builder.CreateSub(value, one);
        Builder.CreateStore(value, dataPtr);
        break;
      case '.':
        value = Builder.CreateLoad(dataPtr);
        Builder.CreateCall(PutFunction, value);
        break;
      case ',':
        value = Builder.CreateCall(GetFunction);
        Builder.CreateStore(value, dataPtr);
        break;
    }
  }

  // Finish off brainfuck_main and dump.
  Builder.CreateRetVoid();
  MainModule.dump();
}
