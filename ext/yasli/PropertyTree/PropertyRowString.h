/**
 *  yasli - Serialization Library.
 *  Copyright (C) 2007-2013 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */

#pragma once

#include "PropertyRowField.h"
#include "PropertyTreeBase.h"
#include "PropertyTreeModel.h"
#include "Unicode.h"

class PropertyRowString : public PropertyRowField
{
public:
	PropertyRowString() : searchHandle_() {}
	bool isLeaf() const override{ return true; }
	bool isStatic() const override{ return false; }
	bool assignTo(yasli::string& str) const;
	bool assignTo(yasli::wstring& str) const;
	bool assignToByPointer(void* instance, const yasli::TypeID& type) const override;
	void setValue(const char* str, const void* handle, const yasli::TypeID& typeId);
	void setValue(const wchar_t* str, const void* handle, const yasli::TypeID& typeId);
	property_tree::InplaceWidget* createWidget(PropertyTreeBase* tree) override;
	yasli::string valueAsString() const override;
	WidgetPlacement widgetPlacement() const override{ return WIDGET_VALUE; }
	void serializeValue(yasli::Archive& ar) override;
	const yasli::string& value() const{ return value_; }
	const void* searchHandle() const override { return searchHandle_; }
	yasli::TypeID searchType() const override { return searchType_; }
private:
	yasli::string value_;
	yasli::TypeID searchType_;
	const void * searchHandle_;
};

