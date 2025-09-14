# NetLogAI Plugins Directory

This directory contains official NetLogAI plugins.

## Plugin Structure

Each plugin should be organized as:
```
plugins/
├── plugin-name/
│   ├── plugin.json          # Plugin manifest
│   ├── plugin-name.dll      # Compiled plugin (Windows)
│   ├── plugin-name.so       # Compiled plugin (Linux)
│   └── README.md            # Plugin documentation
```

## Plugin Manifest (plugin.json)

```json
{
  "name": "plugin-name",
  "version": "1.0.0",
  "description": "Plugin description",
  "author": "Author Name",
  "api_version": "1.0.0",
  "plugin_type": "SECURITY",
  "capabilities": ["LOG_ANALYSIS", "ALERTING"],
  "entry_point": "plugin-name.dll",
  "dependencies": [],
  "metadata": {}
}
```

## Installation

Plugins can be installed using:
```bash
netlogai plugin install path/to/plugin.dll
```

## Development

See the `examples/plugins/` directory for plugin development examples.