/**
 *  yasli - Serialization Library.
 *  Copyright (C) 2007-2013 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */

#include "yasli/Pointers.h"
#include "yasli/Archive.h"
#include "yasli/BinArchive.h"
#include "QPropertyTree.h"
#include "QUIFacade.h"
#include "PropertyTree/IDrawContext.h"
#include "PropertyTree/Serialization.h"
#include "PropertyTree/PropertyTreeModel.h"
#include "PropertyTree/PropertyOArchive.h"
#include "PropertyTree/PropertyIArchive.h"
#include "PropertyTree/PropertyTreeStyle.h"
#include "PropertyTree/Unicode.h"
#include "PropertyTree/PropertyTreeMenuHandler.h"
#include "PropertyTree/MathUtils.h"
#include "PropertyTree/Layout.h"

#include "yasli/ClassFactory.h"


#include <QRect>
#include <QTimer>
#include <QMimeData>
#include <QMenu>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QScrollBar>
#include <QLineEdit>
#include <QPainter>
#include <QElapsedTimer>
#include <QStyle>
#include <QToolTip>
#include <QStylePainter>
#include <QStyleOptionFocusRect>

// only for clipboard:
#include <QClipboard>
#include <QApplication>
#include "PropertyTree/PropertyRowPointer.h"
#include "PropertyTree/PropertyRowContainer.h"
// ^^^
#include "PropertyTree/PropertyRowObject.h"
#include "QDrawContext.h"
using property_tree::toQRect;
using property_tree::toQPoint;


static int translateKey(int qtKey)
{
	switch (qtKey) {
	case Qt::Key_Backspace: return KEY_BACKSPACE;
	case Qt::Key_Delete: return KEY_DELETE;
	case Qt::Key_Down: return KEY_DOWN;
	case Qt::Key_End: return KEY_END;
	case Qt::Key_Escape: return KEY_ESCAPE;
	case Qt::Key_F1: return KEY_F1;
	case Qt::Key_F2: return KEY_F2;
	case Qt::Key_Home: return KEY_HOME;
	case Qt::Key_Insert: return KEY_INSERT;
	case Qt::Key_Left: return KEY_LEFT;
	case Qt::Key_Menu: return KEY_MENU;
	case Qt::Key_Return: return KEY_RETURN;
	case Qt::Key_Enter: return KEY_ENTER;
	case Qt::Key_Right: return KEY_RIGHT;
	case Qt::Key_Space: return KEY_SPACE;
	case Qt::Key_Up: return KEY_UP;
	case Qt::Key_C: return KEY_C;
	case Qt::Key_V: return KEY_V;
	case Qt::Key_Z: return KEY_Z;
	}

	return KEY_UNKNOWN;
}

static int translateModifiers(int qtModifiers)
{
	int result = 0;
	if (qtModifiers & Qt::ControlModifier)
		result |= MODIFIER_CONTROL;
	else if (qtModifiers & Qt::ShiftModifier)
		result |= MODIFIER_SHIFT;
	else if (qtModifiers & Qt::AltModifier)
		result |= MODIFIER_ALT;
	return result;
}

static KeyEvent translateKeyEvent(const QKeyEvent& ev)
{
	KeyEvent result;
	result.key_ = translateKey(ev.key());
	result.modifiers_ = translateModifiers(ev.modifiers());
	return result;
}

QCursor translateCursor(property_tree::Cursor cursor)
{
	switch (cursor)
	{
	case property_tree::CURSOR_ARROW:
		return QCursor(Qt::ArrowCursor);
	case property_tree::CURSOR_BLANK:
		return QCursor(Qt::BlankCursor);
	case property_tree::CURSOR_SLIDE:
		return QCursor(Qt::ArrowCursor); // TODO
	case property_tree::CURSOR_HAND:
		return QCursor(Qt::PointingHandCursor);
	default:
		return QCursor();
	}
}

using yasli::Serializers;

static QMimeData* propertyRowToMimeData(PropertyRow* row, ConstStringList* constStrings)
{
	PropertyRow::setConstStrings(constStrings);
	SharedPtr<PropertyRow> clonedRow(row->clone(constStrings));
	yasli::BinOArchive oa;
	PropertyRow::setConstStrings(constStrings);
	if (!oa(clonedRow, "row", "Row")) {
		PropertyRow::setConstStrings(0);
        return 0;
	}
	PropertyRow::setConstStrings(0);

	QByteArray byteArray(oa.buffer(), oa.length());
	QMimeData* mime = new QMimeData;
	mime->setData("binary/qpropertytree", byteArray);
	return mime;
}

static bool smartPaste(PropertyRow* dest, SharedPtr<PropertyRow>& source, PropertyTreeModel* model, bool onlyCheck)
{
	bool result = false;
	// content of the pulled container has a priority over the node itself
	PropertyRowContainer* destPulledContainer = static_cast<PropertyRowContainer*>(dest->inlinedContainer());
	if((destPulledContainer && strcmp(destPulledContainer->elementTypeName(), source->typeName()) == 0)) {
		PropertyRow* elementRow = model->defaultType(destPulledContainer->elementTypeName());
		YASLI_ESCAPE(elementRow, return false);
		if(strcmp(elementRow->typeName(), source->typeName()) == 0){
			result = true;
			if(!onlyCheck){
				PropertyRow* dest = elementRow;
				if(dest->isPointer() && !source->isPointer()){
					PropertyRowPointer* d = static_cast<PropertyRowPointer*>(dest);
					SharedPtr<PropertyRowPointer> newSourceRoot = static_cast<PropertyRowPointer*>(d->clone(model->constStrings()).get());
					source->swapChildren(newSourceRoot, model);
					source = newSourceRoot;
				}
				destPulledContainer->add(source.get());
			}
		}
	}
	else if((source->isContainer() && dest->isContainer() &&
			 strcmp(static_cast<PropertyRowContainer*>(source.get())->elementTypeName(),
					static_cast<PropertyRowContainer*>(dest)->elementTypeName()) == 0) ||
			(!source->isContainer() && !dest->isContainer() && strcmp(source->typeName(), dest->typeName()) == 0)){
		result = true;
		if(!onlyCheck){
			if(dest->isPointer() && !source->isPointer()){
				PropertyRowPointer* d = static_cast<PropertyRowPointer*>(dest);
				SharedPtr<PropertyRowPointer> newSourceRoot = static_cast<PropertyRowPointer*>(d->clone(model->constStrings()).get());
				source->swapChildren(newSourceRoot, model);
				source = newSourceRoot;
			}
			const char* name = dest->name();
			const char* nameAlt = dest->label();
			source->setName(name);
			source->setLabel(nameAlt);
			if(dest->parent())
				dest->parent()->replaceAndPreserveState(dest, source, model, true);
			else{
				if (dest->isStruct()) {
					dest->asStruct()->clear();
					dest->asStruct()->swapChildren(source, model);
				}
			}
			source->setLabelChanged();
		}
	}
	else if(dest->isContainer()){
		if(model){
			PropertyRowContainer* container = static_cast<PropertyRowContainer*>(dest);
			PropertyRow* elementRow = model->defaultType(container->elementTypeName());
			YASLI_ESCAPE(elementRow, return false);
			if(strcmp(elementRow->typeName(), source->typeName()) == 0){
				result = true;
				if(!onlyCheck){
					PropertyRow* dest = elementRow;
					if(dest->isPointer() && !source->isPointer()){
						PropertyRowPointer* d = static_cast<PropertyRowPointer*>(dest);
						SharedPtr<PropertyRowPointer> newSourceRoot = static_cast<PropertyRowPointer*>(d->clone(model->constStrings()).get());
						source->swapChildren(newSourceRoot, model);
						source = newSourceRoot;
					}

					container->add(source.get());
				}
			}
			container->setLabelChanged();
		}
	}
	
	return result;
}

