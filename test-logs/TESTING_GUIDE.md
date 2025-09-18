# NetLogAI Testing Guide with Log Files

## ✅ Testing Setup Complete

Your NetLogAI project now has a comprehensive log file testing infrastructure that eliminates the need for GNS3 or EVE-NG network simulation.

## 📁 Test Structure Created

```
test-logs/
├── cisco-ios/
│   ├── bgp/bgp_neighbor_down.log           # BGP neighbor state changes
│   ├── interface/interface_flap.log        # Interface up/down events
│   ├── ospf/ospf_neighbor_changes.log      # OSPF adjacency changes
│   └── system/config_changes.log          # Configuration modifications
├── cisco-asa/
│   └── security/
│       ├── firewall_blocks.log             # Security blocks & denies
│       └── real_asa_samples.log           # Real Elastic Beats samples
├── cisco-nx-os/
│   └── interface/nexus_port_status.log     # Nexus interface events
└── cisco-ios-xr/                          # Ready for IOS-XR samples
```

## 🧪 Testing Commands

### Basic Parser Testing
```bash
# Test BGP analysis
./netlogai-core/build/Release/netlogai.exe analyze test-logs/cisco-ios/bgp/bgp_neighbor_down.log

# Test interface flapping
./netlogai-core/build/Release/netlogai.exe analyze test-logs/cisco-ios/interface/interface_flap.log

# Test ASA firewall logs
./netlogai-core/build/Release/netlogai.exe analyze test-logs/cisco-asa/security/firewall_blocks.log

# List available parsers
./netlogai-core/build/Release/netlogai.exe parser list
```

### AI Analysis (Requires API Key)
```bash
# Configure AI first
./netlogai-core/build/Release/netlogai.exe config ai

# Then analyze with AI
./netlogai-core/build/Release/netlogai.exe ask "Why is BGP neighbor 192.168.1.1 down?" --file test-logs/cisco-ios/bgp/bgp_neighbor_down.log
```

### Automated Testing
```bash
# Run all tests
cd test-logs
run-tests.bat
```

## 📊 Sample Log Patterns Available

### BGP Issues
- `%BGP-5-ADJCHANGE: neighbor Down/Up`
- `%BGP-3-NOTIFICATION: hold time expired`
- `%BGP-4-MAXPFXEXCEED: prefix limit reached`

### Interface Problems
- `%LINEPROTO-5-UPDOWN: Line protocol changed state`
- `%LINK-3-UPDOWN: Interface changed state`

### OSPF Adjacency
- `%OSPF-5-ADJCHG: Process from FULL to DOWN`
- Full OSPF state machine transitions

### ASA Security Events
- `%ASA-4-106023: Deny tcp/udp traffic`
- `%ASA-6-302013: Built outbound connection`
- `%ASA-1-106021: Reverse path check failures`

## 🎯 Next Steps

1. **Test Parser Functionality**: Use the analyze command to test basic parsing
2. **AI Configuration**: Set up Claude API key for intelligent analysis
3. **Add More Samples**: Expand test-logs/ with your specific network scenarios
4. **Create Custom Scenarios**: Add device-specific log patterns for your environment

## 💡 Benefits of This Approach

✅ **No Network Setup Required** - Focus on core functionality
✅ **Reproducible Tests** - Same logs every time
✅ **Version Control Friendly** - Track test changes
✅ **CI/CD Ready** - Easy automation
✅ **Real-World Samples** - Based on actual Cisco documentation

Your NetLogAI project can now move forward with testing and development without network simulation complexity!