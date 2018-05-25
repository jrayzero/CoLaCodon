#include "seq/varexpr.h"
#include "seq/chunk.h"

using namespace seq;
using namespace llvm;

Chunk::Chunk(Expr *key) :
    Stage("chunk", types::ArrayType::get(), types::ArrayType::get()), key(key)
{
}

Chunk::Chunk(Func *key) : Chunk(new FuncExpr(key))
{
}

Chunk::Chunk() : Chunk((Expr *)nullptr)
{
}

void Chunk::validate()
{
	if (getPrev() && getPrev()->getOutType()->isGeneric(types::ArrayType::get()))
		in = out = getPrev()->getOutType();

	Stage::validate();
}

void Chunk::codegen(Module *module)
{
	ensurePrev();
	validate();

	LLVMContext& context = module->getContext();
	BasicBlock *preambleBlock = getBase()->getPreamble();
	BasicBlock *entry = prev->getAfter();
	Function *func = entry->getParent();

	Value *f = key ? key->codegen(getBase(), entry) : nullptr;
	IRBuilder<> builder(entry);
	Value *ptr = builder.CreateLoad(getSafe(prev->outs, SeqData::ARRAY));
	Value *len = builder.CreateLoad(getSafe(prev->outs, SeqData::LEN));

	BasicBlock *loop = BasicBlock::Create(context, "chunk", func);
	builder.CreateBr(loop);
	builder.SetInsertPoint(loop);

	PHINode *control = builder.CreatePHI(seqIntLLVM(context), 2, "i");
	Value *cond = builder.CreateICmpSLT(control, len);
	Value *next = builder.CreateAdd(control, oneLLVM(context), "next");

	BasicBlock *body = BasicBlock::Create(context, "body", func);
	BranchInst *branch = builder.CreateCondBr(cond, body, body);  // we set false-branch below
	builder.SetInsertPoint(body);

	auto *type = dynamic_cast<types::ArrayType *>(getInType());
	assert(type != nullptr);

	ValMap firstInChunk = makeValMap();
	ValMap firstInChunkKey = makeValMap();
	type->getBaseType()->codegenLoad(getBase(),
	                                 firstInChunk,
	                                 body,
	                                 ptr,
	                                 control);
	if (key)
		key->getType()->call(getBase(), firstInChunk, firstInChunkKey, f, body);

	PHINode *control2;
	{
		/* inner loop */
		BasicBlock *loop2 = BasicBlock::Create(context, "chunk_inner", func);
		builder.CreateBr(loop2);
		builder.SetInsertPoint(loop2);

		control2 = builder.CreatePHI(seqIntLLVM(context), 2, "j");
		Value *cond2 = builder.CreateICmpSLT(control2, len);
		Value *next2 = builder.CreateAdd(control2, oneLLVM(context), "next");

		BasicBlock *body2 = BasicBlock::Create(context, "body", func);
		BranchInst *branch2 = builder.CreateCondBr(cond2, body2, body2);  // we set false-branch below
		builder.SetInsertPoint(body2);

		ValMap nextInChunk = makeValMap();
		ValMap nextInChunkKey = makeValMap();
		type->getBaseType()->codegenLoad(getBase(),
		                                 nextInChunk,
		                                 body2,
		                                 ptr,
		                                 control2);
		if (key)
			key->getType()->call(getBase(), nextInChunk, nextInChunkKey, f, body2);

		Value *eq = key ? key->getType()->getCallType(type->getBaseType())->checkEq(getBase(), firstInChunkKey, nextInChunkKey, body2) :
		                  type->getBaseType()->checkEq(getBase(), firstInChunk, nextInChunk, body2);

		control2->addIncoming(next, body);
		control2->addIncoming(next2, body2);

		BasicBlock *exit2 = BasicBlock::Create(context, "exit", func);
		branch2->setSuccessor(1, exit2);

		builder.CreateCondBr(eq, loop2, exit2);

		block = exit2;
		builder.SetInsertPoint(block);
	}

	Value *subptr = builder.CreateGEP(ptr, control);
	Value *sublen = builder.CreateSub(control2, control);
	Value *subptrVar = makeAlloca(
	                     ConstantPointerNull::get(
	                       PointerType::get(type->getBaseType()->getLLVMType(context), 0)), preambleBlock);
	Value *sublenVar = makeAlloca(zeroLLVM(context), preambleBlock);
	builder.CreateStore(subptr, subptrVar);
	builder.CreateStore(sublen, sublenVar);
	outs->insert({SeqData::ARRAY, subptrVar});
	outs->insert({SeqData::LEN, sublenVar});

	codegenNext(module);

	builder.SetInsertPoint(getAfter());
	builder.CreateBr(loop);

	control->addIncoming(zeroLLVM(context), entry);
	control->addIncoming(control2, getAfter());

	BasicBlock *exit = BasicBlock::Create(context, "exit", func);
	branch->setSuccessor(1, exit);
	prev->setAfter(exit);
}

Chunk& Chunk::make(Expr *key)
{
	return *new Chunk(key);
}

Chunk& Chunk::make(Func& key)
{
	return *new Chunk(&key);
}

Chunk& Chunk::make()
{
	return *new Chunk();
}