static bool propertyRowFromMimeData(SharedPtr<PropertyRow>& row, const QMimeData* mimeData, ConstStringList* constStrings)
{
	PropertyRow::setConstStrings(constStrings);
	QStringList formats = mimeData->formats();
	QByteArray array = mimeData->data("binary/qpropertytree");
	if (array.isEmpty())
		return 0;
	yasli::BinIArchive ia;
	if (!ia.open(array.data(), array.size()))
		return 0;

	if (!ia(row, "row", "Row"))
		return false;

	PropertyRow::setConstStrings(0);
	return true;

}

bool propertyRowFromClipboard(SharedPtr<PropertyRow>& row, ConstStringList* constStrings)
{
	const QMimeData* mime = QApplication::clipboard()->mimeData();
	if (!mime)
		return false;
	return propertyRowFromMimeData(row, mime, constStrings);
}



class FilterEntry : public QLineEdit
{
public:
	FilterEntry(QPropertyTree* tree)
    : QLineEdit(tree)
    , tree_(tree)
	{
	}
protected:

    void keyPressEvent(QKeyEvent * ev)
    {
        if (ev->key() == Qt::Key_Escape || ev->key() == Qt::Key_Return || ev->key() == Qt::Key_Enter)
        {
            ev->accept();
            tree_->setFocus();
            tree_->keyPressEvent(ev);
        }

        if (ev->key() == Qt::Key_Backspace && text().isEmpty())
        {
            tree_->setFilterMode(false);
        }
        QLineEdit::keyPressEvent(ev);
    }
private:
	QPropertyTree* tree_;
};


// ---------------------------------------------------------------------------

DragWindow::DragWindow(QPropertyTree* tree)
: tree_(tree)
, offset_(0, 0)
{
	QWidget::setWindowFlags(Qt::ToolTip);
	QWidget::setWindowOpacity(192.0f/ 256.0f);
}

void DragWindow::set(QPropertyTree* tree, PropertyRow* row, const QRect& rowRect)
{
	QRect rect = tree->rect();
	rect.setTopLeft(tree->mapToGlobal(rect.topLeft()));

	offset_ = rect.topLeft();

	row_ = row;
	rect_ = rowRect;
}

void DragWindow::setWindowPos(bool visible)
{
	QWidget::move(rect_.left() + offset_.x() - 3,  rect_.top() + offset_.y() - 3 + tree_->area_.top());
	QWidget::resize(rect_.width() + 5, rect_.height() + 5);
}

void DragWindow::show()
{
	setWindowPos(true);
	QWidget::show();
}

void DragWindow::move(int deltaX, int deltaY)
{
	offset_ += QPoint(deltaX, deltaY);
	setWindowPos(isVisible());
}

void DragWindow::hide()
{
	setWindowPos(false);
	QWidget::hide();
}


void DragWindow::drawRow(QPainter& p)
{
	QRect entireRowRect(0, 0, rect_.width() + 4, rect_.height() + 4);

	p.setBrush(tree_->palette().button());
	p.setPen(QPen(tree_->palette().color(QPalette::WindowText)));
	p.drawRect(entireRowRect);

	QPoint leftTop = toQRect(row_->rect(tree_)).topLeft();
	int offsetX = -leftTop.x() - tree_->config().tabSize + 3;
	int offsetY = -leftTop.y() + 3;
	p.translate(offsetX, offsetY);
	QDrawContext context(tree_, &p);
	tree_->drawRowLayout(context, row_);
	p.translate(-offsetX, -offsetY);
}

void DragWindow::paintEvent(QPaintEvent* ev)
{
	QPainter p(this);

	drawRow(p);
}

// ---------------------------------------------------------------------------

class QPropertyTree::DragController
{
public:
	DragController(QPropertyTree* tree)
	: tree_(tree)
	, captured_(false)
	, dragging_(false)
	, before_(false)
	, row_(0)
	, clickedRow_(0)
	, window_(tree)
	, hoveredRow_(0)
	, destinationRow_(0)
	{
	}

	void beginDrag(PropertyRow* clickedRow, PropertyRow* draggedRow, QPoint pt)
	{
		row_ = draggedRow;
		clickedRow_ = clickedRow;
		startPoint_ = pt;
		lastPoint_ = pt;
		captured_ = true;
		dragging_ = false;
	}

