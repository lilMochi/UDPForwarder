# BeamNG UDP Forwarder

## Overview

The BeamNG UDP Forwarder is a lightweight C program designed to facilitate telemetry data communication between BeamNG.drive and the DK software (Sigma Integrate Motion Systems) for motion simulation platforms. It forwards UDP packets from BeamNG.drive's telemetry output ports (9000 for Gauges, 9001 for Motion) to the DK software's expected input ports (4444 for Gauges, 4445 for Motion) on the same local machine. Additionally, it provides a direct listening mode for testing data on ports 4444 and 4445, and a debug mode to display detailed telemetry information, including vehicle speed (in mph and km/h), engine RPM, and motion parameters.

## Features

- **UDP Forwarding**: Forwards Gauges (port 9000 → 4444) and Motion (port 9001 → 4445) telemetry streams from BeamNG.drive to DK software.
- **Direct Listening Mode**: Optionally listens directly on ports 4444 and 4445 for testing incoming telemetry data without forwarding.
- **Debug Mode**: Displays detailed packet information, including:
  - Gauges: Vehicle speed (mph and km/h), engine RPM
  - Motion: Sway, surge, heave accelerations (m/s²), roll and pitch positions (radians)
  - Packet length and hexadecimal dump
- **Data Validation**: Ensures minimum packet sizes (20 bytes for Gauges, 60 bytes for Motion) as per DK software requirements.
- **Little-Endian Parsing**: Accurately interprets telemetry data in little-endian format, as specified by the BeamNG Telemetry Scheme.
- **Windows Compatibility**: Built with Winsock for reliable UDP communication on Windows systems.

## Prerequisites

- **Operating System**: Windows (tested on Windows 10 and above)
- **Development Environment**: Visual Studio (or any C compiler with Winsock support)
- **Libraries**: Winsock2 (`ws2_32.lib`)
- **BeamNG.drive**: Configured to output telemetry on UDP ports 9000 (Gauges) and 9001 (Motion)
- **DK Software**: Configured to receive telemetry on UDP ports 4444 (Gauges) and 4445 (Motion)

## Installation

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/your-username/beamng-udp-forwarder.git
   cd beamng-udp-forwarder
   ```

2. **Open in Visual Studio**:
   - Open the `.sln` file or create a new Visual Studio project.
   - Add the `UDPForwarder.c` file to the project.

3. **Link Winsock Library**:
   - In Visual Studio, go to **Project Properties > Linker > Input**.
   - Add `ws2_32.lib` to **Additional Dependencies**.

4. **Build the Project**:
   - Build the solution in Visual Studio (Debug or Release mode).

## Usage

1. **Run the Program**:
   - Execute the compiled executable (e.g., `UDPForwarder.exe`) from the command prompt or by double-clicking.

2. **Configure Modes**:
   - **Debug Mode Prompt**: Enter `1` to enable detailed telemetry output, or `0` for minimal output.
   - **Direct Listen Mode Prompt**: Enter `1` to listen directly on ports 4444 and 4445 (for testing), or `0` to forward from 9000/9001 to 4444/4445.

3. **Example Interaction**:
   ```
   Enable debug output? (1 for yes, 0 for no): 1
   Debug mode enabled
   Listen directly to ports 4444/4445? (1 for yes, 0 for no): 0
   Forwarding mode enabled (9000->4444, 9001->4445)
   Listening for UDP packets on ports 9000 (Gauges) and 9001 (Motion)...
   ```

4. **Debug Output (if enabled)**:
   ```
   === Port 9000 (Gauges) Received ===
   Length: 20 bytes
   Vehicle Speed: 34.67 mph (55.80 km/h) (offset 12-15)
   Engine RPM: 2500.00 (offset 16-19)
   Hex dump: ... [20 bytes of data]
   Forwarded to local port 4444
   ```

5. **Testing with Direct Listen Mode**:
   - Select `1` for direct listening to monitor data sent to 4444/4445 (useful for verifying DK software input).

## Troubleshooting

- **Bind Errors**:
  - Ensure no other applications are using ports 9000, 9001, 4444, or 4445.
  - Check firewall settings to allow UDP traffic on these ports.

- **No Data Received**:
  - Verify BeamNG.drive is configured to send telemetry to 127.0.0.1:9000 (Gauges) and 127.0.0.1:9001 (Motion).
  - Ensure DK software is listening on 127.0.0.1:4444 (Gauges) and 127.0.0.1:4445 (Motion).

- **Packet Size Errors**:
  - If debug mode reports packets smaller than 20 bytes (Gauges) or 60 bytes (Motion), check BeamNG.drive's telemetry output configuration.

- **Winsock Errors**:
  - Ensure `ws2_32.lib` is linked correctly in the project settings.
  - Run the program with administrator privileges if network issues persist.

## Contributing

Contributions are welcome! Please submit a pull request or open an issue on GitHub for bug reports, feature requests, or improvements.

## Acknowledgments

- Built for compatibility with BeamNG.drive and Sigma Integrate Motion Systems (DK software).
- Based on the [BeamNG Telemetry Scheme](https://github.com/your-username/beamng-udp-forwarder/raw/main/BeamNG_Telemetry_Scheme.pdf) for accurate data handling.
