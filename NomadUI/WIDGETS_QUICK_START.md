# NomadUI Widgets - Quick Start Guide 🚀

## 📦 What's Included

**6 Production-Ready Widgets:**
- 🔘 **Button** - Interactive buttons with callbacks
- 📝 **Label** - Text display with styling
- 🎚️ **Slider** - Value selection controls
- ☑️ **Checkbox** - Toggle switches
- ⌨️ **TextInput** - Text entry fields
- 📦 **Panel** - Container layouts

**Plus:**
- ✨ Complete demo application
- 🎨 Theme integration
- 🖱️ Event handling
- 📊 Text rendering
- 🎬 Smooth animations

---

## ⚡ 30-Second Setup

### 1. Build the Demo
```bash
cmake -B build -S . -DNOMADUI_BUILD_EXAMPLES=ON
cmake --build build
./build/bin/NomadUI_WidgetsDemo.exe
```

### 2. See It In Action
The demo showcases:
- All 6 widgets working together
- Interactive examples
- Text rendering samples
- Real-time FPS counter

---

## 💡 Basic Usage

### Create a Button
```cpp
auto btn = std::make_shared<NUIButton>("Click Me!");
btn->setBounds(x, y, 150, 40);
btn->setOnClick([]() { std::cout << "Clicked!\n"; });
btn->setTheme(theme);
parent->addChild(btn);
```

### Create a Slider
```cpp
auto slider = std::make_shared<NUISlider>(0.0f, 100.0f, 50.0f);
slider->setBounds(x, y, 200, 30);
slider->setOnValueChange([](float val) { 
    std::cout << "Value: " << val << "\n"; 
});
slider->setTheme(theme);
parent->addChild(slider);
```

### Create a Text Input
```cpp
auto input = std::make_shared<NUITextInput>("Enter text...");
input->setBounds(x, y, 300, 40);
input->setOnTextChange([](const std::string& text) { 
    std::cout << "Text: " << text << "\n"; 
});
input->setTheme(theme);
parent->addChild(input);
```

### Create a Panel with Widgets
```cpp
auto panel = std::make_shared<NUIPanel>("Settings");
panel->setBounds(50, 50, 400, 300);
panel->setTitleBarEnabled(true);
panel->setTheme(theme);

auto content = panel->getContentBounds();

// Add widgets inside panel...
auto label = std::make_shared<NUILabel>("Volume:");
label->setBounds(content.x, content.y, 200, 30);
label->setTheme(theme);
panel->addChild(label);

auto volumeSlider = std::make_shared<NUISlider>(0, 100, 75);
volumeSlider->setBounds(content.x, content.y + 40, content.width - 20, 30);
volumeSlider->setTheme(theme);
panel->addChild(volumeSlider);
```

---

## 🎨 Widget Cheat Sheet

### NUIButton
```cpp
button->setText("Text");
button->setOnClick(callback);
button->setGlowEnabled(true);
button->setFontSize(16.0f);
```

### NUILabel
```cpp
label->setText("Text");
label->setTextAlign(NUILabel::TextAlign::Center);
label->setFontSize(18.0f);
label->setShadowEnabled(true);
label->setTextColor(NUIColor::fromHex(0xFF0000));
```

### NUISlider
```cpp
slider->setValue(50.0f);
slider->setRange(0.0f, 100.0f);
slider->setOnValueChange(callback);
slider->setThumbRadius(10.0f);
```

### NUICheckbox
```cpp
checkbox->setChecked(true);
checkbox->setLabel("Enable");
checkbox->setOnChange(callback);
checkbox->setBoxSize(24.0f);
```

### NUITextInput
```cpp
textInput->setText("Hello");
textInput->setPlaceholder("Type here...");
textInput->setPasswordMode(true);
textInput->setOnTextChange(callback);
textInput->setOnSubmit(callback);
```

### NUIPanel
```cpp
panel->setTitle("Title");
panel->setTitleBarEnabled(true);
panel->setPadding(15.0f);
panel->setBorderEnabled(true);
panel->setShadowEnabled(true);
auto content = panel->getContentBounds(); // For positioning children
```

---

## 🎯 Common Patterns

### Form Layout
```cpp
auto form = std::make_shared<NUIPanel>("User Form");
form->setBounds(50, 50, 400, 300);
form->setTheme(theme);

auto content = form->getContentBounds();
float y = content.y;

// Name field
auto nameLabel = std::make_shared<NUILabel>("Name:");
nameLabel->setBounds(content.x, y, 100, 30);
nameLabel->setTheme(theme);
form->addChild(nameLabel);

auto nameInput = std::make_shared<NUITextInput>("Enter name...");
nameInput->setBounds(content.x + 110, y, 250, 30);
nameInput->setTheme(theme);
form->addChild(nameInput);
y += 45;

// Age slider
auto ageLabel = std::make_shared<NUILabel>("Age:");
ageLabel->setBounds(content.x, y, 100, 30);
ageLabel->setTheme(theme);
form->addChild(ageLabel);

auto ageSlider = std::make_shared<NUISlider>(18, 100, 25);
ageSlider->setBounds(content.x + 110, y, 250, 30);
ageSlider->setTheme(theme);
form->addChild(ageSlider);
y += 45;

// Submit button
auto submitBtn = std::make_shared<NUIButton>("Submit");
submitBtn->setBounds(content.x, y, 100, 40);
submitBtn->setOnClick([nameInput, ageSlider]() {
    std::cout << "Name: " << nameInput->getText() << "\n";
    std::cout << "Age: " << ageSlider->getValue() << "\n";
});
submitBtn->setTheme(theme);
form->addChild(submitBtn);
```