	bool dragOn(QPoint screenPoint)
	{
		if (dragging_)
			window_.move(screenPoint.x() - lastPoint_.x(), screenPoint.y() - lastPoint_.y());

		bool needCapture = false;
		if(!dragging_ && (startPoint_ - screenPoint).manhattanLength() >= 5)
			if(row_->canBeDragged()){
				needCapture = true;
				QRect rect = toQRect(row_->rect(tree_));
				rect = QRect(rect.topLeft() - toQPoint(tree_->offset_) + QPoint(tree_->config().tabSize, 0), 
							 rect.bottomRight() - toQPoint(tree_->offset_));

				window_.set(tree_, row_, rect);
				window_.move(screenPoint.x() - startPoint_.x(), screenPoint.y() - startPoint_.y());
				window_.show();
				tree_->setDraggedRow(row_);
				dragging_ = true;
			}

		if(dragging_){
			QPoint point = tree_->mapFromGlobal(screenPoint);
			trackRow(point);
		}
		lastPoint_ = screenPoint;
		return needCapture;
	}

	void interrupt()
	{
		captured_ = false;
		dragging_ = false;
		row_ = 0;
		window_.hide();
		tree_->setDraggedRow(0);
	}

	void trackRow(QPoint pt)
	{
		hoveredRow_ = 0;
		destinationRow_ = 0;

		QPoint point = pt;
		PropertyRow* row = tree_->rowByPoint(fromQPoint(point));
		if(!row || !row_)
			return;
		row = row->findNonInlinedParent();
		if(!row->parent() || row->isChildOf(row_) || row == row_)
			return;

		int rowHeight = tree_->_defaultRowHeight();
		do {
			Rect rowRect = row->rect(tree_);
			if (row->expanded() && row->hasVisibleChildren(tree_)) {
				Rect childrenRect = row->childrenRect(tree_);
				if (childrenRect.height() > 0)
					rowRect.h = childrenRect.bottom() - rowRect.top();
			}
			float pos = point.y() - rowRect.top();
			bool canBeDroppedNextTo = row_->canBeDroppedOn(row->parent(), row, tree_);
			if(row_->canBeDroppedOn(row, 0, tree_) && (!canBeDroppedNextTo || 
				(pos > rowHeight * 0.25f && pos < rowHeight * 0.75f))){
				hoveredRow_ = destinationRow_ = row;
				return;
			}
			else if(canBeDroppedNextTo){
				if(pos < rowHeight * 0.5f){
					destinationRow_ = row->parent();
					hoveredRow_ = row;
					before_ = true;
					return;
				}
				if(pos >= rowHeight * 0.5f){
					destinationRow_ = row->parent();
					hoveredRow_ = row;
					before_ = false;
					return;
				}
			}
			row = row->parent();
			if (!row->parent())
				break;
		} while (hoveredRow_ == 0);

	}

	void drawUnder(QPainter& painter)
	{
		if(dragging_ && destinationRow_ == hoveredRow_ && hoveredRow_){
			QRect rowRect = toQRect(hoveredRow_->rect(tree_));
			rowRect.setLeft(rowRect.left() + tree_->config().tabSize);
			QBrush brush(true ? tree_->palette().highlight() : tree_->palette().shadow());
			QColor brushColor = brush.color();
			QColor borderColor(brushColor.alpha() / 4, brushColor.red(), brushColor.green(), brushColor.blue());
			fillRoundRectangle(painter, brush, rowRect, borderColor, 6);
		}
	}

	void drawOver(QPainter& painter)
	{
		if(!dragging_)
			return;

		QRect rowRect = toQRect(row_->rect(tree_));

		if(destinationRow_ != hoveredRow_ && hoveredRow_){
			const int tickSize = 4;

			QPoint lineStart;
			QPoint lineEnd;

			if(before_){ 
				QRect hoveredRect = toQRect(hoveredRow_->rect(tree_));
				hoveredRect.setLeft(hoveredRect.left() + tree_->config().tabSize);
				lineStart = QPoint(hoveredRect.left(), hoveredRect.top());
				lineEnd = QPoint(hoveredRect.right(), hoveredRect.top());
			}
			else{
				Rect r = hoveredRow_->rect(tree_);
				if (hoveredRow_->expanded() && hoveredRow_->hasVisibleChildren(tree_)) {
					Rect childrenRect = hoveredRow_->childrenRect(tree_);
					if (childrenRect.height() > 0)
						r.h = childrenRect.bottom() - r.top();
				}
				QRect hoveredRect = toQRect(r);
				hoveredRect.setLeft(hoveredRect.left() + tree_->config().tabSize);
				lineStart = QPoint(hoveredRect.left(), hoveredRect.bottom());
				lineEnd = QPoint(hoveredRect.right(), hoveredRect.bottom());
			}

			QRect rect(lineStart.x() - 1 , lineStart.y() - 1, lineEnd.x() - lineStart.x(), 2);
			QRect rectLeft(lineStart.x() - 1 , lineStart.y() - tickSize, 2, tickSize * 2);
			QRect rectRight(lineEnd.x() - 1 , lineStart.y() - tickSize, 2, tickSize * 2);
			painter.fillRect(rect, tree_->palette().highlight());
			painter.fillRect(rectLeft, tree_->palette().highlight());
			painter.fillRect(rectRight, tree_->palette().highlight());
		}
	}

	bool drop(QPoint screenPoint)
	{
		tree_->setDraggedRow(0);
		bool rowLayoutChanged = false;
		PropertyTreeModel* model = tree_->model();
		if(row_ && hoveredRow_){
			YASLI_ASSERT(destinationRow_);
			clickedRow_->setSelected(false);
			if (destinationRow_->isStruct())
				row_->dropInto(destinationRow_->asStruct(), destinationRow_ == hoveredRow_ ? 0 : hoveredRow_, tree_, before_);
			rowLayoutChanged = true;
		}

		captured_ = false;
		dragging_ = false;
		row_ = 0;
		window_.hide();
		hoveredRow_ = 0;
		destinationRow_ = 0;
		return rowLayoutChanged;
	}

	bool captured() const{ return captured_; }
	bool dragging() const{ return dragging_; }
	PropertyRow* draggedRow() { return row_; }
protected:
	DragWindow window_;
	QPropertyTree* tree_;
	PropertyRow* row_;
	PropertyRow* clickedRow_;
	PropertyRow* hoveredRow_;
	PropertyRow* destinationRow_;
	QPoint startPoint_;
	QPoint lastPoint_;
	bool captured_;
	bool dragging_;
	bool before_;
};

// ---------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable: 4355) //  'this' : used in base member initializer list
QPropertyTree::QPropertyTree(QWidget* parent)
: QWidget(parent)
, PropertyTreeBase(new QUIFacade(this))
, sizeHint_(180, 180)
, dragController_(new DragController(this))
, sizeToContent_(false)
, aggregateMouseEvents_(0)
, aggregatedMouseEventCount_(0)

