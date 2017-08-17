#pragma once

#include "Style.h"

namespace bWidgets {

class StyleBlenderClassic : public Style
{
public:
	StyleBlenderClassic();

	void setWidgetStyle(const class Widget& widget);

private:
	void setWidgetStyleDefault(const class Widget& widget);
	void setWidgetStyleLabel();
	void setWidgetStyleRadioButton(const class Widget& widget);
};

} // namespace bWidgets
