#include <algorithm>
#include <cassert>
#include <iostream>

#include "bwAbstractButton.h"
#include "bwPainter.h"
#include "bwPanel.h"
#include "bwPoint.h"

#include "Layout.h"

using namespace bWidgetsDemo;
using namespace bWidgets; // Less verbose


namespace bWidgetsDemo {
/**
 * \brief Layout-item wrapping a widget to place it within a parent layout.
 */
class WidgetLayoutItem : public LayoutItem
{
public:
	WidgetLayoutItem(bwWidget* widget, LayoutItem* parent = nullptr);

	void draw(bwStyle &style) const override;
	void resolve(
	        const bWidgets::bwPoint& layout_pos,
	        const unsigned int item_margin,
	        const float scale_fac,
	        const LayoutItem& parent) override;
	bwWidget* getWidget() const override;
	bool isHidden() const override;

	void alignWidgetItem(const LayoutItem& parent);
	bool canAlignWidgetItem() const;
	void alignmentSanityCheck(const LayoutItem& parent) const;
	// These could easily share code with needsMarginBeforeNext(). But it works now, so no touching ;).
	bool isAlignedtoPrevious(const LayoutItem& parent) const;
	bool isAlignedtoNext(const LayoutItem& parent) const;

	bwWidget* widget;
};

} // namespace bWidgetsDemo


LayoutItem::LayoutItem(
        LayoutItemType item_type,
        const bool align,
        LayoutItem* parent,
        FlowDirection flow_direction) :
    type(item_type), flow_direction(flow_direction), align(align)
{
	if (parent) {
		assert(parent->hasChild(*this) == false);
		parent->addLayoutItem(this);
	}
}

LayoutItem::~LayoutItem()
{
	for (LayoutItem* item : child_items) {
		if (item->type == LAYOUT_ITEM_TYPE_WIDGET) {
			WidgetLayoutItem* widget_item = static_cast<WidgetLayoutItem*>(item);
			delete widget_item->widget;
		}
		delete item;
	}
}

void LayoutItem::draw(bwStyle& style) const
{
	for (LayoutItem* item : child_items) {
		if (!item->isHidden()) {
			item->draw(style);
		}
	}
}

bool LayoutItem::iterateWidgets(
        bool (*callback)(bwWidget&, void*),
        void* custom_data,
        bool skip_hidden)
{
	for (LayoutItem* item : child_items) {
		bwWidget* widget = item->getWidget();

		if (skip_hidden && item->isHidden()) {
			// skip
		}
		else if (widget && !callback(*widget, custom_data)) {
			return false;
		}
		else if (skip_hidden && areChildrenHidden()) {
			// skip
		}
		else if (!item->iterateWidgets(callback, custom_data, skip_hidden)) {
			return false;
		}
	}

	return true;
}

void LayoutItem::addWidget(bwWidget* widget)
{
	WidgetLayoutItem* widget_item = new WidgetLayoutItem(widget);
	addLayoutItem(widget_item);
}

void LayoutItem::addLayoutItem(LayoutItem* item)
{
	child_items.push_back(item);
	item->iterator_item = std::prev(child_items.end());
}

void LayoutItem::addLayout(RowLayout* row)
{
	addLayoutItem(row);
}

void LayoutItem::addLayout(ColumnLayout* column)
{
	addLayoutItem(column);
}

bool LayoutItem::hasChild(const LayoutItem& potential_child) const
{
	return std::find(child_items.begin(), child_items.end(), &potential_child) != child_items.end();
}

bwWidget* LayoutItem::getWidget() const
{
	return nullptr;
}

bool LayoutItem::isHidden() const
{
	return false;
}

bool LayoutItem::areChildrenHidden() const
{
	return false;
}

unsigned int LayoutItem::getHeight() const
{
	return height;
}