, updateHeightsTime_(0)
, paintTime_(0)
{
	setFocusPolicy(Qt::WheelFocus);
	setMouseTracking(true); // need to receive mouseMoveEvent to update mouse cursor and tooltip

	scrollBar_ = new QScrollBar(Qt::Vertical, this);
	connect(scrollBar_, SIGNAL(valueChanged(int)), this, SLOT(onScroll(int)));

    filterEntry_.reset(new FilterEntry(this));
    QObject::connect(filterEntry_.data(), SIGNAL(textChanged(const QString&)), this, SLOT(onFilterChanged(const QString&)));
    filterEntry_->hide();

	mouseStillTimer_ = new QTimer(this);
	mouseStillTimer_->setSingleShot(true);
	connect(mouseStillTimer_, SIGNAL(timeout()), this, SLOT(onMouseStillTimer()));

	boldFont_ = font();
	boldFont_.setBold(true);
}
#pragma warning(pop)

QPropertyTree::~QPropertyTree()
{
	clearMenuHandlers();
	delete dragController_;
}

void QPropertyTree::interruptDrag()
{
	dragController_->interrupt();
}

void QPropertyTree::updateHeights()
{
	DebugTimer t(__FUNCTION__, 10);
	{
		QFontMetrics fm(font());
		defaultRowHeight_ = max(16, int(fm.lineSpacing() * 1.5f)); // to fit at least 16x16 icons

		QElapsedTimer timer;
		timer.start();

		QRect widgetRect = rect();

		int scrollBarW = style()->pixelMetric(QStyle::PM_ScrollBarExtent);
		area_ = fromQRect(widgetRect.adjusted(2, 2, -2 - scrollBarW, -2));

		updateScrollBar();

		model()->root()->updateLabel(this, 0, false);
		int lb = style_->compact ? 0 : 4;
		int rb = widgetRect.right() - lb - scrollBarW - 2;
		leftBorder_ = lb;
		rightBorder_ = rb;
		model()->root()->updateTextSize_r(this, 0);

		updateHeightsTime_ = timer.elapsed();

		size_.setX(area_.width());
		size_.setY(0);

		updateHeightsTime_ = timer.elapsed();
	}

	{
		QElapsedTimer timer;
		timer.start();

		updateLayout();
		size_.setY(model()->root()->childrenRect(this).height());
		updateLayoutTime_ = timer.elapsed();
	}

	_arrangeChildren();

	update();
}

bool QPropertyTree::updateScrollBar()
{
	int pageSize = rect().height() - (filterMode_ ? filterEntry_->height() + 2 : 0) - 4;
	offset_.setX(clamp(offset_.x(), 0, max(0, size_.x() - area_.right() - 1)));
	offset_.setY(clamp(offset_.y(), 0, max(0, size_.y() - pageSize)));

	if (pageSize < size_.y())
	{
		scrollBar_->setRange(0, size_.y() - pageSize);
		scrollBar_->setSliderPosition(offset_.y());
		scrollBar_->setPageStep(pageSize);
		scrollBar_->show();
		scrollBar_->move(rect().right() - scrollBar_->width(), 0);
		scrollBar_->resize(scrollBar_->width(), height());
		return true;
	}
	else
	{
		scrollBar_->hide();
		return false;
	}
}

const QFont& QPropertyTree::boldFont() const
{
	return boldFont_;
}

void QPropertyTree::onScroll(int pos)
{
	offset_.setY(scrollBar_->sliderPosition());
	_arrangeChildren();
	repaint();
}

void QPropertyTree::copyRow(PropertyRow* row)
{
	QMimeData* mime = propertyRowToMimeData(row, model()->constStrings());
	if (mime)
		QApplication::clipboard()->setMimeData(mime);
}

void QPropertyTree::pasteRow(PropertyRow* row)
{
	if(!canBePasted(row))
		return;
	PropertyRow* parent = row->parent();

	model()->rowAboutToBeChanged(row);

	SharedPtr<PropertyRow> source;
	if (!propertyRowFromClipboard(source, model()->constStrings()))
		return;

	if (!smartPaste(row, source, model(), false))
		return;
	
	model()->rowChanged(parent ? parent : model()->root());
}

bool QPropertyTree::canBePasted(PropertyRow* destination)
{
	SharedPtr<PropertyRow> source;
	if (!propertyRowFromClipboard(source, model_->constStrings()))
		return false;

	if (!smartPaste(destination, source, model(), true))
		return false;
	return true;
}

bool QPropertyTree::canBePasted(const char* destinationType)
{
	SharedPtr<PropertyRow> source;
	if (!propertyRowFromClipboard(source, model()->constStrings()))
		return false;

	bool result = strcmp(source->typeName(), destinationType) == 0;
	return result;
}

bool QPropertyTree::hasFocusOrInplaceHasFocus() const
{
	if (hasFocus())
		return true;

	QWidget* inplaceWidget = 0;
	if (widget_.get() && widget_->actualWidget())
		inplaceWidget = (QWidget*)widget_->actualWidget();

	if (inplaceWidget) {
		if (inplaceWidget->hasFocus())
			return true;
		if (inplaceWidget->isAncestorOf(QWidget::focusWidget()))
			return true;

	}

	return false;
}

void QPropertyTree::setFilterMode(bool inFilterMode)
{
    bool changed = filterMode_ != inFilterMode;
    filterMode_ = inFilterMode;
    
	if (filterMode_)
	{
        filterEntry_->show();
		filterEntry_->setFocus();
        filterEntry_->selectAll();
	}
    else
        filterEntry_->hide();

    if (changed)
    {
        onFilterChanged(QString());
    }
}

void QPropertyTree::startFilter(const char* filter)
{
	setFilterMode(true);
	filterEntry_->setText(filter);
    onFilterChanged(filter);
}

int QPropertyTree::filterAreaHeight() const
{
	if (!filterMode_)
		return 0;
	return filterEntry_ ? filterEntry_->height() + 4 : 0;
}

