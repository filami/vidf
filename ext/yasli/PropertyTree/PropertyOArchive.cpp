/**
 *  yasli - Serialization Library.
 *  Copyright (C) 2007-2013 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */

#include <math.h>
#include <memory>

#include "yasli/STL.h"
#include "ValidatorBlock.h"
#include "PropertyOArchive.h"
#include "PropertyTreeModel.h"
#include "PropertyTreeBase.h"

#include "PropertyRowContainer.h"
#include "PropertyRowBool.h"
#include "PropertyRowString.h"
#include "PropertyRowNumber.h"
#include "PropertyRowPointer.h"
#include "PropertyRowObject.h"
#include "ConstStringList.h"

#include "yasli/Archive.h"
#include "yasli/BitVector.h"
#include "yasli/ClassFactory.h"
#include "yasli/StringList.h"
#include "yasli/Callback.h"
using yasli::TypeID;

PropertyOArchive::PropertyOArchive(PropertyTreeModel* model, PropertyRowStruct* root, ValidatorBlock* validator)
: Archive(OUTPUT | EDIT | VALIDATION | DOCUMENTATION)
, model_(model)
, currentNode_(root)
, lastNode_(0)
, updateMode_(false)
, defaultValueCreationMode_(false)
, rootNode_(root)
, outlineMode_(false)
, validator_(validator)
{
	stack_.push_back(Level());
	YASLI_ASSERT(model != 0);
	if(rootNode_->count() != 0){
		updateMode_ = true;
	}
}

PropertyOArchive::PropertyOArchive(PropertyTreeModel* model, bool forDefaultType)
: Archive(OUTPUT | EDIT | VALIDATION | DOCUMENTATION)
, model_(model)
, currentNode_(0)
, lastNode_(0)
, updateMode_(false)
, defaultValueCreationMode_(forDefaultType)
, rootNode_(0)
, outlineMode_(false)
, validator_(0)
{
	rootNode_ = new PropertyRowStruct();
	rootNode_->setName("root");
	currentNode_ = rootNode_.get();
	stack_.push_back(Level());
}

PropertyOArchive::~PropertyOArchive()
{
	YASLI_ASSERT(!updateMode_, "Call finalize() for update mode");
}

void PropertyOArchive::finalize()
{
	rootNode_->eraseOldRecursive();
	updateMode_ = false;
}

PropertyRow* PropertyOArchive::defaultValueRootNode()
{
	if (!rootNode_)
		return 0;
	return rootNode_->childByIndex(0);
}

void PropertyOArchive::enterNode(PropertyRowStruct* row, bool isBlock)
{
	if (!row->isStruct()) {
		YASLI_ASSERT(0);
		return;
	}
	currentNode_ = row;
	stack_.push_back(Level(isBlock && currentNode_ ? currentNode_->countUpdated() : 0));
}

void PropertyOArchive::closeStruct(const char* name, bool isBlock)
{
	stack_.pop_back();

	if(currentNode_){
		if(!isBlock)
			currentNode_->eraseOld();
		lastNode_ = currentNode_;
		currentNode_ = currentNode_->parent();
	}
}

template<class RowType, class ValueType>
RowType* PropertyOArchive::updateRow(const char* name, const char* label, const char* typeName, const ValueType& value, bool isBlock)
{
	SharedPtr<RowType> newRow;
	if(currentNode_ == 0){
		if (rootNode_)
			newRow = static_cast<RowType*>(rootNode_.get());
		else		
			newRow.reset(new RowType());
		newRow->setNames(name, label, typeName);
		newRow->updated_ = true;
		if(updateMode_){
			if (!newRow->isStruct() ) {
				// fail
				return NULL;
			}
			model_->setRoot(newRow->asStruct());
			return newRow;
		}
		else{
			if(defaultValueCreationMode_)
				rootNode_ = newRow;
			else
				model_->setRoot(newRow);
			newRow->setValueAndContext(value, *this);
			return newRow;
		}
	}
	else{
		Level& level = stack_.back();
		int rowIndex;
		PropertyRow* oldRow = currentNode_->findFromIndex(&rowIndex, name, typeName, level.rowIndex, !isBlock);
		const char* oldLabel;
		if(oldRow){
			oldRow->setMultiValue(false);
			newRow = static_cast<RowType*>(oldRow);
			level.rowIndex = rowIndex + 1;
			oldLabel = oldRow->label();
			newRow->setNames(name, label, typeName);
		}
		else{
			PropertyRowFactory& factory = PropertyRowFactory::the();
			newRow = static_cast<RowType*>(factory.create(typeName));
			if(!newRow)
				newRow.reset(new RowType());
			newRow->setNames(name, label, typeName);
			if(model_->expandLevels() != 0 && (model_->expandLevels() == -1 || model_->expandLevels() >= currentNode_->level()))
				newRow->_setExpanded(true);

			currentNode_->insertAfterUpdated(newRow, level.realIndex);
		}

		if (!oldRow || oldLabel != label) {
			// for new rows we should mark all parents with labelChanged_
			newRow->setLabelChanged();
			newRow->setLabelChangedToChildren();
		}
		newRow->setValueAndContext(value, *this);
		++level.realIndex;
		newRow->updated_ = true;
		return newRow;
	}
}

