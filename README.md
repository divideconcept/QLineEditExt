# QLineEditExt
An hybrid slider/text value/progress bar widget for Qt based on QLineEdit.
It requires only one header file inclusion (no additional slots or signals are created).

The purpose of this hybrid widget is to cover all the needs of values input in the most modern/ergonomic way.
- click the value to edit it (supports touch input on Windows, with automatic pop and hide of the virtual keyboard when needed)
- drag the widget to increment/decrement the value by mouse or touch, mouse wheel or up/down keyboard arrows
- multiple scales: linear, power, custom value and text list (strict and non-strict), custom (implement your own increment/decrement)
- strict and non-strict min/max range boundaries
- optional background progress bar reflecting the current value in the defined range
- optional prefix and suffix text (usually description and unit)
- double-click to reset to default value

Every feature is optional and fully customizable, from the simple text edit to a fully autonomous value hybrid slider.

- customizable number of decimals
- customizable progress bar color via the stylesheet
- customizable drag distance (both by mouse and touch)
- support custom text alignment
- support custom contentsMargins

![QLineEditExt Examples](qlineeditext.png)