void QPropertyTree::_arrangeChildren()
{
	if(widget_.get()){
		PropertyRow* row = widgetRow_;
		if(row->visible(this)){
			QWidget* w = (QWidget*)widget_->actualWidget();
			YASLI_ASSERT(w);
			if(w){
				QRect rect = toQRect(row->widgetRect(this));
				rect = QRect(rect.topLeft() - toQPoint(offset_), 
							 rect.bottomRight() - toQPoint(offset_));
				w->move(rect.topLeft());
				w->resize(rect.size());
				if(!w->isVisible()){
					w->show();
					w->setFocus();
				}
			}
			else{
				//YASLI_ASSERT(w);
			}
		}
		else{
			widget_.reset();
		}
	}

	if (filterEntry_) {
		QSize size = rect().size();
		const int padding = 2;
		QRect pos(padding, padding, size.width() - padding * 2, filterEntry_->height());
		filterEntry_->move(pos.topLeft());
		filterEntry_->resize(pos.size() - QSize(scrollBar_ ? scrollBar_->width() : 0, 0));
	}
}

QPoint QPropertyTree::_toScreen(Point point) const
{
	QPoint pt ( point.x() - offset_.x() + area_.left(), 
				point.y() - offset_.y() + area_.top() );

	return mapToGlobal(pt);
}

void QPropertyTree::attachPropertyTree(PropertyTreeBase* propertyTree) 
{ 
	if(attachedPropertyTree_)
		disconnect((QPropertyTree*)attachedPropertyTree_, SIGNAL(signalChanged()), this, SLOT(onAttachedTreeChanged()));
	PropertyTreeBase::attachPropertyTree(propertyTree);
	connect((QPropertyTree*)attachedPropertyTree_, SIGNAL(signalChanged()), this, SLOT(onAttachedTreeChanged()));
}

struct FilterVisitor
{
	const QPropertyTree::RowFilter& filter_;

	FilterVisitor(const QPropertyTree::RowFilter& filter) 
    : filter_(filter)
    {
    }

	static void markChildrenAsBelonging(PropertyRow* row, bool belongs)
	{
		int count = int(row->count());
		for (int i = 0; i < count; ++i)
		{
			PropertyRow* child = row->childByIndex(i);
			child->setBelongsToFilteredRow(belongs);

			markChildrenAsBelonging(child, belongs);
		}
	}
	
	static bool hasMatchingChildren(PropertyRow* row)
	{
		int numChildren = (int)row->count();
		for (int i = 0; i < numChildren; ++i)
		{
			PropertyRow* child = row->childByIndex(i);
			if (!child)
				continue;
			if (child->matchFilter())
				return true;
			if (hasMatchingChildren(child))
				return true;
		}
		return false;
	}

	ScanResult operator()(PropertyRow* row, PropertyTreeBase* _tree)
	{
		QPropertyTree* tree = (QPropertyTree*)_tree;
		const char* label = row->labelUndecorated();
		yasli::string value = row->valueAsString();

		bool matchFilter = filter_.match(label, filter_.NAME_VALUE, 0, 0) || filter_.match(value.c_str(), filter_.NAME_VALUE, 0, 0);
		if (matchFilter && filter_.typeRelevant(filter_.NAME))
			filter_.match(label, filter_.NAME, 0, 0);
		if (matchFilter && filter_.typeRelevant(filter_.VALUE))
			matchFilter = filter_.match(value.c_str(), filter_.VALUE, 0, 0);
		if (matchFilter && filter_.typeRelevant(filter_.TYPE))
			matchFilter = filter_.match(row->typeNameForFilter(tree).c_str(), filter_.TYPE, 0, 0);						   
		
		int numChildren = int(row->count());
		if (matchFilter) {
			if (row->inlinedBefore() || row->inlined()) {
				// treat pulled rows as part of parent
				PropertyRow* parent = row->parent();
				parent->setMatchFilter(true);
				markChildrenAsBelonging(parent, true);
				parent->setBelongsToFilteredRow(false);
			}
			else {
				markChildrenAsBelonging(row, true);
				row->setBelongsToFilteredRow(false);
				row->setLabelChanged();
			}
		}
		else {
			bool belongs = hasMatchingChildren(row);
			row->setBelongsToFilteredRow(belongs);
			if (belongs) {
				tree->expandRow(row, true, false);
				for (int i = 0; i < numChildren; ++i) {
					PropertyRow* child = row->childByIndex(i);
					if (child->inlined())
						child->setBelongsToFilteredRow(true);
				}
			}
			else {
				row->_setExpanded(false);
			}
		}

		row->setMatchFilter(matchFilter);
		return SCAN_CHILDREN_SIBLINGS;
	}

protected:
	yasli::string labelStart_;
};


void QPropertyTree::onFilterChanged(const QString& text)
{
	QByteArray arr = filterEntry_->text().toLocal8Bit();
	const char* filterStr = filterMode_ ? arr.data() : "";
	rowFilter_.parse(filterStr);
	FilterVisitor visitor(rowFilter_);
	model()->root()->scanChildrenBottomUp(visitor, this);
	updateHeights();
}

