# clangd Setup Instructions

## Installation Steps

1. **Install the clangd extension in VS Code**:
   - Open VS Code
   - Go to Extensions (Ctrl+Shift+X or Cmd+Shift+X)
   - Search for "clangd" (by LLVM)
   - Click "Install"

2. **Restart VS Code** after installation to ensure all settings take effect.

3. **When prompted, allow clangd to download the language server** if it's not already installed on your system.

## Configuration Complete

The following configuration has been set up for you:

1. **VS Code Settings**:
   - Microsoft C/C++ IntelliSense has been disabled
   - clangd has been configured with optimal settings
   - Editor inlay hints have been enabled

2. **Project Configuration**:
   - A `.clangd` configuration file has been created with C++23 support and code style settings
   - A symbolic link to `compile_commands.json` has been created in the project root

3. **CMake Integration**:
   - Your existing CMake configuration already generates the necessary `compile_commands.json` file

## Usage Tips

- **Code Completion**: Press Ctrl+Space to trigger code completion
- **Go to Definition**: Ctrl+Click or F12 on a symbol
- **Find References**: Shift+F12 on a symbol
- **Hover Information**: Hover over a symbol to see its type and documentation
- **Inlay Hints**: Parameter names and deduced types will be shown inline
- **Code Actions**: Click on the lightbulb icon or press Ctrl+. to see available code actions

## Troubleshooting

If you encounter any issues:

1. **Check clangd Status**:
   - Look at the clangd status in the VS Code status bar
   - Click on it to see more information

2. **Restart clangd Server**:
   - Press Ctrl+Shift+P or Cmd+Shift+P
   - Type "clangd: Restart language server"
   - Press Enter

3. **Regenerate compile_commands.json**:
   - Run the "cmake: generate compile_commands.json" task from the VS Code task menu

4. **Check Logs**:
   - Press Ctrl+Shift+P or Cmd+Shift+P
   - Type "Output: Focus on Output View"
   - Select "clangd" from the dropdown menu
