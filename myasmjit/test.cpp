#include "../src/asmjit/asmjit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include "test.h"

using namespace std;
using namespace asmjit;


int add(int value1, int value2)
{
	cout << "arg1: " << value1 << "  arg2: " << value2 << endl;
	return value1 + value2;
}

void compile(X86Compiler& cc) {

	CCFunc* func = cc.newFunc(FuncSignature2<int, int, int>(CallConv::kIdHost));

	X86Gp a = cc.newInt32("a");
	X86Gp b = cc.newInt32("b");

	cc.addFunc(func);
	cc.setArg(0, a);
	cc.setArg(1, b);
	cc.add(a, b);
	cc.ret(a);
	cc.endFunc();
}

std::string to_string2(int value) {

	static const char digits[19] = {
		'9','8','7','6','5','4','3','2','1','0',
		'1','2','3','4','5','6','7','8','9'
	};
	static const char* zero = digits + 9;//zero->'0'

	char buf[24];
	int i = value;
	char *p = buf + 24;
	*--p = '\0' ;
	do {
		int lsd = i % 10;
		i /= 10;
		*--p = zero[lsd];
	} while (i != 0);
	if (value <0)
		*--p = '-';
	return std::string(p);
}



template <typename R,typename ...Args>
class Func {

public:
	R (*pf) (Args...);
};
	

//Õ¹¿ªº¯Êý

/*template <typename ...Args>
Func<Args...> create(){
	return Func<Args...>();
};*/

/*template <typename ...Args>
int create(std::string flags,int n) {

	if (n >= flags.size()) {
		return 0;
	}
	else {
		switch (flags[n])
		{
		case  'I':
			return create<int, Args...>(flags,n + 1);
		case  'D':
			return create<double, Args...>(flags, n + 1);
		case  'l':
			return create<long long, Args...>(flags, n + 1);
		default:
			return create<int, Args...>(flags, n + 1);
		}
	}
};*/

/*
decltype(auto) create(std::string flags) {

	return create(flags,0);
};*/



class AsmSign : public FuncSignature {

public:
	ASMJIT_INLINE AsmSign(std::string flags,uint32_t ccId = CallConv::kIdHost) noexcept {

		uint32_t size = flags.size();
		uint8_t *args = new uint8_t[size - 1];
		for (int i = 1 ;i < size; i++) {
			args[i - 1] = kTypeOf(flags[i]);
		}
		init(ccId, kTypeOf(flags[0]), args, size - 1);
	}

protected:

	TypeId::Id kTypeOf(const char &c) noexcept {
		switch (c)
		{
		case  'I':
			return TypeId::kI32;
		case  'D':
			return TypeId::kI64;
		case  'l':
			return TypeId::kI64;
		default:
			return TypeId::kI32;
		}
	}
};


class AsmFunc {

public :
	string Name;
	string Args;
	vector<string> Expressions;
	
	void Compile(X86Compiler &cc) noexcept {
	
		AsmSign sign(Args, CallConv::kIdHost);
		CCFunc* func2 = cc.newFunc(sign); //cc.newFunc();
		CCFunc* func = cc.newFunc(sign);

		X86Gp a = cc.newInt32("a");
		X86Gp b = cc.newInt32("b");
		Label tramp = cc.newLabel();
		X86Gp tmp = cc.newInt32("tmp");

		cc.addFunc(func);
		cc.setArg(0, a);
		cc.setArg(1, b);
		cc.cmp(a, 0);
		cc.jz(tramp);
		CCFuncCall* call = cc.call(func2->getLabel(), sign);
		call->setArg(0, a);
		call->setArg(1, b);
		call->setRet(0, tmp);
		cc.add(a, tmp);
		cc.bind(tramp);
		cc.add(a, b);
		cc.ret(a);
		cc.endFunc();

	}

protected:

	

};


class AsmField{

public:
	std::string Name;
	std::string Type;

	AsmField(const string& name, const string& type) {
	
	}

protected:
	void setGp(const X86Compiler &cc) noexcept {
	
	}



	X86Gp mValue;
};

class AsmClass {

public :

	std::string Name;

	AsmFunc* GetFunc(const string& name)
	{
		auto itor = mFuncTable.find(name);
		if (itor != mFuncTable.end()) {
			return itor->second;
		}
		return GetSuperFunc(name);
	}

	void SetFunc(const string& name, AsmFunc* func)
	{
		mFuncTable[name] = func;
	}

	AsmFunc* GetSuperFunc(const string& name)
	{
		return nullptr == mSuper ? nullptr : mSuper->GetFunc(name);
	}

	AsmField* GetField(const string& name)
	{
		auto itor = mFieldTable.find(name);
		if (itor != mFieldTable.end()) {
			return itor->second;
		}
		return GetSuperField(name);
	}

	AsmField* GetSuperField(const string& name)
	{
		return nullptr == mSuper ? nullptr : mSuper->GetField(name);
	}

protected:
	AsmClass* mSuper;
	map<string, AsmFunc *> mFuncTable;
	map<string, AsmField *> mFieldTable;
};



class AsmSegment {

public:

