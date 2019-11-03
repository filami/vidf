#pragma once

#include <memory>
#include <vector>
#include <string>


using namespace std;



enum class Type
{
	Unkown,

	Void,
	Char,
	Short,
	Int,
	Long,
	Float,
	Double,
	Signed,
	Unsigned,
};



enum class StorageClass
{
	Typedef,
	Extern,
	Static,
	Auto,
	Register,
};



enum class TypeQualifier
{
	Const,
	Volatile,
};



class KCSemanticNode
{
public:
	virtual void Print(int level) = 0;
	virtual int GetChildCount() const { return 0; }
	virtual KCSemanticNode* GetChild(int idx) const { return nullptr; }
	virtual KCSemanticNode* GetInterface(const type_info& typeInfo) { return nullptr; }
};



class KCNode
{
public:
	KCNode() {}

	KCNode(const char* _name)
		: name(_name)
	{
	}
	KCNode(const char* _name, const char* _text)
		: name(_name)
		, text(_text)
	{
	}
	KCNode(const char* _name, KCNode* child)
		: name(_name)
	{
		children.push_back(child);
	}
	KCNode(const char* _name, KCNode* child0, KCNode* child1)
		: name(_name)
	{
		children.push_back(child0);
		children.push_back(child1);
	}

	~KCNode()
	{
		for (KCNode* child : children)
			delete child;
	}

	string name;
	string text;
	vector<KCNode*> children;
	shared_ptr<KCSemanticNode> semanticNode = nullptr;
};



KCNode* MakeDeclaration(KCNode* specifiers, KCNode* initList);
KCNode* MakeTypeSpecifier(Type type);
KCNode* MakeStorageClass(StorageClass type);
KCNode* MakeTypeQualifier(TypeQualifier type);
KCNode* MakeDeclSpec(KCNode* left, KCNode* right);
