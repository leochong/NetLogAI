# NetLog Parser DSL Reference

## Overview

The NetLog Parser DSL (Domain-Specific Language) is a Lua-based scripting system that allows users to create custom log parsers for network devices and applications not covered by the built-in parsers. Parser scripts have a `.nlp` extension (NetLog Parser).

## Required Functions

Every NetLog Parser script must implement these four functions:

### 1. `can_parse(raw_message)`
Determines if this parser can handle the given log message.

**Parameters:**
- `raw_message` (string): The raw log message to evaluate

**Returns:**
- `boolean`: `true` if parser can handle the message, `false` otherwise

**Example:**
```lua
function can_parse(raw_message)
    return string.find(raw_message, "MYAPP:") ~= nil
end
```

### 2. `parse(raw_message)`
The main parsing function that extracts structured data from the raw message.

**Parameters:**
- `raw_message` (string): The raw log message to parse

**Returns:**
- `table`: Log entry table with parsed fields, or `nil` if parsing failed

**Example:**
```lua
function parse(raw_message)
    local entry = netlog.create_log_entry()
    entry.message = "Extracted message"
    entry.severity = 6  -- Info level
    entry.facility = "MYAPP"
    return entry
end
```

### 3. `get_device_type()`
Returns the device type this parser is designed for.

**Returns:**
- `string`: Device type ("CiscoIOS", "CiscoNXOS", "CiscoASA", "GenericSyslog", "PaloAlto", etc.)

### 4. `get_parser_name()`
Returns the human-readable name of this parser.

**Returns:**
- `string`: Parser name

## Optional Functions

### `get_version()`
Returns the parser version (defaults to "1.0.0").

### `get_supported_patterns()`
Returns an array of regex patterns this parser can handle.

## Log Entry Structure

The log entry table returned by `parse()` can contain these fields:

### Required Fields
- `message` (string): The main log message content

### Optional Fields
- `timestamp` (number): Unix timestamp (use `netlog.parse_timestamp()`)
- `severity` (number): Severity level (0-7, see severity levels below)
- `facility` (string): Log facility name
- `hostname` (string): Source hostname
- `process_name` (string): Process or service name
- `process_id` (number): Process ID
- `metadata` (table): Key-value pairs for additional structured data

### Severity Levels
```lua
0 = Emergency   -- System is unusable
1 = Alert       -- Action must be taken immediately
2 = Critical    -- Critical conditions
3 = Error       -- Error conditions
4 = Warning     -- Warning conditions
5 = Notice      -- Normal but significant condition
6 = Info        -- Informational messages
7 = Debug       -- Debug-level messages
```

## NetLog API Functions

The `netlog` global table provides utility functions:

### `netlog.create_log_entry()`
Creates a new empty log entry table.

### `netlog.parse_timestamp(timestamp_string)`
Parses a timestamp string and returns a Unix timestamp.

### `netlog.parse_severity(severity_string)`
Parses a severity string and returns a numeric severity level.

### `netlog.parse_device_type(device_string)`
Parses a device type string and returns a normalized device type.

### Logging Functions
- `netlog.log_debug(message)`: Log debug message
- `netlog.log_info(message)`: Log info message  
- `netlog.log_warn(message)`: Log warning message
- `netlog.log_error(message)`: Log error message

## Example Parser Template

```lua
-- Parser metadata
local parser_name = "My Custom Parser"
local parser_version = "1.0.0"
local device_type = "GenericSyslog"

-- Check if we can parse this message
function can_parse(raw_message)
    -- Add your detection logic here
    return string.find(raw_message, "MYAPP") ~= nil
end

-- Main parsing function
function parse(raw_message)
    if not can_parse(raw_message) then
        return nil
    end
    
    -- Create log entry
    local entry = netlog.create_log_entry()
    
    -- Extract fields from raw_message
    entry.message = "Parsed message content"
    entry.severity = 6  -- Info
    entry.facility = "MYAPP"
    
    -- Add metadata
    entry.metadata = {
        parser = "my_custom_parser",
        custom_field = "value"
    }
    
    return entry
end

-- Required metadata functions
function get_device_type()
    return device_type
end

function get_parser_name()
    return parser_name
end

function get_version()
    return parser_version
end

function get_supported_patterns()
    return {".*MYAPP.*"}
end
```

## Best Practices

1. **Robust Pattern Matching**: Use multiple methods to identify your log format in `can_parse()`
2. **Error Handling**: Return `nil` from `parse()` if parsing fails
3. **Consistent Metadata**: Include a `parser` field in metadata to identify the source parser
4. **Logging**: Use `netlog.log_*()` functions for debugging during development
5. **Performance**: Keep `can_parse()` lightweight as it's called frequently
6. **Documentation**: Include comments explaining your parsing logic

## Testing Your Parser

1. Save your script as `my_parser.nlp`
2. Use the NetLogAI CLI to test: `nla parser test my_parser.nlp --input sample_logs.txt`
3. Validate syntax: `nla parser validate my_parser.nlp`
4. Install: `nla parser install my_parser.nlp`

## Common Patterns

### CSV Parsing
```lua
local fields = {}
for field in string.gmatch(raw_message, "([^,]+)") do
    table.insert(fields, field)
end
```

### Regex Extraction
```lua
local timestamp, level, message = string.match(raw_message, 
    "(%d+-%d+-%d+ %d+:%d+:%d+) (%w+): (.*)")
```

### Syslog Format
```lua
local month, day, time, hostname, process, message = string.match(raw_message,
    "(%a+) +(%d+) +(%d+:%d+:%d+) +([%w.-]+) +([%w]+)%[?%d*%]?: +(.*)")
```