void QPropertyTree::drawFilteredString(QPainter& p, const char* text, RowFilter::Type type, const QFont* font, const QRect& rect, const QColor& textColor, bool pathEllipsis, bool center) const
{
	int textLen = (int)strlen(text);

	if (textLen == 0)
		return;

	QFontMetrics fm(*font);
	QString str(text);
	QRect textRect = rect;
	int alignment;
	if (center)
		alignment = Qt::AlignHCenter | Qt::AlignVCenter;
	else {
		if (pathEllipsis && textRect.width() < fm.width(str))
			alignment = Qt::AlignRight | Qt::AlignVCenter;
		else
			alignment = Qt::AlignLeft | Qt::AlignVCenter;
	}

	if (filterMode_) {
		size_t hiStart = 0;
		size_t hiEnd = 0;
		bool matched = rowFilter_.match(text, type, &hiStart, &hiEnd) && hiStart != hiEnd;
		if (!matched && (type == RowFilter::NAME || type == RowFilter::VALUE))
			matched = rowFilter_.match(text, RowFilter::NAME_VALUE, &hiStart, &hiEnd);
		if (matched && hiStart != hiEnd) {
			QRectF boxFull;
			QRectF boxStart;
			QRectF boxEnd;

			boxFull = fm.boundingRect(textRect, alignment, str);

			if (hiStart > 0)
				boxStart = fm.boundingRect(textRect, alignment, str.left(hiStart));
			else {
				boxStart = fm.boundingRect(textRect, alignment, str);
				boxStart.setWidth(0.0f);
			}
			boxEnd = fm.boundingRect(textRect, alignment, str.left(hiEnd));

			QColor highlightColor, highlightBorderColor;
			{
				highlightColor = palette().color(QPalette::Highlight);
				int h, s, v;
				highlightColor.getHsv(&h, &s, &v);
				h -= 175;
				if (h < 0)
					h += 360;
				highlightColor.setHsv(h, min(255, int(s * 1.33f)), v, 255);
				highlightBorderColor.setHsv(h, s * 0.5f, v, 255);
			}

			int left = int(boxFull.left() + boxStart.width()) - 1;
			int top = int(boxFull.top());
			int right = int(boxFull.left() + boxEnd.width());
			int bottom = int(boxFull.top() + boxEnd.height());
			QRect highlightRect(left, top, right - left, bottom - top);
			QBrush br(highlightColor);
			p.setBrush(br);
			p.setPen(highlightBorderColor);
			bool oldAntialiasing = p.renderHints().testFlag(QPainter::Antialiasing);
			p.setRenderHint(QPainter::Antialiasing, true);

			QRect intersectedHighlightRect = rect.intersected(highlightRect);
			p.drawRoundedRect(intersectedHighlightRect, 4.0, 4.0);
			p.setRenderHint(QPainter::Antialiasing, oldAntialiasing);
		}
	}

	QBrush textBrush(textColor);
	p.setBrush(textBrush);
	p.setPen(textColor);
	QFont previousFont = p.font();
	p.setFont(*font);
	p.drawText(textRect, alignment, str, 0);
	p.setFont(previousFont);
}

void QPropertyTree::_drawRowValue(QPainter& p, const char* text, const QFont* font, const QRect& rect, const QColor& textColor, bool pathEllipsis, bool center) const
{
	drawFilteredString(p, text, RowFilter::VALUE, font, rect, textColor, pathEllipsis, center);
}

QSize QPropertyTree::sizeHint() const
{
	if (sizeToContent_)
		return minimumSize();
	else
		return sizeHint_;
}

void QPropertyTree::paintEvent(QPaintEvent* ev)
{
	QElapsedTimer timer;
	timer.start();
	QPainter painter(this);
	QRect clientRect = this->rect().adjusted(0, filterAreaHeight(), 0, 0);

	int clientWidth = clientRect.width();
	int clientHeight = clientRect.height();
	painter.fillRect(clientRect, palette().window());
	painter.setClipRect(clientRect);

	painter.translate(-offset_.x(), -offset_.y());

	if(dragController_->captured())
	 	dragController_->drawUnder(painter);

	painter.translate(offset_.x(), offset_.y());

	
	if (dragController_->captured()) {
	 	painter.translate(-toQPoint(offset_));
	 	dragController_->drawOver(painter);
	 	painter.translate(toQPoint(offset_));
	}
	else{
	// 	if(model()->focusedRow() != 0 && model()->focusedRow()->isRoot() && tree_->hasFocus()){
	// 		clientRect.left += 2; clientRect.top += 2;
	// 		clientRect.right -= 2; clientRect.bottom -= 2;
	// 		DrawFocusRect(dc, &clientRect);
	// 	}
	}

	if (layout_) {
		QDrawContext context(this, &painter);

		painter.translate(-offset_.x(), -offset_.y());

		//int focusedElement = layoutElementByFocusIndex(cursorX_, cursorY_);

		int h = height();
		drawLayout(context, h);

		int num = layout_->rectangles.size();
		if (config_.debugDrawLayout) {
			const int visibleParts[] = { property_tree::PART_WIDGET, property_tree::PART_LABEL };
			for (int i = 0; i < num; ++i) {
				const LayoutElement & element = layout_->elements[i];
				QRect r = toQRect(layout_->rectangles[i]);
				bool visible = false;
				for (int j = 0; j < sizeof(visibleParts)/sizeof(visibleParts[0]); ++j) {
					if (element.rowPart == visibleParts[j]) {
						visible = true;
						break;
					}
				}
				if (!visible)
					continue;

				painter.setPen(QColor(0,0,0, focusedLayoutElement_ ? 255 : 16));
				if (element.focusFlags == NOT_FOCUSABLE ) {
					painter.setPen(Qt::NoPen);
				} else if (element.focusFlags == FORWARDS_FOCUS) {
					painter.setPen(QColor(128,128,128));
				} else {
					painter.setPen(QColor(0,0,0, focusedLayoutElement_ ? 255 : 16));
				}

				switch (element.type) {
				case property_tree::FIXED_SIZE:
					painter.setBrush(QColor(0,128 - 128 / MAX_PRIORITY * element.priority,255,64));
					break;
				case property_tree::EXPANDING:
					painter.setBrush(QColor(0,255,0,64));
					break;
				case property_tree::EXPANDING_MAGNET:
					painter.setBrush(QColor(255,100,255,64));
					break;
				default:
					painter.setBrush(Qt::NoBrush);
				}
				painter.drawRect(r);
			}
		}

		if (size_t(focusedLayoutElement_) < layout_->rectangles.size() && hasFocusOrInplaceHasFocus()) {
			QRect r = toQRect(layout_->rectangles[focusedLayoutElement_]);
			bool focusedSelected = layout_->rows[focusedLayoutElement_] ? layout_->rows[focusedLayoutElement_]->selected() : false;

			QStyleOptionFocusRect option;
			option.initFrom(this);
			option.rect = r.adjusted(-1, -1, 1, 1);
			option.backgroundColor = focusedSelected ? palette().color(QPalette::Highlight) : palette().color(QPalette::Background);
			style()->drawPrimitive(QStyle::PE_FrameFocusRect, &option, &painter, this);
		}

		painter.translate(offset_.x(), offset_.y());
	}

	if (size_.y() > clientHeight)
	{
		const int shadowHeight = 10;
		QColor color1(0, 0, 0, 0);
		QColor color2(0, 0, 0, 96);

		QRect upperRect(clientRect.left(), clientRect.top(), area_.width(), shadowHeight);
		QLinearGradient upperGradient(upperRect.left(), upperRect.top(), upperRect.left(), upperRect.bottom());
		upperGradient.setColorAt(0.0f, color2);
		upperGradient.setColorAt(1.0f, color1);
		QBrush upperBrush(upperGradient);
		painter.fillRect(upperRect, upperBrush);

		QRect lowerRect(clientRect.left(), clientRect.bottom() - shadowHeight / 2, clientRect.width(), shadowHeight / 2 + 1);
		QLinearGradient lowerGradient(lowerRect.left(), lowerRect.top(), lowerRect.left(), lowerRect.bottom());		
		lowerGradient.setColorAt(0.0f, color1);
		lowerGradient.setColorAt(1.0f, color2);
		QBrush lowerBrush(lowerGradient);
		painter.fillRect(lowerRect, lowerGradient);
	}

	paintTime_ = timer.elapsed();
}