void LayoutItem::resolve(
        const bwPoint& layout_pos,
        const unsigned int item_margin,
        const float scale_fac,
        const LayoutItem& /*parent*/)
{
	int xpos = layout_pos.x;
	int ypos = layout_pos.y;
	unsigned int item_width = 0;
	// Width calculations may have imprecisions so widths don't add up to full layout width. We 'fix' this
	// by adding a pixel to each layout-item until the remainder is 0, meaning total width matches parent
	// width precisely. Also makes layout less jaggy on window size changes.
	int additional_remainder_x = 0;

	height = 0;

	if (flow_direction == FLOW_DIRECTION_HORIZONTAL) {
		const unsigned int margin_count = countNeededMargins();
		const unsigned int width_no_margins = width - (margin_count * item_margin);
		// The max-width of each item is the max-width of the parent, divided by the count of its
		// horizontally placed sub-items. In other words, each item has the same max-width that
		// add up to the parent's max-width. From this max-width, the item margins are subtracted.
		unsigned int tot_row_cols = countRowColumns();

		if (tot_row_cols == 0) {
			width = 0;
		}
		else {
			item_width = width_no_margins / tot_row_cols;
			additional_remainder_x = width_no_margins % tot_row_cols;
		}
		assert(additional_remainder_x >= 0);
	}
	else {
		item_width = width;
	}

	for (LayoutItem* item : child_items) {
		if (item->isHidden()) {
			continue;
		}

		LayoutItem* next = item->getNext(*this, true);

		item->width = item_width;
		// Simple correction for precision issues.
		if (additional_remainder_x > 0) {
			if (next) {
				item->width++;
				additional_remainder_x--;
			}
			else {
				// For the last button in a row, additional_remainder_x may be
				// larger than 1, so add all that is left to it.
//				assert(additional_remainder_x <= 2);
				item->width += additional_remainder_x;
				additional_remainder_x = 0;
			}
		}

		item->resolve(bwPoint(xpos, ypos), item_margin, scale_fac, *this);

		if (flow_direction == FLOW_DIRECTION_VERTICAL) {
			if (item->needsMarginBeforeNext(*this)) {
				ypos -= item_margin;
				height += item_margin;
			}
			else if (next) {
				ypos += 1;
				height -= 1;
			}
			ypos -= item->height;
			height += item->height;
		}
		else if (flow_direction == FLOW_DIRECTION_HORIZONTAL) {
			if (item->needsMarginBeforeNext(*this)) {
				xpos += item_margin;
			}
			else if (next) {
				xpos -= 1;
				additional_remainder_x += 1;
			}
			xpos += item->width;
			height = std::max(height, item->height);
		}
	}
	// xpos should match right side of layout precisely now.
	assert((flow_direction != FLOW_DIRECTION_HORIZONTAL) || (xpos == layout_pos.x + width));
}

bool LayoutItem::needsMarginBeforeNext(const LayoutItem& parent) const
{
	const LayoutItem* next = getNext(parent, true);

	if (!next) {
		return false;
	}
	else if (parent.align && (type == LAYOUT_ITEM_TYPE_WIDGET) && (next->type == LAYOUT_ITEM_TYPE_WIDGET)) {
		const WidgetLayoutItem& widget_item = static_cast<const WidgetLayoutItem&>(*this);
		const WidgetLayoutItem& next_widget_item = static_cast<const WidgetLayoutItem&>(*next);

		if (widget_item.canAlignWidgetItem() && next_widget_item.canAlignWidgetItem()) {
			return false;
		}
	}

	return true;
}

unsigned int LayoutItem::countRowColumns() const
{
	unsigned int count_child_cols = 0;

	assert(type == LAYOUT_ITEM_TYPE_ROW);

	for (LayoutItem* item : child_items) {
		if (item->isHidden()) {
			continue;
		}
		switch (item->type) {
			case LAYOUT_ITEM_TYPE_WIDGET:
			case LAYOUT_ITEM_TYPE_COLUMN:
			case LAYOUT_ITEM_TYPE_PANEL:
				count_child_cols++;
				break;
			case LAYOUT_ITEM_TYPE_ROW:
				count_child_cols += item->countRowColumns();
				break;
			default:
				assert(0);
		}
	}

	return count_child_cols;
}

/**
 * Calculate how many margins need to be added in between child-items of this layout.
 */
unsigned int LayoutItem::countNeededMargins() const
{
	unsigned int tot_margins = 0;

	for (LayoutItem* item : child_items) {
		if (item->isHidden()) {
			continue;
		}
		if (item->needsMarginBeforeNext(*this)) {
			tot_margins++;
		}
	}

	return tot_margins;
}

