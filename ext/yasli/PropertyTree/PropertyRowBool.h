/**
 *  yasli - Serialization Library.
 *  Copyright (C) 2007-2013 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */

#pragma once

#include "yasli/Config.h"
#include "PropertyRow.h"
#include "Unicode.h"

class PropertyRowBool : public PropertyRow
{
public:
	PropertyRowBool();
	bool assignToPrimitive(void* val, size_t size) const override;
	bool assignToByPointer(void* instance, const yasli::TypeID& type) const override;
	void setValue(bool value, const void* handle, const yasli::TypeID& typeId) { value_ = value; searchHandle_ = handle; }

	void redraw(IDrawContext& context) override;
	bool isLeaf() const override{ return true; }
	bool isStatic() const override{ return false; }
	bool canBePacked() const override{ return true; }

	bool onActivate(const PropertyActivationEvent& ev) override;
	bool onKeyDown(PropertyTreeBase* tree, const property_tree::KeyEvent* ev) override;
	DragCheckBegin onMouseDragCheckBegin() override;
	bool onMouseDragCheck(PropertyTreeBase* tree, bool value) override;
	yasli::string valueAsString() const override{ return value_ ? "true" : "false"; }
	WidgetPlacement widgetPlacement() const override{ return WIDGET_ICON; }
	void serializeValue(yasli::Archive& ar) override;
	int widgetSizeMin(const PropertyTreeBase* tree) const override;
	const void * searchHandle() const override { return searchHandle_; }
	yasli::TypeID searchType() const override { return yasli::TypeID::get<bool>(); }
protected:
	bool value_;
	const void* searchHandle_;
};