QPoint QPropertyTree::pointToRootSpace(const QPoint& point) const
{
	return toQPoint(PropertyTreeBase::pointToRootSpace(fromQPoint(point)));
}

QPoint QPropertyTree::pointFromRootSpace(const QPoint& point) const
{
	return toQPoint(PropertyTreeBase::pointFromRootSpace(fromQPoint(point)));
}

void QPropertyTree::moveEvent(QMoveEvent* ev)
{
	QWidget::moveEvent(ev);
}

void QPropertyTree::resizeEvent(QResizeEvent* ev)
{
	QWidget::resizeEvent(ev);

	updateHeights();
}

void QPropertyTree::mousePressEvent(QMouseEvent* ev)
{
	DebugTimer t("mousePressEvent", 3);
	setFocus(Qt::MouseFocusReason);

	HitResult hit;
	hitTest(&hit, pointToRootSpace(fromQPoint(ev->pos())));
	PropertyRow* row = hit.row;

	if (ev->button() == Qt::LeftButton)
	{
		if(row && !row->isSelectable())
			row = row->parent();
		if(row){
			if(onRowLMBDown(hit,
							ev->modifiers().testFlag(Qt::ControlModifier),
							ev->modifiers().testFlag(Qt::ShiftModifier))){
				capturedRow_ = row;
				lastStillPosition_ = fromQPoint(pointToRootSpace(ev->pos()));
			} else if (!dragCheckMode_){
				row = rowByPoint(fromQPoint(ev->pos()));
				PropertyRow* draggedRow = row;
				while (draggedRow && (!draggedRow->isSelectable() || draggedRow->inlined() || draggedRow->inlinedBefore()))
					draggedRow = draggedRow->parent();
				if(draggedRow && !draggedRow->userReadOnly() && !widget_.get()){
					dragController_->beginDrag(row, draggedRow, ev->globalPos());
				}
			}
		}
		update();
	}
	else if (ev->button() == Qt::RightButton)
	{
		onRowRMBDown(hit);
	}
}

void QPropertyTree::mouseReleaseEvent(QMouseEvent* ev)
{
	DebugTimer t("QPropertyTree::mouseReleaseEvent", 3);
	QWidget::mouseReleaseEvent(ev);

	if (ev->button() == Qt::LeftButton)
	{
		HitResult hit;
		hitTest(&hit, pointToRootSpace(fromQPoint(ev->pos())));

		 if(dragController_->captured()){
		 	if (dragController_->drop(QCursor::pos()))
				updateHeights();
			else
				update();
		}
		 if (dragCheckMode_) {
			 dragCheckMode_ = false;
		 }
		 else {
			 Point point = fromQPoint(ev->pos());
			 PropertyRow* row = rowByPoint(point);
			 if(capturedRow_){
				 Rect rowRect = capturedRow_->rect(this);
				 onRowLMBUp(hit);
				 mouseStillTimer_->stop();
				 capturedRow_ = 0;
				 update();
			 }
		 }
	}
	else if (ev->button() == Qt::RightButton)
	{

	}

	unsetCursor();
}

void QPropertyTree::focusInEvent(QFocusEvent* ev)
{
	QWidget::focusInEvent(ev);
	widget_.reset();
}

void QPropertyTree::defocusInplaceEditor()
{
	if (hasFocusOrInplaceHasFocus())
		setFocus();
}

void QPropertyTree::keyPressEvent(QKeyEvent* ev)
{
	if (ev->key() == Qt::Key_F && ev->modifiers() == Qt::CTRL) {
		setFilterMode(true);
	}

	if (ev->key() == Qt::Key_Up){
		// TODO:
		// int y = model()->root()->verticalIndex(this, focusedRow());
		// if (filterMode_ && y == 0) {
		// 	setFilterMode(true);
		// 	update();
		// 	return;
		// }
	}
	else if (ev->key() == Qt::Key_Down) {
		if (filterMode_ && filterEntry_->hasFocus()) {
			setFocus();
			update();
			return;
		}
	}

	if (filterMode_) {
		if (ev->key() == Qt::Key_Escape && ev->modifiers() == Qt::NoModifier) {
			setFilterMode(false);
		}
	}

	bool result = false;
	if (!widget_.get()) {
		PropertyRow* row = focusedRow();
		KeyEvent keyEvent = translateKeyEvent(*ev);
		onRowKeyDown(row, &keyEvent);
	}
	update();
	if(!result)
		QWidget::keyPressEvent(ev);
}


void QPropertyTree::mouseDoubleClickEvent(QMouseEvent* ev)
{
	QWidget::mouseDoubleClickEvent(ev);

	Point point = fromQPoint(ev->pos());
	PropertyRow* row = rowByPoint(point);
	if(row){
		PropertyActivationEvent e;
		e.tree = this;
		e.rename = true;
		e.reason = e.REASON_DOUBLECLICK;
		PropertyRow* nonPulledParent = row;
		while (nonPulledParent && nonPulledParent->inlined())
			nonPulledParent = nonPulledParent->parent();

		if(row->widgetRect(this).contains(pointToRootSpace(point))){
			if(!row->onActivate(e))
				toggleRow(nonPulledParent);	
		}
		else if(!toggleRow(row)) {
			if (!row->onActivate(e))
				if(!toggleRow(nonPulledParent)) {
					// activate first visible inline row
					for (size_t i = 0; i < row->count(); ++i) {
						PropertyRow* child = row->childByIndex(i);
						if (child && child->inlined() && child->visible(this)) {
							child->onActivate(e);
							break;
						}
					}
				}
		}
	}
}