LayoutItem* LayoutItem::getPrevious(
        const LayoutItem& parent,
        const bool skip_hidden) const
{
	if (iterator_item == parent.child_items.begin()) {
		return nullptr;
	}

	for (IteratorItem iterator_item_prev = std::prev(iterator_item);
	     std::next(iterator_item_prev) != parent.child_items.begin();
	     iterator_item_prev = std::prev(iterator_item_prev))
	{
		if (!skip_hidden || ((*iterator_item_prev)->isHidden() == false)) {
			return *iterator_item_prev;
		}
	}

	return nullptr;
}
LayoutItem* LayoutItem::getNext(
        const LayoutItem& parent,
        const bool skip_hidden) const
{
	for (IteratorItem iterator_item_next = std::next(iterator_item);
	     iterator_item_next != parent.child_items.end();
	     iterator_item_next = std::next(iterator_item_next))
	{
		if (!skip_hidden || ((*iterator_item_next)->isHidden() == false)) {
			return *iterator_item_next;
		}
	}

	return nullptr;
}



/**
 * \param max_size: If \a direction is #FLOW_DIRECTION_VERTICAL, this defines the max-width of the layout.
 */
RootLayout::RootLayout(
//        FlowDirection direction,
        const int ymax,
        const unsigned int max_size,
        const bool align) :
    LayoutItem(LAYOUT_ITEM_TYPE_ROOT, align, nullptr, FLOW_DIRECTION_VERTICAL),
    max_size(max_size), ymax(ymax)
{
	
}

void RootLayout::resolve(
        const float vertical_scroll,
        const float scale_fac)
{
	// Could check if layout actually needs to be updated.

	bwPoint layout_position{(float)padding, ymax - padding - vertical_scroll};

	width = max_size - (padding * 2);
	LayoutItem::resolve(layout_position, item_margin, scale_fac, *this);
}

void RootLayout::setMaxSize(const unsigned int _max_size)
{
	max_size = _max_size;
}

void RootLayout::setYmax(const int value)
{
	ymax = value;
}


ColumnLayout::ColumnLayout(
        const bool align,
        LayoutItem* parent) :
    LayoutItem(LAYOUT_ITEM_TYPE_COLUMN, align, parent, FLOW_DIRECTION_VERTICAL)
{
	
}


RowLayout::RowLayout(
        const bool align,
        LayoutItem* parent) :
    LayoutItem(LAYOUT_ITEM_TYPE_ROW, align, parent, FLOW_DIRECTION_HORIZONTAL)
{
	
}


PanelLayout::PanelLayout(
        const std::string& label,
        unsigned int header_height_hint,
        LayoutItem* parent) :
    LayoutItem(LAYOUT_ITEM_TYPE_PANEL, false, parent, FLOW_DIRECTION_VERTICAL),
    panel(bwPointer_new<bwPanel>(label, header_height_hint))
{
	
}

void PanelLayout::draw(bwStyle &style) const
{
	// Draw panel background
	if (!panel->rectangle.isEmpty()) {
		panel->draw(style);
	}

	if (panel->panel_state == bwPanel::PANEL_OPEN) {
		// Draw children
		LayoutItem::draw(style);
	}
}


void PanelLayout::resolve(
        const bwPoint& layout_pos,
        const unsigned int item_margin,
        const float scale_fac,
        const LayoutItem& /*parent*/)
{
	const float width_panel = width;
	const unsigned int padding = item_margin; // XXX Using item_margin as padding...

	panel->header_height = panel->getHeaderHeightHint() * scale_fac;

	height = 0;
	if (panel->panel_state == bwPanel::PANEL_OPEN) {
		resolveContent(layout_pos, padding, item_margin, scale_fac);
		height += 3 * padding;
	}

	height += panel->header_height;
	panel->rectangle.set(layout_pos.x, width_panel, layout_pos.y - height, height);
}

