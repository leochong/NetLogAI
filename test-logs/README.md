# NetLogAI Test Logs

This directory contains sample network log files for testing the NetLogAI parser and analysis engine.

## Directory Structure

```
test-logs/
├── cisco-ios/          # Cisco IOS device logs
│   ├── bgp/            # BGP-related logs
│   ├── ospf/           # OSPF-related logs
│   ├── interface/      # Interface up/down logs
│   ├── system/         # System configuration logs
│   └── security/       # Security and authentication logs
├── cisco-asa/          # Cisco ASA firewall logs
├── cisco-nx-os/        # Cisco Nexus NX-OS logs
└── cisco-ios-xr/       # Cisco IOS XR logs
```

## Sample Sources

- **GitHub Repositories**: elastic/beats, logstash samples
- **Cisco Documentation**: Official syslog message guides
- **Generated Samples**: Synthetic logs for testing specific scenarios

## Usage

```bash
# Test specific log type
./build/Release/netlogai.exe analyze test-logs/cisco-ios/bgp/bgp_neighbor_down.log

# Test parser against all samples
./build/Release/netlogai.exe parser test --directory test-logs/
```