### Settings Panel
```cpp
auto settings = std::make_shared<NUIPanel>("Settings");
settings->setBounds(0, 0, 300, 400);
settings->setTheme(theme);

auto content = settings->getContentBounds();
float y = content.y;

auto fullscreenCheck = std::make_shared<NUICheckbox>("Fullscreen", false);
fullscreenCheck->setBounds(content.x, y, 250, 30);
fullscreenCheck->setTheme(theme);
settings->addChild(fullscreenCheck);
y += 40;

auto vsyncCheck = std::make_shared<NUICheckbox>("VSync", true);
vsyncCheck->setBounds(content.x, y, 250, 30);
vsyncCheck->setTheme(theme);
settings->addChild(vsyncCheck);
y += 50;

auto volumeLabel = std::make_shared<NUILabel>("Volume:");
volumeLabel->setBounds(content.x, y, 250, 25);
volumeLabel->setTheme(theme);
settings->addChild(volumeLabel);
y += 30;

auto volumeSlider = std::make_shared<NUISlider>(0, 100, 75);
volumeSlider->setBounds(content.x, y, 250, 30);
volumeSlider->setTheme(theme);
settings->addChild(volumeSlider);
```

---

## 🎨 Theme Customization

### Using Default Theme
```cpp
auto theme = NUITheme::createDefault();
widget->setTheme(theme);
```

### Custom Colors
```cpp
// Override widget colors
button->setBackgroundColor(NUIColor::fromHex(0x00FF00));
label->setTextColor(NUIColor::fromHex(0xFF0000));
slider->setFillColor(NUIColor::fromHex(0x0000FF));

// Reset to theme colors
widget->resetColors();
```

### Theme Colors Available
- `theme->getPrimary()` - Accent color
- `theme->getSurface()` - Widget backgrounds
- `theme->getBackground()` - App background
- `theme->getText()` - Primary text
- `theme->getTextSecondary()` - Secondary text
- `theme->getBorder()` - Border colors
- `theme->getHover()` - Hover states
- `theme->getActive()` - Active states

---

## 🔥 Pro Tips

### 1. **Always Set Theme**
```cpp
// Do this for every widget!
widget->setTheme(theme);
```

### 2. **Use Content Bounds for Panels**
```cpp
auto panel = std::make_shared<NUIPanel>("Title");
panel->setPadding(20.0f);

// Use content bounds to position children correctly
auto content = panel->getContentBounds();
child->setBounds(content.x, content.y, width, height);
```

### 3. **Lambda Captures for Callbacks**
```cpp
button->setOnClick([input, slider]() {
    std::cout << input->getText() << ": " << slider->getValue() << "\n";
});
```

### 4. **Shared Pointers for Widgets**
```cpp
auto widget = std::make_shared<NUIButton>("Button");
parent->addChild(widget); // Safe reference management
```

### 5. **Consistent Sizing**
```cpp
const float WIDGET_HEIGHT = 40.0f;
const float SPACING = 15.0f;

button->setBounds(x, y, 150, WIDGET_HEIGHT);
y += WIDGET_HEIGHT + SPACING;
input->setBounds(x, y, 200, WIDGET_HEIGHT);
```

---

## 📁 File Structure

```
NomadUI/
├── Widgets/
│   ├── NUIButton.h/cpp       # Button widget
│   ├── NUILabel.h/cpp        # Label widget
│   ├── NUISlider.h/cpp       # Slider widget
│   ├── NUICheckbox.h/cpp     # Checkbox widget
│   ├── NUITextInput.h/cpp    # Text input widget
│   ├── NUIPanel.h/cpp        # Panel widget
│   └── README.md             # Full documentation
│
├── Examples/
│   ├── SimpleDemo.cpp        # Basic demo
│   └── WidgetsDemo.cpp       # Full widget showcase
│
├── docs/
│   └── WIDGETS_COMPLETE.md   # Implementation details
│
└── CMakeLists.txt            # Build configuration
```

---

## 🏗️ Building

### Windows
```bash
cmake -B build -S . -DNOMADUI_BUILD_EXAMPLES=ON
cmake --build build
./build/bin/NomadUI_WidgetsDemo.exe
```

### Build Targets
- `NomadUI_Core` - Core library with widgets
- `NomadUI_WindowDemo` - Basic demo
- `NomadUI_WidgetsDemo` - **Complete widget showcase** ⭐

---

## 📚 Learn More

- **Full Documentation:** `Widgets/README.md`
- **Implementation Guide:** `docs/WIDGETS_COMPLETE.md`
- **Live Examples:** `Examples/WidgetsDemo.cpp`
- **Widget Headers:** Each widget has detailed API docs in its `.h` file

---

## 🎉 You're Ready!

Start building amazing UIs with NomadUI widgets!

```cpp
// Your first NomadUI application
auto app = std::make_shared<NUIApp>();
app->initialize(1280, 720, "My App");

auto theme = NUITheme::createDefault();

auto root = std::make_shared<NUIPanel>("My First App");
root->setBounds(0, 0, 1280, 720);
root->setTheme(theme);

auto button = std::make_shared<NUIButton>("Hello, NomadUI!");
button->setBounds(100, 100, 200, 50);
button->setOnClick([]() { 
    std::cout << "Welcome to NomadUI! 🎉\n"; 
});
button->setTheme(theme);
root->addChild(button);

app->setRootComponent(root);
app->run();
```

**Happy coding! 🚀**