void PanelLayout::resolveContent(
        const bwPoint& panel_pos,
        const unsigned int padding,
        const unsigned int item_margin,
        const float scale_fac)
{
	const unsigned int initial_width = width;
	bwPoint panel_items_pos = panel_pos;

	panel_items_pos.x += padding;
	panel_items_pos.y -= padding + panel->header_height + padding;

	width -= 2.0f * padding;

	LayoutItem::resolve(panel_items_pos, item_margin, scale_fac, *this);

	width = initial_width;
}

bwWidget* PanelLayout::getWidget() const
{
	return panel.get();
}

bool PanelLayout::areChildrenHidden() const
{
	return (panel->panel_state == bwPanel::PANEL_CLOSED);
}


WidgetLayoutItem::WidgetLayoutItem(
        bwWidget* widget,
        LayoutItem* parent) :
    LayoutItem(LAYOUT_ITEM_TYPE_WIDGET, false, parent), widget(widget)
{
	
}

void WidgetLayoutItem::draw(bwStyle &style) const
{
	assert(widget->hidden == false);
	if (!widget->rectangle.isEmpty()) {
		widget->draw(style);
	}
}

void WidgetLayoutItem::resolve(
        const bwPoint &layout_pos,
        const unsigned int /*item_margin*/,
        const float scale_fac,
        const LayoutItem& parent)
{
	height = widget->height_hint * scale_fac;
	widget->rectangle.set(layout_pos.x, width, layout_pos.y - height, height);
	alignWidgetItem(parent);
}

bwWidget* WidgetLayoutItem::getWidget() const
{
	return widget;
}

bool WidgetLayoutItem::isHidden() const
{
	return widget->hidden;
}

void WidgetLayoutItem::alignmentSanityCheck(const LayoutItem& parent) const
{
	assert(*iterator_item != nullptr);
	assert(parent.hasChild(*this));
	assert(static_cast<const WidgetLayoutItem*>(this)->canAlignWidgetItem());

	(void)parent;
}

bool WidgetLayoutItem::canAlignWidgetItem() const
{
	return (widget->type == bwWidget::WIDGET_TYPE_PUSH_BUTTON) ||
	       (widget->type == bwWidget::WIDGET_TYPE_RADIO_BUTTON) ||
	       (widget->type == bwWidget::WIDGET_TYPE_TEXT_BOX);
}

bool WidgetLayoutItem::isAlignedtoPrevious(const LayoutItem& parent) const
{
	LayoutItem* item_prev;

	alignmentSanityCheck(parent);

	if (!parent.align) {
		return false;
	}
	if (!(item_prev = getPrevious(parent, true)) ||
	    (item_prev->type != LAYOUT_ITEM_TYPE_WIDGET) ||
	    (!(static_cast<WidgetLayoutItem*>(item_prev)->canAlignWidgetItem())))
	{
		return false;
	}

	return true;
}
bool WidgetLayoutItem::isAlignedtoNext(const LayoutItem& parent) const
{
	LayoutItem* item_next;

	alignmentSanityCheck(parent);

	if (!parent.align) {
		return false;
	}
	if (!(item_next = getNext(parent, true)) || (item_next->type != LAYOUT_ITEM_TYPE_WIDGET)) {
		return false;
	}
	else if ((item_next->type == LAYOUT_ITEM_TYPE_WIDGET) &&
	         (!(static_cast<WidgetLayoutItem*>(item_next)->canAlignWidgetItem())))
	{
		return false;
	}

	return true;
}

void WidgetLayoutItem::alignWidgetItem(const LayoutItem& parent)
{
	bwAbstractButton* abstract_button;

	if (!canAlignWidgetItem() || !(abstract_button = widget_cast<bwAbstractButton*>(widget))) {
		return;
	}
	abstract_button->rounded_corners = 0;

	if (!isAlignedtoPrevious(parent)) {
		abstract_button->rounded_corners |= (parent.flow_direction == FLOW_DIRECTION_HORIZONTAL) ?
		                                        (TOP_LEFT | BOTTOM_LEFT) : (TOP_LEFT | TOP_RIGHT);
	}
	if (!isAlignedtoNext(parent)) {
		abstract_button->rounded_corners |= (parent.flow_direction == FLOW_DIRECTION_HORIZONTAL) ?
		                                        (TOP_RIGHT| BOTTOM_RIGHT) : (BOTTOM_LEFT | BOTTOM_RIGHT);
	}
}