template<class RowType, class ValueType>
PropertyRow* PropertyOArchive::updateRowPrimitive(const char* name, const char* label, const char* typeName, const ValueType& value, const void* handle, const yasli::TypeID& typeId)
{
	SharedPtr<RowType> newRow;

	if(currentNode_ == 0){
		if (rootNode_) {
			newRow = static_cast<RowType*>(static_cast<PropertyRow*>(rootNode_.get()));
		} else		
			newRow.reset(new RowType());
		newRow->setNames(name, label, typeName);
		newRow->updated_ = true;
		if(updateMode_){
			model_->setRoot(newRow->asStruct());
			return newRow;
		}
		else{
			if(defaultValueCreationMode_) {
				if (!newRow->isStruct()) {
					return NULL;
				}
				rootNode_ = newRow->asStruct();
			}
			else
				model_->setRoot(newRow->asStruct());
			return newRow;
		}
	}

	int rowIndex;
	Level& level = stack_.back();
	PropertyRow* oldRow = currentNode_->findFromIndex(&rowIndex, name, typeName, level.rowIndex, true);
	if(oldRow){
		oldRow->setMultiValue(false);
		newRow.reset(static_cast<RowType*>(oldRow));
		level.rowIndex = rowIndex + 1;
	}
	else{
		//printf("creating new row '%s' '%s' '%s'\n", name, label, typeName);
		newRow = new RowType();
		newRow->setNames(name, label, typeName);
		if(model_->expandLevels() != 0){
			if(model_->expandLevels() == -1 || model_->expandLevels() >= currentNode_->level())
				newRow->_setExpanded(true);
		}
		currentNode_->insertAfterUpdated(newRow, level.realIndex);
		// for new rows we should mark all parents with labelChanged_
		newRow->setLabelChanged();
	}

	newRow->setValue(value, handle, typeId);
	newRow->updated_ = true;
	++level.realIndex;
	return newRow;
}

bool PropertyOArchive::operator()(const yasli::Serializer& ser, const char* name, const char* label)
{
    const char* typeName = ser.type().name();
    size_t size = ser.size();

	lastNode_ = currentNode_;
	bool hideChildren = outlineMode_ && currentNode_ && currentNode_->isContainer();
	PropertyRowStruct* row = updateRow<PropertyRowStruct>(name, label, typeName, ser);
	row->setHideChildren(hideChildren);

	PropertyRow* nonLeaf = 0;
	if(!row->isLeaf() || currentNode_ == 0){
		enterNode(row);

		if(currentNode_->isLeaf())
			return false;
		else
			nonLeaf = currentNode_;
	}
	else{
		lastNode_ = row;
		return true;
	}

	if (ser)
		ser(*this);

	if (nonLeaf)
		nonLeaf->closeNonLeaf(ser, *this);

	closeStruct(name);
	return true;
}

yasli::string escape(const char* str)
{
	yasli::string val;
	val.reserve(strlen(str));
	for(const char* p = str; *p; ++p){
		if(*p == '\n'){
			val.push_back('\\');
			val.push_back('n');
		}		
		else
			val.push_back(*p);
	}
	return val;
}

bool PropertyOArchive::operator()(yasli::StringInterface& value, const char* name, const char* label)
{
	lastNode_ = updateRowPrimitive<PropertyRowString>(name, label, "string", escape(value.get()).c_str(), value.handle(), value.type());
	return true;
}

