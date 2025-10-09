# NomadUI Widgets

A comprehensive collection of modern, GPU-accelerated UI widgets for the Nomad UI framework.

## Available Widgets

### 🔘 NUIButton
Interactive button widget with hover and pressed states.

**Features:**
- Text labels
- Hover/pressed animations
- Click callbacks
- Customizable colors
- Optional glow effect
- Border control

**Usage:**
```cpp
auto button = std::make_shared<NUIButton>("Click Me!");
button->setBounds(x, y, width, height);
button->setOnClick([]() {
    std::cout << "Button clicked!" << std::endl;
});
button->setTheme(theme);
```

---

### 📝 NUILabel
Simple text display widget with alignment options.

**Features:**
- Multiple text alignment options
- Customizable font size
- Word wrapping (optional)
- Shadow effects
- Custom text colors

**Usage:**
```cpp
auto label = std::make_shared<NUILabel>("Hello, World!");
label->setBounds(x, y, width, height);
label->setTextAlign(NUILabel::TextAlign::Center);
label->setFontSize(18.0f);
label->setTheme(theme);
```

---

### 🎚️ NUISlider
Horizontal slider for value selection.

**Features:**
- Smooth dragging interaction
- Configurable min/max range
- Value change callbacks
- Animated hover effects
- Customizable colors
- Thumb size control

**Usage:**
```cpp
auto slider = std::make_shared<NUISlider>(0.0f, 100.0f, 50.0f);
slider->setBounds(x, y, width, height);
slider->setOnValueChange([](float value) {
    std::cout << "Value: " << value << std::endl;
});
slider->setTheme(theme);
```

---

### ☑️ NUICheckbox
Toggle checkbox with optional label.

**Features:**
- Checked/unchecked states
- Smooth check animation
- Optional label text
- Change callbacks
- Hover effects
- Customizable box size

**Usage:**
```cpp
auto checkbox = std::make_shared<NUICheckbox>("Enable feature", false);
checkbox->setBounds(x, y, width, height);
checkbox->setOnChange([](bool checked) {
    std::cout << "Checked: " << checked << std::endl;
});
checkbox->setTheme(theme);
```

---

### ⌨️ NUITextInput
Text input field with keyboard support.

**Features:**
- Text entry and editing
- Blinking cursor animation
- Placeholder text
- Password mode
- Submit callback (Enter key)
- Text change callbacks
- Focus indicators

**Usage:**
```cpp
auto textInput = std::make_shared<NUITextInput>("Enter text...");
textInput->setBounds(x, y, width, height);
textInput->setOnTextChange([](const std::string& text) {
    std::cout << "Text: " << text << std::endl;
});
textInput->setTheme(theme);
```

---

### 📦 NUIPanel
Container panel for organizing widgets.

**Features:**
- Optional title bar
- Customizable padding
- Border and shadow effects
- Background customization
- Content area management
- Child widget containment

**Usage:**
```cpp
auto panel = std::make_shared<NUIPanel>("My Panel");
panel->setBounds(x, y, width, height);
panel->setTitleBarEnabled(true);
panel->setPadding(15.0f);
panel->setTheme(theme);

// Add widgets to panel
auto contentBounds = panel->getContentBounds();
// Position children within contentBounds...
panel->addChild(childWidget);
```

---

## Widget Demos

### SimpleDemo
Basic demonstration of component rendering and mouse events.

**Features:**
- Basic panel rendering
- Mouse interaction
- FPS counter
- Glow effects

**Build & Run:**
```bash
cmake --build build --target NomadUI_WindowDemo
./build/bin/NomadUI_WindowDemo
```

---

### WidgetsDemo
**Comprehensive showcase of all widgets and text rendering capabilities.**

**Features:**
- All 6 widgets demonstrated
- Interactive examples
- Real-time value updates
- Text rendering samples
- Multiple color schemes
- Layout examples

