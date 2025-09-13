# NetLog Parser DSL Specification

## Overview

The NetLog Parser Domain-Specific Language (DSL) is a Lua-based scripting system designed for creating custom network log parsers. It provides a comprehensive API for parsing, validating, and extracting structured data from various network device log formats.

## Core Concepts

### Parser Script Structure

Every NetLog Parser script (`.nlp` file) must implement the following functions:

```lua
-- Required Functions
function parse(raw_message)       -- Parse a log message and return structured data
function can_parse(raw_message)   -- Check if this parser can handle the message
function get_device_type()        -- Return the device type this parser handles
function get_parser_name()        -- Return a unique parser name

-- Optional Functions
function get_version()            -- Return parser version (default: "1.0.0")
function get_supported_patterns() -- Return array of regex patterns this parser recognizes
function initialize()            -- Called once when parser is loaded
function cleanup()               -- Called when parser is unloaded
```

### Log Entry Structure

The `parse()` function must return a table with the following structure, or `nil` if parsing fails:

```lua
{
    timestamp = 1640995200,           -- Unix timestamp (number) or ISO string
    severity = "error",               -- Severity level (string or number)
    message = "Interface down",       -- Main log message
    facility = "LINK",               -- Log facility/subsystem
    hostname = "switch01",           -- Source hostname/device name
    process_name = "LINEPROTO",      -- Process/daemon name
    process_id = 1234,               -- Process ID (optional)
    metadata = {                     -- Additional key-value pairs
        interface = "GigabitEthernet0/1",
        vlan = "100",
        reason = "admin_down"
    }
}
```

## Built-in API Functions

### Core NetLog API (netlog.*)

```lua
-- Log Entry Creation
entry = netlog.create_log_entry()

-- Parsing Utilities
timestamp = netlog.parse_timestamp("Jan 15 10:30:45")
severity = netlog.parse_severity("error")
device_type = netlog.parse_device_type("cisco_ios")

-- Logging Functions (for debugging)
netlog.log_debug("Debug message")
netlog.log_info("Info message")
netlog.log_warn("Warning message")
netlog.log_error("Error message")
```

### Standard Lua Libraries

All standard Lua libraries are available:
- `string.*` - String manipulation
- `table.*` - Table operations
- `math.*` - Mathematical functions
- `os.*` - Operating system interface
- `io.*` - Input/output operations

### Regular Expression Support

```lua
-- Lua pattern matching (built-in)
local match = string.match(message, "Interface (%w+)")

-- For complex regex, use string patterns or implement custom logic
local patterns = {
    cisco_ios = "^%%([%w_%-]+)%-(%d+)%-([%w_]+): (.+)",
    syslog = "^(%w+ %d+ %d+:%d+:%d+) (%w+) ([%w_]+)%[?(%d*)%]?: (.+)"
}
```

## Device Type Constants

```lua
-- Supported device types (return as strings from get_device_type())
"CiscoIOS"       -- Cisco IOS/IOS-XE devices
"CiscoNXOS"      -- Cisco NX-OS devices
"CiscoASA"       -- Cisco ASA/FTD devices
"GenericSyslog"  -- Generic syslog format
"Unknown"        -- Unknown/custom format
```

## Severity Levels

```lua
-- Severity constants (case-insensitive)
"emergency"   -- 0 - System is unusable
"alert"      -- 1 - Action must be taken immediately
"critical"   -- 2 - Critical conditions
"error"      -- 3 - Error conditions
"warning"    -- 4 - Warning conditions
"notice"     -- 5 - Normal but significant condition
"info"       -- 6 - Informational messages
"debug"      -- 7 - Debug-level messages

-- Also accepts numeric values 0-7
```

## Best Practices

### Performance Guidelines

1. **Fast Rejection**: Implement efficient `can_parse()` checks
2. **Pattern Matching**: Use simple string operations before complex regex
3. **Memory Management**: Avoid creating large temporary objects
4. **Early Returns**: Return `nil` quickly for non-matching messages

