# Local AI Setup

ProteusManager uses a local Ollama API to generate SQL schemas and database
access layer code. AI features stay disabled until the application can reach an
Ollama endpoint and at least one model is installed.

## Install Ollama

1. Download Ollama from https://ollama.com/download.
2. Install Ollama.
3. Restart ProteusManager after installation.

On Windows, ProteusManager checks the system `PATH`, the current user's local
Ollama install directory, and `C:/Program Files/Ollama/ollama.exe`.

## Start Ollama

Ollama usually starts in the background after installation. If the API is not
reachable, start Ollama manually or run:

```powershell
ollama serve
```

The default endpoint is:

```text
http://localhost:11434
```

You can change the endpoint in ProteusManager under **Project Settings >
Ollama Endpoint**. This prepares the app for future local or tool-based AI
agent workflows.

## Install An AI Model

Install at least one model before using SQL or DAL generation:

```powershell
ollama pull qwen3:8b
```

Other useful options:

```powershell
ollama pull qwen2.5-coder:7b
ollama pull codellama
ollama pull qwen2.5-coder
```

Installed models are loaded automatically in the AI model list.

## Troubleshooting

If ProteusManager shows that Ollama is missing:

- Verify that Ollama is installed.
- Restart ProteusManager.
- Check whether `ollama.exe` is available in your `PATH`.

If ProteusManager shows that the API is unavailable:

- Start Ollama.
- Verify that the endpoint is `http://localhost:11434`.
- Open `http://localhost:11434/api/tags` in a browser or API tool.

If no models are listed:

- Run `ollama list`.
- Install a model with `ollama pull llama3.2`.
- Click **Check** in ProteusManager after installation.

## Validation Behavior

ProteusManager validates the AI environment before generation requests. SQL and
DAL generation remain disabled until:

- the Ollama endpoint is reachable
- at least one model is installed
- a model is selected