**Build & Run:**
```bash
cmake --build build --target NomadUI_WidgetsDemo
./build/bin/NomadUI_WidgetsDemo
```

**What you'll see:**
- **Interactive Widgets Panel:**
  - Button with click counter
  - Slider with live value display
  - Checkbox with state indicator
  - Text input with echo display

- **Text & Display Panel:**
  - Various font sizes (small, normal, large, title)
  - Different text alignments
  - Shadow effects
  - Custom colors (red, green, blue)

- **Info Panel:**
  - Framework description
  - Feature highlights

- **FPS Counter:**
  - Real-time performance monitoring

---

## Common Widget Features

All widgets inherit from `NUIComponent` and share these features:

### Theme Integration
```cpp
widget->setTheme(theme);
```

### Bounds & Layout
```cpp
widget->setBounds(x, y, width, height);
widget->setPosition(x, y);
widget->setSize(width, height);
```

### State Management
```cpp
widget->setVisible(true);
widget->setEnabled(true);
widget->setOpacity(0.8f);
```

### Event Handling
All widgets support mouse events through the `onMouseEvent()` override.

### Customization
Most widgets support:
- Custom colors (with theme fallback)
- Size adjustments
- Visual effects (glow, shadow, borders)

---

## Building Widgets

To build the widget library and demos:

```bash
# Configure CMake
cmake -B build -S . -DNOMADUI_BUILD_EXAMPLES=ON

# Build
cmake --build build

# Run demos
./build/bin/NomadUI_WidgetsDemo  # Comprehensive demo
./build/bin/NomadUI_WindowDemo   # Basic demo
```

---

## Creating Custom Widgets

To create your own widget:

1. **Inherit from NUIComponent:**
```cpp
class MyWidget : public NUIComponent {
public:
    MyWidget();
    ~MyWidget() override = default;
    
    void onRender(NUIRenderer& renderer) override;
    void onUpdate(double deltaTime) override;
    bool onMouseEvent(const NUIMouseEvent& event) override;
};
```

2. **Implement rendering:**
```cpp
void MyWidget::onRender(NUIRenderer& renderer) {
    auto theme = getTheme();
    auto bounds = getBounds();
    
    // Draw your widget
    renderer.fillRoundedRect(bounds, radius, color);
    
    // Render children
    NUIComponent::onRender(renderer);
}
```

3. **Handle events:**
```cpp
bool MyWidget::onMouseEvent(const NUIMouseEvent& event) {
    if (containsPoint(event.position)) {
        // Handle event
        return true; // Event consumed
    }
    return false;
}
```

---

## Architecture

### Widget Hierarchy
```
NUIComponent (base)
├── NUIButton
├── NUILabel
├── NUISlider
├── NUICheckbox
├── NUITextInput
└── NUIPanel
```

### Rendering Pipeline
1. Update phase (`onUpdate()`)
2. Event processing (`onMouseEvent()`, `onKeyEvent()`)
3. Render phase (`onRender()`)
4. Children rendering (automatic)

### Theme System
Widgets automatically inherit theme settings:
- Colors (primary, surface, text, etc.)
- Dimensions (border radius, padding, etc.)
- Effects (glow intensity, shadows, etc.)
- Font sizes (small, normal, large, title)

---

## Performance

All widgets are optimized for:
- **GPU acceleration** - All rendering uses OpenGL
- **Minimal state updates** - Dirty flag system
- **Smooth animations** - Frame-rate independent
- **Low memory usage** - Shared theme resources

---

## License

Part of the Nomad UI framework. See LICENSE for details.

---

## Contributing

To add new widgets:
1. Create header in `Widgets/NUIWidgetName.h`
2. Create implementation in `Widgets/NUIWidgetName.cpp`
3. Add to `CMakeLists.txt` in `NOMADUI_WIDGET_SOURCES`
4. Add demo example in `WidgetsDemo.cpp`
5. Update this README

---

**Happy widget building! 🎨**
