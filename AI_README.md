# MistEngine AI Integration

This implementation adds AI-powered assistance to your MistEngine game engine project. The AI assistant can help with feature suggestions, code implementations, game logic advice, and general development questions.

## Features

- **Ask AI Window**: Interactive chat interface for AI assistance
- **Specialized Request Types**: 
  - Feature Suggestions
  - Code Implementation Help
  - Game Logic Advice
  - Code Optimization
  - Code Explanation
  - Custom Prompts
- **Multiple AI Providers**: Support for OpenAI and Azure OpenAI
- **Configuration Management**: Secure API key storage
- **Hotkey Support**: Press F2 to open AI window

## Setup Instructions

### 1. Install NuGet Dependencies

Add the following package to your project using NuGet Package Manager:

```xml
<package id="nlohmann.json" version="3.11.3" targetFramework="native" />
```

Or via Package Manager Console:
```
Install-Package nlohmann.json
```

### 2. Update Project Files

Add the following files to your Visual Studio project:

**Header Files (include/AI/):**
- `AIProvider.h`
- `HttpClient.h` 
- `OpenAIProvider.h`
- `AIManager.h`
- `AIWindow.h`
- `AIConfig.h`

**Source Files (src/AI/):**
- `HttpClient.cpp`
- `OpenAIProvider.cpp`
- `AIManager.cpp`
- `AIWindow.cpp`
- `AIConfig.cpp`

### 3. Configure API Key

#### Option 1: Configuration Dialog
1. Run the application
2. Go to **AI Menu > Configure API Key**
3. Enter your OpenAI API key
4. Click "Save & Connect"

#### Option 2: Configuration File
1. Create `ai_config.json` in your project root:

```json
{
  "api_keys": {
    "OpenAI": "sk-your-openai-api-key-here"
  },
  "endpoints": {
    "OpenAI": "",
    "Azure": "https://your-resource.openai.azure.com/"
  },
  "defaults": {
    "model": "gpt-3.5-turbo",
    "temperature": 0.7,
    "max_tokens": 1000
  }
}
```

### 4. Getting API Keys

#### OpenAI
1. Go to [OpenAI Platform](https://platform.openai.com/)
2. Sign up or log in
3. Navigate to API Keys section
4. Create a new API key
5. Copy the key (starts with `sk-`)

#### Azure OpenAI
1. Create an Azure OpenAI resource in Azure Portal
2. Get the API key from the resource
3. Get the endpoint URL (e.g., `https://your-resource.openai.azure.com/`)

## Usage

### Opening the AI Assistant
- Press **F2** keyboard shortcut
- Or use **Window > Ask AI** menu
- Or use **AI > Open AI Assistant** menu

### Request Types

1. **General Chat**: Free-form conversation about game development
2. **Feature Suggestion**: Get ideas for new engine features
3. **Code Implementation**: Request specific code implementations
4. **Game Logic Advice**: Get help with game logic patterns
5. **Code Optimization**: Get suggestions for improving performance
6. **Code Explanation**: Understand how code works
7. **Custom**: Use your own system prompt

### Quick Actions
Use the **AI Menu > Quick Actions** for predefined prompts:
- Suggest New Feature
- Code Review Help
- Game Logic Advice

## Architecture Overview

### Core Components

1. **AIProvider**: Abstract interface for AI services
2. **OpenAIProvider**: OpenAI API implementation
3. **AIManager**: Coordinates AI functionality
4. **AIWindow**: ImGui-based user interface
5. **AIConfig**: Configuration management
6. **HttpClient**: HTTP client for API calls

### Design Patterns

- **Strategy Pattern**: AIProvider interface allows different AI services
- **Singleton Pattern**: AIConfig for global configuration
- **PIMPL Pattern**: HttpClient implementation hiding
- **Observer Pattern**: UI updates based on AI responses

## Security Considerations

- API keys are stored locally in `ai_config.json`
- Add `ai_config.json` to `.gitignore`
- Keys are masked in saved configuration files
- Memory buffers are cleared after use

## Dependencies

- **nlohmann/json**: JSON parsing for API communication
- **WinINet**: HTTP client implementation (Windows only)
- **ImGui**: User interface components

## Troubleshooting

### Common Issues

1. **"HTTP client not implemented"**
   - Ensure you're building on Windows
   - WinINet library should be linked automatically

2. **"No AI provider available"**
   - Check your API key configuration
   - Verify internet connectivity
   - Ensure API key is valid

3. **JSON parsing errors**
   - Ensure nlohmann/json package is installed
   - Check that the package is properly referenced

### Build Requirements

- C++14 or later
- Windows platform (for WinINet HTTP client)
- Visual Studio 2017 or later
- NuGet package management

## Future Enhancements

- Add support for other AI providers (Anthropic, Google)
- Implement cross-platform HTTP client
- Add conversation history persistence
- Integrate with code editor for context-aware suggestions
- Add voice input/output capabilities
- Implement AI-assisted debugging

## Example Usage

```cpp
// In your main.cpp, after UI initialization:
uiManager.InitializeAI("your-api-key", "OpenAI");

// The AI window can be accessed via:
// - F2 hotkey
// - AI menu
// - Window menu
```

## API Reference

See the header files for detailed API documentation. Key classes:

- `AIManager`: Main interface for AI functionality
- `AIWindow`: UI component for user interaction
- `AIConfig`: Configuration management
- `AIProvider`: Abstract base for AI services