	std::string Name;

	AsmFunc* GetFunc(const string& name)
	{
		auto itor = mFuncTable.find(name);
		if (itor != mFuncTable.end()) {
			return itor->second;
		}
		return nullptr;
	}

	AsmField* GetField(const string& name)
	{
		auto itor = mFieldTable.find(name);
		if (itor != mFieldTable.end()) {
			return itor->second;
		}
		return nullptr;
	}

	AsmClass* GetClass(const string& name)
	{
		auto itor = mClassTable.find(name);
		if (itor != mClassTable.end()) {
			return itor->second;
		}
		return nullptr;
	}

	void Attach(AsmGlobal* global)
	{
		global->Add(this);
	}

	void Detach(AsmGlobal* global)
	{
		global->Remove(this);
	}

protected:

	map<string, AsmFunc *> mFuncTable;
	map<string, AsmField *> mFieldTable;
	map<string, AsmClass *> mClassTable;
};



class AsmGlobal {

public:

	std::string Name;

	AsmFunc* GetFunc(const string& name)
	{
		AsmFunc* func;
		for (auto seg : mAsmSegments) {
			func = seg->GetFunc(name);
			if (nullptr != func) {
				return func;
			}
		}
		return nullptr;
	}

	AsmField* GetField(const string& name)
	{
		AsmField* field;
		for (auto seg : mAsmSegments) {
			field = seg->GetField(name);
			if (nullptr != field) {
				return field;
			}
		}
		return nullptr;
	}

	AsmClass* GetClass(const string& name)
	{
		AsmClass* cls;
		for (auto seg : mAsmSegments) {
			cls = seg->GetClass(name);
			if (nullptr != cls) {
				return cls;
			}
		}
		return nullptr;
	}

	void Add(AsmSegment * seg)
	{
		mAsmSegments.push_back(seg);
	}

	void Remove(AsmSegment * seg)
	{
		auto itor = mAsmSegments.begin();
		while (itor != mAsmSegments.end()) {
			if (*itor == seg) {
				mAsmSegments.erase(itor);
				break;
			}
			itor++;
		}
	}

protected:

	vector<AsmSegment *> mAsmSegments;
};

int main(int argc, char* argv[])
{
	JitRuntime runtime;

	CodeHolder code;
	code.init(runtime.getCodeInfo());

	FileLogger logger(stderr);
	code.setLogger(&logger);

	AsmSign sign("III", CallConv::kIdHost);



	X86Compiler cc(&code);
	CCFunc* func2 = cc.newFunc(sign); //cc.newFunc();
	CCFunc* func = cc.newFunc(sign);
	X86Gp pFn = cc.newIntPtr("pFn");

	X86Gp a = cc.newInt32("a");
	X86Gp b = cc.newInt32("b");
	Label tramp = cc.newLabel();
	X86Gp tmp = cc.newInt32("tmp");

	cc.addFunc(func);
	cc.setArg(0, a);
	cc.setArg(1, b);
	cc.cmp(a, 0);
	cc.jz(tramp);
	cc.mov(pFn, func2->getLabel().getOp());

	//CCFuncCall* call = cc.call(pFn, sign);


	CCFuncCall* call = cc.call(func2->getLabel(), sign);
	
	//CCFuncCall* call = cc.call(pFn, sign);
	call->setArg(0, a);
	call->setArg(1, b);
	call->setRet(0, tmp);
	cc.add(a, tmp);
	cc.bind(tramp);
	cc.add(a, b);
	cc.ret(a);
	cc.endFunc();
	Error err = cc.finalize();
	if (err != kErrorOk) {
		return 1;
	}

	Func<int, int, int> aa;// = create("III", 0);


	err = runtime.add(&aa.pf, &code);
	int d = aa.pf(0, 2);
	runtime.release(aa.pf);
	printf("test >> %d \n", d);

	X86Compiler cc2(&code);

	X86Gp a2 = cc2.newInt32("a2");
	X86Gp b2 = cc2.newInt32("b2");
	cc2.addFunc(func2);
	cc2.setArg(0, a2);
	cc2.setArg(1, b2);
	cc2.add(a2, b2);
	cc2.ret(a2);
	cc2.endFunc();
	err = cc2.finalize();
	if (err != kErrorOk) {
		return 1;
	}


	err = runtime.add(&aa.pf, &code);
	d = aa.pf(1, 2);
	runtime.release(aa.pf);
	printf("test >> %d \n", d);
	/*err = runtime.add(&aa.pf, &code2);
	d = aa.pf(1, 2);
	runtime.release(aa.pf);
	printf("test >> %d \n", d);*/

	//printf("test >> %s  %s\n", to_string2(2147483646).c_str(), to_string(int(2147483646)).c_str());
	//printf("test >> %s  %s\n", to_string2(2222222).c_str(), to_string(2222222).c_str());
	//printf("test >> %s  %s\n", to_string2(-7483646).c_str(), to_string(-7483646).c_str());
	//printf("test >> %s  %s\n", to_string2(2147483649).c_str(), to_string(int(2147483649)).c_str());



	system("pause");
	return 0;
}

	