bool PropertyOArchive::operator()(yasli::WStringInterface& value, const char* name, const char* label)
{
	lastNode_ = updateRowPrimitive<PropertyRowString>(name, label, "string", value.get(), value.handle(), value.type());
	return true;
}

bool PropertyOArchive::operator()(bool& value, const char* name, const char* label)
{
	lastNode_ = updateRowPrimitive<PropertyRowBool>(name, label, "bool", value, &value, yasli::TypeID::get<bool>());
	return true;
}

bool PropertyOArchive::operator()(char& value, const char* name, const char* label)
{
	lastNode_ = updateRowPrimitive<PropertyRowNumber<char> >(name, label, "char", value, &value, yasli::TypeID::get<char>());
	return true;
}

// ---

bool PropertyOArchive::operator()(yasli::i8& value, const char* name, const char* label)
{
	lastNode_ = updateRowPrimitive<PropertyRowNumber<yasli::i8> >(name, label, "int8", value, &value, yasli::TypeID::get<yasli::i8>());
	return true;
}

bool PropertyOArchive::operator()(yasli::i16& value, const char* name, const char* label)
{
	lastNode_ = updateRowPrimitive<PropertyRowNumber<yasli::i16> >(name, label, "int16", value, &value, yasli::TypeID::get<yasli::i16>());
	return true;
}

bool PropertyOArchive::operator()(yasli::i32& value, const char* name, const char* label)
{
	lastNode_ = updateRowPrimitive<PropertyRowNumber<yasli::i32> >(name, label, "int32", value, &value, yasli::TypeID::get<yasli::i32>());
	return true;
}

bool PropertyOArchive::operator()(yasli::i64& value, const char* name, const char* label)
{
	lastNode_ = updateRowPrimitive<PropertyRowNumber<yasli::i64> >(name, label, "int64", value, &value, yasli::TypeID::get<yasli::i64>());
	return true;
}

// ---

bool PropertyOArchive::operator()(yasli::u8& value, const char* name, const char* label)
{
	lastNode_ = updateRowPrimitive<PropertyRowNumber<yasli::u8> >(name, label, "uint8", value, &value, yasli::TypeID::get<yasli::u8>());
	return true;
}

bool PropertyOArchive::operator()(yasli::u16& value, const char* name, const char* label)
{
	lastNode_ = updateRowPrimitive<PropertyRowNumber<yasli::u16> >(name, label, "uint16", value, &value, yasli::TypeID::get<yasli::u16>());
	return true;
}

bool PropertyOArchive::operator()(yasli::u32& value, const char* name, const char* label)
{
	lastNode_ = updateRowPrimitive<PropertyRowNumber<yasli::u32> >(name, label, "uint32", value, &value, yasli::TypeID::get<yasli::u32>());
	return true;
}

bool PropertyOArchive::operator()(yasli::u64& value, const char* name, const char* label)
{
	lastNode_ = updateRowPrimitive<PropertyRowNumber<yasli::u64> >(name, label, "uint64", value, &value, yasli::TypeID::get<yasli::u64>());
	return true;
}

// ---

bool PropertyOArchive::operator()(float& value, const char* name, const char* label)
{
	lastNode_ = updateRowPrimitive<PropertyRowNumber<float> >(name, label, "float", value, &value, yasli::TypeID::get<float>());
	return true;
}

bool PropertyOArchive::operator()(double& value, const char* name, const char* label)
{
	lastNode_ = updateRowPrimitive<PropertyRowNumber<double> >(name, label, "double", value, &value, yasli::TypeID::get<float>());
	return true;
}


bool PropertyOArchive::operator()(yasli::ContainerInterface& ser, const char *name, const char *label)
{
	const char* elementTypeName = ser.elementType().name();
	bool fixedSizeContainer = ser.isFixedSize();
	lastNode_ = currentNode_;
	enterNode(updateRow<PropertyRowContainer>(name, label, ser.containerType().name(), ser));

	if (!model_->defaultTypeRegistered(elementTypeName)) {
		PropertyOArchive ar(model_, true);
		ar.setOutlineMode(outlineMode_);
		ar.setFilter(getFilter());
		ar.setLastContext(lastContext());
		model_->addDefaultType(0, elementTypeName); // add empty default to prevent recursion
		ser.serializeNewElement(ar, "", "<");
		if (ar.defaultValueRootNode() != 0)
			model_->addDefaultType(ar.defaultValueRootNode(), elementTypeName);
	}
	if ( ser.size() > 0 )
		while( true ) {
			ser(*this, "", "xxx");
			if ( !ser.next() )
				break;
		}
	currentNode_->eraseOld();
	currentNode_->labelChanged();
	closeStruct(name);
	return true;
}

