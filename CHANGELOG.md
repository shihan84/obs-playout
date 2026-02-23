# Changelog

All notable changes to the OBS Time Scheduler plugin will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Initial release of OBS Time Scheduler plugin
- Time-based playlist scheduling with HH:MM format support
- Multiple playlist support with day-based filtering
- OBS Studio integration for media source control
- Scene switching capabilities
- Configuration management system
- Comprehensive logging system
- Qt-based settings dialog
- Schedule editor with JSON validation
- File watching for automatic schedule reloading
- Cross-platform support (Windows, Linux, macOS)
- GitHub Actions CI/CD pipeline
- Unit and integration tests

### Features
- **Scheduling Engine**
  - Accurate time-based triggering (< 1 second accuracy)
  - Support for recurring daily/weekly schedules
  - Fallback to idle content when no schedule is active
  - Graceful handling of timezone and DST transitions

- **Playlist Management**
  - JSON-based schedule configuration
  - Support for multiple schedule files
  - Media file validation and duration detection
  - Automatic playlist rotation

- **OBS Integration**
  - Direct control of OBS media sources
  - Scene switching coordination
  - Source visibility management
  - Real-time status display

- **User Interface**
  - Intuitive settings dialog
  - Visual schedule editor
  - Current/next item display
  - Manual override controls

- **Developer Features**
  - Comprehensive API documentation
  - Plugin architecture for extensibility
  - Memory-safe implementation using RAII
  - Thread-safe operations

### Technical Specifications
- **Language**: C++17
- **Build System**: CMake 3.20+
- **Dependencies**: libobs, obs-frontend-api, nlohmann/json, fmt
- **Platform Support**: Windows 10/11, Ubuntu 20.04+, macOS 11+
- **OBS Version**: 29.0.0+

## [1.0.0] - 2024-02-24

### Added
- Initial stable release
- Core scheduling functionality
- OBS integration
- User interface
- Documentation and examples

### Security
- Input validation for all user inputs
- Safe file handling with proper error checking
- Memory leak prevention
- Thread safety guarantees

### Performance
- Minimal CPU usage when idle
- Efficient schedule caching
- Fast JSON parsing
- Optimized OBS API calls

### Reliability
- Auto-recovery from OBS restarts
- Robust error handling
- Comprehensive logging
- Automated testing coverage

## [Future Plans]

### Version 1.1.0 (Planned)
- Advanced scheduling features (cron-like expressions)
- Web-based remote control interface
- Plugin API for third-party extensions
- Performance monitoring dashboard
- Backup and restore functionality

### Version 1.2.0 (Planned)
- Multi-language support
- Advanced transition effects
- Audio-only scheduling support
- Integration with streaming platforms
- Mobile companion app

### Version 2.0.0 (Future)
- Distributed scheduling across multiple OBS instances
- Machine learning-based content recommendations
- Advanced analytics and reporting
- Cloud-based schedule synchronization
- Enterprise features and support

---

## Support

For bug reports, feature requests, or questions:
- **Issues**: [GitHub Issues](https://github.com/your-repo/obs-time-scheduler/issues)
- **Discussions**: [GitHub Discussions](https://github.com/your-repo/obs-time-scheduler/discussions)
- **Documentation**: [Wiki](https://github.com/your-repo/obs-time-scheduler/wiki)

## Contributing

We welcome contributions! Please see our [Contributing Guidelines](CONTRIBUTING.md) for details.

## License

This project is licensed under the GPL v2 License - see the [LICENSE](LICENSE) file for details.