```lua
function can_parse(raw_message)
    -- Quick checks first
    if not string.find(raw_message, "%%", 1, true) then
        return false
    end

    -- More expensive checks only if needed
    return string.match(raw_message, "^%%[%w_%-]+%-[%d]+%-[%w_]+:") ~= nil
end
```

### Error Handling

```lua
function parse(raw_message)
    local success, result = pcall(function()
        -- Your parsing logic here
        return parse_cisco_ios_message(raw_message)
    end)

    if not success then
        netlog.log_error("Parse error: " .. tostring(result))
        return nil
    end

    return result
end
```

### Metadata Usage

Use the metadata table for device-specific or format-specific fields:

```lua
entry.metadata = {
    -- Network-specific
    interface = "Gi0/1",
    vlan = "100",
    mac_address = "aa:bb:cc:dd:ee:ff",

    -- Cisco-specific
    sequence = "12345",
    facility = "23",

    -- Custom fields
    rule_name = "DENY_HTTP",
    correlation_id = "event-12345"
}
```

## Error Handling and Debugging

### Built-in Validation

The NetLog engine automatically validates:
- Required function presence
- Return value types
- Log entry structure
- Script syntax errors

### Debugging Support

```lua
function parse(raw_message)
    netlog.log_debug("Parsing message: " .. raw_message)

    local entry = netlog.create_log_entry()

    -- Add debug information
    entry.metadata = entry.metadata or {}
    entry.metadata.parser_debug = "parsed_with_pattern_v2"

    return entry
end
```

## Advanced Features

### Multi-Pattern Support

```lua
local patterns = {
    interface_down = "^%%LINEPROTO%-5%-UPDOWN: Line protocol on Interface (%S+), changed state to (%w+)",
    bgp_notification = "^%%BGP%-3%-NOTIFICATION: sent to neighbor (%S+) (%d+)/(%d+)",
    ospf_neighbor = "^%%OSPF%-5%-ADJCHG: Process (%d+), Nbr (%S+) on (%S+) from (%w+) to (%w+)"
}

function parse(raw_message)
    for pattern_name, pattern in pairs(patterns) do
        local matches = {string.match(raw_message, pattern)}
        if #matches > 0 then
            return parse_with_pattern(pattern_name, matches, raw_message)
        end
    end
    return nil
end
```

### State Management

```lua
-- Module-level state (shared across all parse calls)
local message_count = 0
local last_sequence = 0

function parse(raw_message)
    message_count = message_count + 1

    local entry = netlog.create_log_entry()
    entry.metadata = {
        parser_message_count = message_count,
        sequence_gap = calculate_sequence_gap(raw_message)
    }

    return entry
end
```

## Testing and Validation

### Unit Testing Pattern

```lua
-- Test cases (can be included in parser for self-validation)
local test_cases = {
    {
        input = "%LINEPROTO-5-UPDOWN: Line protocol on Interface GigabitEthernet0/1, changed state to down",
        expected = {
            facility = "LINEPROTO",
            severity = "notice",
            message = "Line protocol on Interface GigabitEthernet0/1, changed state to down",
            metadata = {
                interface = "GigabitEthernet0/1",
                state = "down"
            }
        }
    }
}

function run_self_test()
    for i, test in ipairs(test_cases) do
        local result = parse(test.input)
        -- Validation logic here
    end
end
```

## File Organization

```
parsers/
├── cisco/
│   ├── ios-general.nlp          -- General Cisco IOS parser
│   ├── ios-interface.nlp        -- Interface-specific events
│   ├── ios-routing.nlp          -- Routing protocol events
│   ├── nxos-general.nlp         -- Cisco NX-OS parser
│   └── asa-firewall.nlp         -- ASA firewall events
├── generic/
│   ├── syslog-rfc3164.nlp       -- RFC 3164 syslog format
│   └── syslog-rfc5424.nlp       -- RFC 5424 syslog format
└── custom/
    └── company-router.nlp       -- Custom device parsers
```

This specification provides the foundation for creating powerful, efficient, and maintainable network log parsers using the NetLog Parser DSL.