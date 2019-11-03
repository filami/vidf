#include <iostream>
#include "kcparser.h"
#include "kc.h"


void PrintKCNode(KCNode* node, int level)
{
	if (node->semanticNode)
	{
		node->semanticNode->Print(level);
	}
	else
	{
		for (int i = 0; i < level; ++i)
			cout << "  ";
		cout << "[";
		cout << node->name;
		if (!node->text.empty())
			cout << " : " << node->text;
		cout << "]";
		cout << endl;
	}
	for (KCNode* child : node->children)
		PrintKCNode(child, level + 1);
}



const char* ToString(Type type)
{
	switch (type)
	{
	case Type::Void:     return "void";
	case Type::Char:     return "char";
	case Type::Short:    return "short";
	case Type::Int:      return "int";
	case Type::Long:     return "long";
	case Type::Float:    return "float";
	case Type::Double:   return "double";
	case Type::Signed:   return "signed";
	case Type::Unsigned: return "unsigned";
	default:
		__debugbreak();
		return "__";
	}
}



template<typename T>
shared_ptr<T> CastNode(KCNode* node)
{
	if (!node || !node->semanticNode)
		return nullptr;
	if (node->semanticNode->GetInterface(typeid(T)) == nullptr)
		return nullptr;
	return static_pointer_cast<T>(node->semanticNode);
}



template<typename T>
shared_ptr<T> CastOrCreateNode(KCNode* node)
{
	if (!node || !node->semanticNode)
		return make_shared<T>();
	if (node->semanticNode->GetInterface(typeid(T)) == nullptr)
		return make_shared<T>();
	return static_pointer_cast<T>(node->semanticNode);
}



class KCTypeSpecifier : public KCSemanticNode
{
public:
	KCTypeSpecifier(Type _type)
		: type(_type) {}

	virtual void Print(int level) override
	{
		for (int i = 0; i < level; ++i)
			cout << "  ";
		cout << "KCTypeSpecifier" << endl;
	}

	virtual KCSemanticNode* GetInterface(const type_info& typeInfo) override
	{
		if (typeInfo == typeid(KCTypeSpecifier))
			return this;
		else
			KCSemanticNode::GetInterface(typeInfo);
	}

public:
	Type type;
};



class KCStorageClass : public KCSemanticNode
{
public:
	KCStorageClass(StorageClass _storageClass)
		: storageClass(_storageClass) {}

	virtual void Print(int level) override
	{
		for (int i = 0; i < level; ++i)
			cout << "  ";
		cout << "KCStorageClass" << endl;
	}

	virtual KCSemanticNode* GetInterface(const type_info& typeInfo) override
	{
		if (typeInfo == typeid(KCStorageClass))
			return this;
		else
			KCSemanticNode::GetInterface(typeInfo);
	}

public:
	StorageClass storageClass;
};



class KCTypeQualifier : public KCSemanticNode
{
public:
	KCTypeQualifier(TypeQualifier _typeQualifier)
		: typeQualifier(_typeQualifier) {}

	virtual void Print(int level) override
	{
		for (int i = 0; i < level; ++i)
			cout << "  ";
		cout << "KCTypeQualifier" << endl;
	}

	virtual KCSemanticNode* GetInterface(const type_info& typeInfo) override
	{
		if (typeInfo == typeid(KCStorageClass))
			return this;
		else
			KCSemanticNode::GetInterface(typeInfo);
	}

public:
	TypeQualifier typeQualifier;
};



class KCDeclSpec : public KCSemanticNode
{
public:
	virtual void Print(int level) override
	{
		for (int i = 0; i < level; ++i)
			cout << "  ";
		cout << "KCDeclSpec : ";
		cout << ToString(type);
		if (isStatic)
			cout << " | static";
		if (isConst)
			cout << " | const";
		if (isVolatile)
			cout << " | volatile";
		cout << endl;
	}

	void Apply(KCNode* node)
	{
		if (auto typeNode = CastNode<KCTypeSpecifier>(node))
		{
			// TODO : error : setting type twice
			type = typeNode->type;
		}
		else if (auto typeNode = CastNode<KCStorageClass>(node))
		{
			switch (typeNode->storageClass)
			{
			case StorageClass::Typedef:
				break;
			case StorageClass::Extern:
				break;
			case StorageClass::Static:
				isStatic = true;
				break;
			case StorageClass::Auto:
				break;
			case StorageClass::Register:
				break;
			}
		}
		else if (auto typeNode = CastNode<KCTypeQualifier>(node))
		{
			switch (typeNode->typeQualifier)
			{
			case TypeQualifier::Const:
				isConst = true;
				break;
			case TypeQualifier::Volatile:
				isVolatile = true;
				break;
			}
		}
	}

	virtual KCSemanticNode* GetInterface(const type_info& typeInfo) override
	{
		if (typeInfo == typeid(KCDeclSpec))
			return this;
		else
			KCSemanticNode::GetInterface(typeInfo);
	}

public:
	Type type = Type::Unkown;
	bool isStatic = false;
	bool isConst = false;
	bool isVolatile = false;
};



class KCDeclaration : public KCSemanticNode
{
public:
	KCDeclaration(shared_ptr<KCDeclSpec> _declSpec)
		: declSpec(_declSpec) {}

	virtual void Print(int level) override
	{
		for (int i = 0; i < level; ++i)
			cout << "  ";
		cout << "KCDeclaration" << endl;
		declSpec->Print(level + 1);
	}

	virtual int GetChildCount() const { return 1; }

	virtual KCSemanticNode* GetChild(int idx) const { return declSpec.get(); }

	virtual KCSemanticNode* GetInterface(const type_info& typeInfo) override
	{
		if (typeInfo == typeid(KCDeclaration))
			return this;
		else
			KCSemanticNode::GetInterface(typeInfo);
	}

public:
	shared_ptr<KCDeclSpec> declSpec;
};




KCNode* MakeDeclaration(KCNode* _declSpec, KCNode* initList)
{
	auto declSpec = CastNode<KCDeclSpec>(_declSpec);
	auto declaration = make_shared<KCDeclaration>(declSpec);

	delete _declSpec;
	delete initList;
	KCNode* node = new KCNode("__");
	node->semanticNode = declaration;
	return node;
}



KCNode* MakeTypeSpecifier(Type type)
{
	auto typeSpec = make_shared<KCTypeSpecifier>(type);

	KCNode* node = new KCNode("__");
	node->semanticNode = typeSpec;
	return node;
}



KCNode* MakeStorageClass(StorageClass type)
{
	auto typeSpec = make_shared<KCStorageClass>(type);

	KCNode* node = new KCNode("__");
	node->semanticNode = typeSpec;
	return node;
}



KCNode* MakeTypeQualifier(TypeQualifier type)
{
	auto typeSpec = make_shared<KCTypeQualifier>(type);

	KCNode* node = new KCNode("__");
	node->semanticNode = typeSpec;
	return node;
}



KCNode* MakeDeclSpec(KCNode* left, KCNode* right)
{
	auto declSpec = CastOrCreateNode<KCDeclSpec>(right);
	declSpec->Apply(left);

	delete left;
	delete right;
	KCNode* node = new KCNode("__");
	node->semanticNode = declSpec;
	return node;
}



void KC()
{
	FILE* in = fopen("data/main.kc", "r");
	parse Parser(in);
	Parser.yyparse();
	PrintKCNode(Parser.root, 0);
}