bool PropertyOArchive::operator()(yasli::PointerInterface& ptr, const char *name, const char *label)
{
	lastNode_ = currentNode_;

	bool hideChildren = outlineMode_ && currentNode_ && currentNode_->isContainer();
	PropertyRowPointer* row = updateRow<PropertyRowPointer>(name, label, ptr.baseType().name(), ptr);
	row->setHideChildren(hideChildren);
	enterNode(row);
	{
		TypeID baseType = ptr.baseType();
		yasli::ClassFactoryBase* factory = ptr.factory();
		size_t count = factory->size();

		const char* nullLabel = factory->nullLabel();
		if (!(nullLabel && nullLabel[0] == '\0'))
		{
			PropertyDefaultDerivedTypeValue nullValue;
			nullValue.factory = factory;
			nullValue.factoryIndex = -1;
			nullValue.label = nullLabel ? nullLabel : "[ null ]";
			model_->addDefaultType(baseType, nullValue);
		}

		for(size_t i = 0; i < count; ++i) {
			const yasli::TypeDescription *desc = factory->descriptionByIndex((int)i);
			if (!model_->defaultTypeRegistered(baseType, desc->name())){
				PropertyOArchive ar(model_, true);
				ar.setOutlineMode(outlineMode_);
				ar.setLastContext(lastContext());
				ar.setFilter(getFilter());

				PropertyDefaultDerivedTypeValue defaultValue;
				defaultValue.registeredName = desc->name();
				defaultValue.factory = factory;
				defaultValue.factoryIndex = int(i);
				defaultValue.label = desc->label();

				model_->addDefaultType(baseType, defaultValue);
				factory->serializeNewByIndex(ar, (int)i, "name", "label");
				if (ar.defaultValueRootNode() != 0) {
					ar.defaultValueRootNode()->setTypeName(desc->name());
					defaultValue.root = ar.defaultValueRootNode();
					model_->addDefaultType(baseType, defaultValue);
				}
			}
		}
	}

	if(Serializer ser = ptr.serializer())
		ser(*this);
	closeStruct(name);
	return true;
}

bool PropertyOArchive::operator()(yasli::CallbackInterface& callback, const char* name, const char* label)
{
	if (!callback.serializeValue(*this, name, label))
		return false;

	lastNode_->setCallback(callback.clone());
	return true;
}

bool PropertyOArchive::operator()(yasli::Object& obj, const char *name, const char *label)
{
	const char* typeName = obj.type().name();

	PropertyRowObject* row = 0;
	if (typeName_.empty())
		 row = updateRow<PropertyRowObject>(name, label, obj.type().name(), obj);
	else
		 row = updateRow<PropertyRowObject>(name, label, obj.type().name(), obj);
	lastNode_ = row;
	return true;
}

bool PropertyOArchive::openBlock(const char* name, const char* label)
{
	PropertyRowStruct* row = updateRow<PropertyRowStruct>(name, label, "block", Serializer(), true);
	lastNode_ = currentNode_;
	enterNode(row, true);
	return true;
}

void PropertyOArchive::validatorMessage(bool error, const void* handle, const yasli::TypeID& type, const char* message)
{
	if (validator_) {
		ValidatorEntry entry(error ? VALIDATOR_ENTRY_ERROR : VALIDATOR_ENTRY_WARNING,
							 handle,
							 type,
							 message);
		validator_->addEntry(entry);
	}
}

void PropertyOArchive::documentLastField(const char* message)
{
	if (lastNode_ && (!currentNode_ || lastNode_->parent() == currentNode_))
		lastNode_->setTooltip(message ? message : "");
	else if (currentNode_)
		currentNode_->setTooltip(message ? message : "");
}

void PropertyOArchive::closeBlock()
{
	closeStruct("block", true);
}

void PropertyOArchive::setOutlineMode(bool outlineMode)
{
	outlineMode_ = outlineMode;
}

// vim:ts=4 sw=4:
