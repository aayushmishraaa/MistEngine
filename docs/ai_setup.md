# AI Assistant Setup

MistEngine's AI assistant (editor panel) can talk to Google Gemini or OpenAI.
Keys live in `ai_config.json` at the repo root. That file is gitignored and
must never be committed.

## Configure Gemini (recommended — free tier available)

1. Visit <https://aistudio.google.com/app/apikey> and create an API key.
2. Copy `ai_config.example.json` to `ai_config.json`.
3. Replace the placeholder key value under the `gemini` block.

```json
{
  "providers": {
    "gemini": {
      "enabled": true,
      "apiKey": "AIza…",
      "endpoint": ""
    }
  },
  "activeProvider": "gemini"
}
```

## Configure OpenAI

Same flow but under the `openai` block; use a key from
<https://platform.openai.com/api-keys>.

## How keys are handled

- The key is read once during `AIManager::InitializeProvider` and held in
  memory. It never touches disk beyond `ai_config.json`.
- For Gemini, the key is sent as an `x-goog-api-key` HTTP header. It is *not*
  embedded in the URL. Earlier builds (<= 0.4.1) appended `?key=<KEY>` to the
  URL, which meant every request was logged to stdout with the key visible —
  if you used a real key on that version, rotate it.
- `HttpClient` strips the query string before logging any URL, so future
  additions that put credentials in the URL won't leak either.
- The Authorization header and all other headers are never logged.

## Safe source for keys

Do not hardcode keys in source files, CMakeLists, or CI configuration.
Production deployments should read the key from an environment variable and
inject it at runtime rather than relying on `ai_config.json`.
