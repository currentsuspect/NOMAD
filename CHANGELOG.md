# Changelog

## [Unreleased] - 2025-01-21

### Added
- **Complete Nomad Color System v1.0**
  - 4-tier layered background system (#181819, #1e1e1f, #242428, #2c2c31)
  - Accent colors with hover/pressed states (#8B7FFF, #A79EFF, #665AD9)
  - Status colors (success, warning, error, info)
  - Text hierarchy (primary, secondary, disabled, link, critical)
  - Border system (subtle, active)
  - Interactive element colors (buttons, toggles, inputs, sliders)
  
- **Enhanced Theme System**
  - Extended `NUIThemeProperties` with 40+ color properties
  - Backward compatible with legacy color names
  - Theme manager with comprehensive color getter
  - Documentation: `NOMAD_COLOR_SYSTEM.md`

- **Production-Ready Context Menu**
  - Nomad-themed styling with proper color palette
  - Checkbox and radio button support with visual indicators
  - Separator support with compact 8px height
  - Hover effects with 15% alpha purple highlight
  - Proper text rendering and alignment
  - Keyboard navigation support
  - 28px item height for comfortable interaction
  - 220px width for clean, compact appearance

- **Custom Window Demo Enhancements**
  - Interactive context menu on right-click
  - Color palette showcase with 11 swatches
  - Theme switcher (Nomad Dark/Light)
  - Grid options with checkboxes
  - Full-screen mode support
  - Responsive text centering

- **Documentation**
  - `NOMAD_COLOR_SYSTEM.md` - Complete color palette reference
  - `THEME_DEMO_GUIDE.md` - Demo usage and code examples
  - Color usage guidelines and design principles
  - Migration guide from old theme system

### Changed
- Context menu now uses layered background system
- Improved text rendering with proper vertical centering
- Separators now 8px height (down from 24px)
- Item height increased to 28px for better touch targets
- Border radius increased to 6px for modern look
- Menu width optimized to 220px

### Fixed
- Context menu linker errors (added to CMakeLists.txt)
- Text alignment in menu items
- Checkbox/radio button visual indicators
- Hover state rendering
- Separator positioning

## Roadmap to v1.0 (This Week)

### Tomorrow (2025-01-22)
- [ ] Icon system implementation
- [ ] Grid component
- [ ] Panel component with layered backgrounds

### This Week
- [ ] Slider refinements
- [ ] Input field styling
- [ ] Button states polish
- [ ] Layout system
- [ ] Documentation completion
- [ ] Example applications
- [ ] v1.0 Release

## Notes
- All changes maintain backward compatibility
- Theme system is production-ready
- Context menu is fully functional
- Ready for v1.0 feature completion
