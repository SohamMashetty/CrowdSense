# CrowdSense: Intelligent Transit Planner for Hyderabad

CrowdSense is a fast, STL-only C++14 route planner that intelligently finds optimal public transport paths through Hyderabad’s RTC network. It supports walking transfers, route-switching with transfer penalties, and time-window based planning — all built with clean, low-overhead graph algorithms and minimal dependencies.

---

## Features

- **Multi-route transfers** with configurable limits  
- **Walking segment support** (<1km) with 15-minute estimated travel time  
- **Time-window aware planning** (earliest departure, latest arrival)  
- **Transfer penalty logic** (+5 min per switch)  
- **Built from scratch in C++14 using only STL**  
- **Extensible**: Plug in real-time crowd/load data, predictive models, or delays

  Future Scope
 Real-time crowd level prediction using sensor data
 Adaptive re-routing with user feedback integration
 GUI with map overlays for interactive route visualization