void QPropertyTree::onMouseStillTimer()
{
	onMouseStill();
}

void QPropertyTree::flushAggregatedMouseEvents()
{
	if (aggregatedMouseEventCount_ > 0) {
		bool gotPendingEvent = aggregatedMouseEventCount_ > 1;
		aggregatedMouseEventCount_ = 0;
		if (gotPendingEvent && lastMouseMoveEvent_.data())
			mouseMoveEvent(lastMouseMoveEvent_.data());
	}
}

void QPropertyTree::mouseMoveEvent(QMouseEvent* ev)
{
	DebugTimer t("QPropertyTree::mouseMoveEvent", 3);
	if (ev->type() == QEvent::MouseMove && aggregateMouseEvents_) {
		lastMouseMoveEvent_.reset(new QMouseEvent(QEvent::MouseMove, ev->localPos(), ev->windowPos(), ev->screenPos(), ev->button(), ev->buttons(), ev->modifiers()));
		ev = lastMouseMoveEvent_.data();
		++aggregatedMouseEventCount_;
		if (aggregatedMouseEventCount_ > 1)
			return;
	}

	QCursor newCursor = QCursor(Qt::ArrowCursor);
	QString newToolTip;
	if(dragController_->captured() && !ev->buttons().testFlag(Qt::LeftButton)) {
		dragController_->interrupt();
	}
	if(dragController_->captured()){
		QPoint pos = QCursor::pos();
		if (dragController_->dragOn(pos)) {
			// SetCapture
		}
		update();
	}
	else{
		Point point = fromQPoint(ev->pos());
		HitResult hit;
		hitTest(&hit, pointToRootSpace(fromQPoint(ev->pos())));
		PropertyRow* row = hit.row;
		if (row && dragCheckMode_ && hit.part == PART_WIDGET) {
			row->onMouseDragCheck(this, dragCheckValue_);
		}
		else if(capturedRow_){
			onRowMouseMove(capturedRow_, point);
			if (config_.sliderUpdateDelay >= 0)
				mouseStillTimer_->start(config_.sliderUpdateDelay);

			/*
			if (cursor().shape() == Qt::BlankCursor)
			{
				pressDelta_ += pointToRootSpace(fromQPoint(ev->pos())) - pressPoint_;
				pointerMovedSincePress_ = true;
				QCursor::setPos(mapToGlobal(toQPoint(PropertyTreeBase::pointFromRootSpace(pressPoint_))));
			}
			else
			*/
			{
				pressDelta_ = pointToRootSpace(fromQPoint(ev->pos())) - pressPoint_;
			}
		}

		PropertyRow* hoverRow = row;
		if (capturedRow_)
			hoverRow = capturedRow_;
		PropertyHoverInfo hover;
		if (hoverRow) {
			Point pointInRootSpace = pointToRootSpace(point);
			if (hoverRow->getHoverInfo(&hover, pointInRootSpace, this)) {
				newCursor = translateCursor(hover.cursor);
				newToolTip = QString::fromUtf8(hover.toolTip.c_str());

				PropertyRow* tooltipRow = hoverRow;
				while (newToolTip.isEmpty() && tooltipRow->parent() && (tooltipRow->inlined() || tooltipRow->inlinedBefore())) {
					// check if parent of inlined property has a tooltip instead
					tooltipRow = tooltipRow->parent();
					if (tooltipRow->getHoverInfo(&hover, pointInRootSpace, this))
						newToolTip = QString::fromUtf8(hover.toolTip.c_str());
				}
			}
			
			if (hit.elementIndex >= 0 && hit.elementIndex < layout_->elements.size()) {
				const LayoutElement & e = layout_->elements[hit.elementIndex];
				if (e.rowPart == PART_VALIDATOR_WARNING_ICON) {
					newCursor = QCursor(Qt::PointingHandCursor);
					newToolTip = "Jump to next warning";
				}
				if (e.rowPart == PART_VALIDATOR_ERROR_ICON) {
					newCursor = QCursor(Qt::PointingHandCursor);
					newToolTip = "Jump to next error";
				}
			}
		}
	}
	setCursor(newCursor);
	if (toolTip() != newToolTip)
		setToolTip(newToolTip);
		if (newToolTip.isEmpty())
			QToolTip::hideText();
}

void QPropertyTree::wheelEvent(QWheelEvent* ev) 
{
	QWidget::wheelEvent(ev);
	
	float delta = ev->angleDelta().ry() / 360.0f;
	if (ev->modifiers() & Qt::CTRL)	{
		if (delta > 0)
			zoomLevel_ += 1;
		else
			zoomLevel_ -= 1;
		if (zoomLevel_ < 8)
			zoomLevel_ = 8;
		if (zoomLevel_ > 30)
			zoomLevel_ = 30;
		float scale = zoomLevel_ * 0.1f;
		QFont font;
		font.setPointSizeF(font.pointSizeF() * scale);		
		setFont(font);
		font.setBold(true);
		boldFont_ = font;

		updateHeights();
	}
	else {
		if (scrollBar_->isVisible() && scrollBar_->isEnabled())
			scrollBar_->setValue(scrollBar_->value() + -ev->delta());
	}
}

void QPropertyTree::onAttachedTreeChanged()
{
	revert();
}

void QPropertyTree::apply(bool continuous)
{
	PropertyTreeBase::apply(continuous);
}

void QPropertyTree::revert()
{
	PropertyTreeBase::revert();
}

void QPropertyTree::_cancelWidget()
{
	QWidget* inplaceWidget = 0;
	if (widget_.get() && widget_->actualWidget())
		inplaceWidget = (QWidget*)widget_->actualWidget();

	bool inplaceWidgetHasFocus = false;

	if (inplaceWidget) {
		if (inplaceWidget->hasFocus())
			inplaceWidgetHasFocus = true;
		else if (inplaceWidget->isAncestorOf(QWidget::focusWidget()))
			inplaceWidgetHasFocus = true;
	}

	if (inplaceWidgetHasFocus)
		setFocus();

	PropertyTreeBase::_cancelWidget();
}

FORCE_SEGMENT(PropertyRowColor)
FORCE_SEGMENT(PropertyRowIconXPM)
FORCE_SEGMENT(PropertyRowFileSave)

// vim:ts=4 sw=4:
