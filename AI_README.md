# AI Integration System

The MistEngine AI system provides integrated AI assistance for game development tasks using Google Gemini AI.

## Features

- **Gemini AI Integration**: Uses Google's powerful Gemini models
- **Multiple Request Types**: Feature suggestions, code implementation, game logic advice
- **Conversation History**: Maintains context across interactions
- **Free Tier Support**: Generous free limits without billing setup
- **ImGui Interface**: Integrated seamlessly with the engine's UI

## Quick Setup

1. **Get API Key**: Visit [Google AI Studio](https://aistudio.google.com/app/apikey)
2. **Configure**: Use `AI > Configure API Key` menu
3. **Test Connection**: Verify your setup works
4. **Start Chatting**: Press F2 to open the AI assistant

## Configuration

### Setting up Gemini API

1. Go to [Google AI Studio](https://aistudio.google.com/app/apikey)
2. Sign in with your Google account
3. Click "Create API Key"
4. Copy the generated API key
5. In MistEngine: `AI > Configure API Key`
6. Paste your key and click "Test Connection"

### Available Models

- **gemini-1.5-flash**: Fast, efficient model (recommended for most use cases)
- **gemini-1.5-pro**: Most capable model with enhanced reasoning
- **gemini-1.0-pro**: Stable baseline model

### Configuration File

The system saves settings to `ai_config.json`:

```json
{
  "api_keys": {
    "Gemini": "your-api-key-here"
  },
  "defaults": {
    "model": "gemini-1.5-flash",
    "temperature": 0.7,
    "max_tokens": 1000
  }
}
```

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

## Rate Limits & Costs

### Free Tier
- **15 requests per minute**
- **No daily token limits**
- **No billing required**
- **Much more generous than OpenAI**

### Paid Plans
- Higher rate limits available
- Pay-per-use pricing
- Enterprise features

## Architecture Overview

### Core Components

1. **AIProvider**: Abstract interface for AI services
2. **GeminiProvider**: Google Gemini API implementation
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

- **WinINet**: HTTP client implementation (Windows only)
- **ImGui**: User interface components
- **SimpleJson**: Basic JSON parsing

## Troubleshooting

### Common Issues

1. **"Unauthorized - Invalid API key"**
   - Get a new key from [Google AI Studio](https://aistudio.google.com/app/apikey)
   - Ensure you copied the complete key
   - Check that the key is active

2. **"Rate limit exceeded"**
   - Free tier: 15 requests/minute
   - Wait a moment and try again
   - Consider upgrading for higher limits

3. **"Forbidden - Access denied"**
   - Enable the Gemini API in Google Cloud Console
   - Check your account permissions
   - Verify API access

### Build Requirements

- C++14 or later
- Windows platform (for WinINet HTTP client)
- Visual Studio 2017 or later

## Advantages of Gemini

- **Free to start**: No billing setup required
- **Generous limits**: 15 requests/minute on free tier
- **High quality**: Comparable to GPT-4 performance
- **Fast responses**: Low latency from Google's infrastructure
- **Multimodal**: Support for text and images
- **Safety focused**: Built-in content filtering

## Future Enhancements

- Add support for vision models with image input
- Implement conversation persistence
- Add voice input/output capabilities
- Integrate with code editor for context-aware suggestions
- Add model fine-tuning support
