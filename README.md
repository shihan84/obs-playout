# OBS Studio Time-Based Playlist Scheduler Plugin

A professional-grade OBS Studio plugin that enables time-based playlist scheduling for Media Sources with seamless transitions and automated content management.

## 🚀 Features

- **Time-Based Scheduling**: Schedule media files to play at specific times (HH:MM format)
- **Recurring Schedules**: Support for daily/weekly recurring schedules
- **Multiple Playlists**: Support for multiple playlists that can be toggled
- **Seamless Transitions**: Handle media transitions smoothly between scheduled items
- **Fallback Content**: Automatic fallback to idle content when no schedule is active
- **OBS Integration**: Full integration with OBS scenes and media sources
- **Real-time UI**: Current/next item display with manual override controls
- **File Monitoring**: Automatic reload when schedule files change

## 📋 Requirements

- OBS Studio 29.0.0 or higher
- Windows 10/11 (x64), Ubuntu 20.04+ (x86_64), or macOS 11+ (Intel/Apple Silicon)

## 🔧 Installation

### Windows
1. Download the latest release from [GitHub Releases](https://github.com/your-repo/obs-time-scheduler/releases)
2. Run the installer and follow the setup wizard
3. Restart OBS Studio

### Linux (Ubuntu/Debian)
```bash
wget https://github.com/your-repo/obs-time-scheduler/releases/latest/download/obs-time-scheduler.deb
sudo dpkg -i obs-time-scheduler.deb
sudo apt-get install -f  # Fix dependencies if needed
```

### macOS
1. Download the `.dmg` file from [GitHub Releases](https://github.com/your-repo/obs-time-scheduler/releases)
2. Open the DMG and drag the plugin to your OBS plugins folder
3. Restart OBS Studio

## 📖 Usage

### Creating a Schedule

Create a JSON schedule file (e.g., `schedule.json`):

```json
{
  "version": "1.0",
  "timezone": "America/New_York",
  "default_idle": "idle_video.mp4",
  "playlists": [
    {
      "name": "Weekday Schedule",
      "days": ["monday", "tuesday", "wednesday", "thursday", "friday"],
      "items": [
        {
          "time": "09:00",
          "source": "Morning_Show",
          "file": "C:\\videos\\morning_intro.mp4",
          "duration": 300,
          "scene": "Main_Scene"
        },
        {
          "time": "12:00",
          "source": "Lunch_Break",
          "file": "C:\\videos\\intermission.mp4",
          "loop": true,
          "scene": "Waiting_Scene"
        }
      ]
    }
  ]
}
```

### Configuration

1. Open OBS Studio
2. Go to **Tools → Time Scheduler Settings**
3. Add your schedule file using the file browser
4. Configure your media sources and scenes
5. Enable the scheduler

### Schedule File Format

- **version**: Schedule format version (currently "1.0")
- **timezone**: Timezone for schedule times (IANA timezone database format)
- **default_idle**: Default media file to play when no schedule is active
- **playlists**: Array of playlist configurations
  - **name**: Human-readable playlist name
  - **days**: Array of days this playlist is active (lowercase English day names)
  - **items**: Array of scheduled items
    - **time**: Time in HH:MM format (24-hour)
    - **source**: OBS media source name to control
    - **file**: Path to media file
    - **duration**: Duration in seconds (optional, auto-detected if not specified)
    - **loop**: Whether to loop the media (default: false)
    - **scene**: OBS scene to switch to (optional)

## 🏗️ Building from Source

### Prerequisites

- CMake 3.20 or higher
- C++17 compatible compiler
- OBS Studio development files
- Git

### Build Steps

```bash
# Clone the repository
git clone https://github.com/your-repo/obs-time-scheduler.git
cd obs-time-scheduler

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build the plugin
cmake --build . --config Release

# Install (optional)
cmake --install .
```

### Platform-Specific Notes

#### Windows
- Use Visual Studio 2022 or newer
- Install OBS Studio with development headers

#### Linux
```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake libobs-dev

# Fedora
sudo dnf install gcc-c++ cmake libobs-devel
```

#### macOS
```bash
# Install dependencies with Homebrew
brew install cmake obs
```

## 🧪 Testing

Run the test suite:

```bash
cd build
ctest --output-on-failure
```

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📝 License

This project is licensed under the GPL v2 License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- OBS Studio team for the excellent plugin API
- Contributors and beta testers
- The open-source community

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/your-repo/obs-time-scheduler/issues)
- **Discussions**: [GitHub Discussions](https://github.com/your-repo/obs-time-scheduler/discussions)
- **Documentation**: [Wiki](https://github.com/your-repo/obs-time-scheduler/wiki)

## 🔄 Changelog

See [CHANGELOG.md](CHANGELOG.md) for version history and updates.
