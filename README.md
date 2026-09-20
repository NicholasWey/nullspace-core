# NullSpace Core

**A 1:1 hyper-realistic spacecraft GNC simulation engine.**

Build a payload, pick a launch vehicle, and operate from separation onward. NullSpace models spacecraft dynamics, sensors, and actuators at the fidelity level used in flight-software development. Reaction wheel jitter from bearing imbalance. IMU bias instability derived from Allan variance parameters. Fuel slosh coupled to rigid-body dynamics through equivalent mechanical models. Quaternion-based attitude control with no Euler angle singularities.

The flight software never sees truth. It estimates state from noisy sensors and commands imperfect actuators, exactly like a real vehicle.

```
+-----------------------------------------------------------------+
|                     Truth Side (~1 kHz)                         |
|  ┌───────────┐   ┌──────────┐   ┌──────────┐   ┌─────────────┐  |
|  │ Dynamics  │   │ Environ- │   │ Sensor   │   │  Actuator   │  |
|  │  Module   │   │  ment    │   │ Models   │   │  Models     │  |
|  └─────┬─────┘   └────┬─────┘   └────┬─────┘   └──────┬──────┘  |
|        └──────────────┴──────┬───────┴────────────────┘         |
|                              │                                  |
|                     Typed Message Bus                           |
|                              │                                  |
|              ┌───────────────┴───────────────────┐              |
|              │                                   │              |
|  ┌───────────┴────────────────────┐  ┌───────────┴───────────┐  |
|  │   Hard Real-Time (100+ Hz)     │  │Soft Real-Time (1-50Hz)│  |
|  │                                │  │                       │  |
|  │  Gyro Read                     │  │  Guidance             │  |
|  │    -> MEKF Propagate           │  │  Mode Logic           │  |
|  │    -> Star Tracker Update      │  │  Momentum Management  │  |
|  │    -> PID Controller           │  │  Vision Processing    │  |
|  │    -> Wheel Allocation         │  │  Telemetry            │  |
|  │    -> Actuator Command         │  │                       │  |
|  └────────────────────────────────┘  └───────────────────────┘  |
+-----------------------------------------------------------------+
                              │
                     Public C API / Python Bindings
                              │
              ┌───────────────┴───────────────┐
              │      Game Layer (Godot 4)     │
              │   (separate commercial repo)  │
              └───────────────────────────────┘
```

---

## Design Principles

**Hard truth/FSW separation.** Flight software modules receive only sensor outputs and produce only actuator commands. They never access true state. This is the same constraint real flight software operates under, and it is the constraint that makes GNC interesting.

**Fused estimation and control.** The hard real-time loop runs as a single pipeline: read gyro, propagate the estimate, incorporate star tracker measurements when available, compute control error, allocate torque, command actuators. No artificial separation between estimator and controller. This matches how real spacecraft flight software works.

**Deterministic seeded runs.** Every noise source, perturbation, and dispersion draws from a per-module seeded PRNG derived from a master seed. Same seed, same results. This enables regression testing, Monte Carlo analysis, and exact replay.

**Doubles everywhere.** All simulation state uses double-precision floating point. No single-precision shortcuts in dynamics or navigation.

**Derivation-first.** Every physics and control module is derived from first principles before implementation, then verified against analytical solutions. No black boxes.

---

## Conventions

| Convention | Choice |
|-----------|--------|
| Quaternion | Scalar-first `[w,x,y,z]`, Hamilton (active). `q` rotates vectors from inertial to body frame |
| Inertial frame | J2000 ECI (CSPICE default) |
| Body frame | X-forward, Y-right, Z-down (aerospace standard) |
| Units (internal) | SI + radians everywhere |
| Units (config) | Degrees accepted with `_deg` suffix, converted once at the parser |
| Time (internal) | TDB seconds past J2000 epoch (CSPICE `et` format) |
| Time (config) | UTC input, converted to TDB at initialization |

---

## What's Inside

### Dynamics
- Rigid-body rotational dynamics (Euler's equation + quaternion kinematics)
- Two-body and multi-body orbital mechanics (JPL DE440 via CSPICE)
- Spherical harmonics (configurable degree/order)
- Fuel slosh (equivalent pendulum/spring-mass models, fill-fraction dependent)
- Flexible appendage dynamics (modal coordinates, time-varying inertia)
- Variable mass and center-of-gravity tracking
- RK4 integrator (fixed-step), optional RK7(8) Dormand-Prince

### Environment
- Multi-body gravity: Sun, Moon, planets via JPL DE440 ephemerides
- Atmospheric drag: exponential or Harris-Priester model
- Solar radiation pressure: flat-plate model
- Earth magnetic field: IGRF/WMM

### Actuators
- Reaction wheels: arbitrary count and orientation, torque/speed limits, Coulomb + viscous friction, static/dynamic imbalance (jitter)
- Pseudo-inverse torque allocation with null-space momentum management (4+ wheels)
- Thrusters: misalignment, magnitude error, minimum impulse bit, PWM, rise/fall transients
- Fault injection hooks on all actuators

### Sensors
- IMU: bias, bias instability, angle random walk, scale factor, misalignment (Allan variance parameterized)
- Star tracker: per-axis noise, sun/earth/moon exclusion zones, slew rate cutoff, measurement latency
- GPS: position/velocity with noise (LEO), relative GPS for proximity ops
- Camera: noisy pose measurement (upgradeable to rendered images)
- Fault injection hooks on all sensors

### Flight Software (reference implementations)
- Hard real-time loop (100+ Hz): fused MEKF + PID attitude control pipeline
- Soft real-time loop (1-50 Hz): guidance, mode logic, momentum management, vision processing
- Momentum desaturation controller
- Proximity operations guidance (V-bar/R-bar approach, braking gates)

### Infrastructure
- Deterministic fixed-rate scheduler
- Typed publish/subscribe message bus
- Headless simulation runner
- Monte Carlo framework (parallel runs, dispersion tables, statistical output)
- Binary telemetry with Python reader and matplotlib plotting tools
- Validation test suite (analytical benchmarks, linear analysis, Basilisk cross-reference)

---

## Tech Stack

| Component | Technology |
|-----------|-----------|
| Language | C++20 |
| Linear algebra | Eigen 3 |
| Build system | CMake 3.20+ |
| Ephemerides | CSPICE |
| Testing | Google Test + Google Benchmark |
| CI | GitHub Actions (Windows MSVC + Linux GCC/Clang) |
| Config format | JSON |
| Dev plotting | Python + matplotlib |
| Plugin API | Versioned C ABI + Python bindings (pybind11) |

---

## Project Structure

```
nullspace-core/
├── CMakeLists.txt
├── README.md
├── LICENSE                       Apache 2.0
├── cmake/                        CMake modules, FetchContent deps
├── src/
│   ├── core/                     Scheduler, message bus, clock, PRNG
│   ├── dynamics/                 Rigid body, orbital mechanics, integrator
│   ├── actuators/                Reaction wheels, thrusters
│   ├── sensors/                  IMU, star tracker, GPS, camera
│   ├── environment/              Gravity, drag, SRP, magnetic field, ephemerides
│   ├── fsw/                      Reference controllers, estimators, guidance
│   ├── disturbances/             Fuel slosh, flex appendages, jitter
│   ├── power/                    Power budget, battery, solar panel models
│   └── faults/                   Fault injection framework
├── include/
│   └── nullspace/                Public API headers
├── plugins/                      Plugin API definition, examples
├── scenarios/                    JSON scenario definitions
├── test/
│   ├── unit/                     Google Test unit tests
│   ├── integration/              Multi-module integration tests
│   ├── validation/               Analytical verification tests
│   └── baseline/                 Regression baseline telemetry
├── tools/
│   ├── plot_telemetry.py         Standard telemetry plotting
│   ├── mc_report.py              Monte Carlo analysis and reporting
│   └── allan_variance.py         IMU Allan variance analysis
└── data/
    ├── ephemerides/              DE440 binary files
    ├── gravity/                  Spherical harmonic coefficients
    └── magnetic/                 IGRF/WMM coefficients
```

---

## Plugin API

Versioned C ABI for custom controllers, estimators, and guidance algorithms:

```c
// nullspace/plugin_api.h
#define NULLSPACE_API_VERSION 1

typedef struct {
    int api_version;
    const char* name;
    void (*init)(const ns_config_t* config);
    void (*update)(double dt, const ns_sensor_data_t* sensors, ns_actuator_cmd_t* commands);
    void (*cleanup)(void);
} ns_plugin_t;

NS_EXPORT ns_plugin_t* ns_plugin_create(void);
```

Python bindings for rapid prototyping:

```python
import nullspace as ns

sim = ns.Simulation("scenarios/detumble_slew.json", seed=42)
sim.run(duration=5400.0)  # one orbit
telem = sim.telemetry()
```

---

## Validation Strategy

### Tier 1: Analytical Verification

| Test Case | Reference | Acceptance |
|-----------|-----------|------------|
| Torque-free axisymmetric body | Nutation frequency | < 0.01% error |
| Two-body orbit period | Kepler's third law | < 1e-12 relative |
| J2 secular RAAN drift | Brouwer theory | < 0.1% over 30 days |
| CW relative motion | Clohessy-Wiltshire equations | < 0.1% at 1 orbit |
| Quaternion norm | Unit constraint | Drift < 1e-14 per step |
| Energy conservation (torque-free) | Rotational kinetic energy | Drift < 1e-10 over 10k orbits |

### Tier 2: Linear Analysis

- Linearize the plant (rigid body + wheels) and controller around operating points
- Bode and Nyquist plots to verify gain and phase margins
- Stability margin verification across the operating envelope (different inertias, wheel configs)
- Frequency-domain validation that the nonlinear sim matches linear predictions for small perturbations
- Linear covariance analysis to cross-check Monte Carlo pointing performance results

### Tier 3: Cross-Validation with Basilisk

Key scenarios run in both NullSpace and Basilisk to validate agreement. State-vector differences within integration tolerance (~1e-8 relative) for identical initial conditions.

### Tier 4: Statistical Validation

- Monte Carlo dispersions match configured input distributions
- NEES/NIS filter consistency at 95% confidence
- Allan variance recovery from simulated IMU output
- Monte Carlo pointing results consistent with linear covariance predictions

---

## Build

```bash
# Prerequisites: CMake 3.20+, C++20 compiler
# Eigen, Google Test, and CSPICE are pulled automatically via FetchContent
git clone https://github.com/<user>/nullspace-core.git
cd nullspace-core
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

### Headless Run

```bash
./build/bin/nullspace-run --config scenarios/detumble_slew.json --seed 42
python tools/plot_telemetry.py output/detumble_slew_42.bin
```

### Monte Carlo

```bash
./build/bin/nullspace-mc --config scenarios/detumble_slew.json --runs 1000 --threads 8
python tools/mc_report.py output/mc_detumble_slew/
```

---

## Roadmap

1. **Rigid Body Attitude Control** - single spacecraft, reaction wheels, quaternion PID, momentum desaturation, headless with plots
2. **Estimation and Sensors** - IMU/star tracker models, MEKF, closed-loop on estimated state, power budget
3. **High-Fidelity Dynamics** - thruster imperfections, slosh, flex, wheel jitter, Monte Carlo framework
4. **Multi-Vehicle and Docking** - two-vehicle dynamics, relative nav, proximity guidance, cooperative docking
5. **Advanced Operations** - uncooperative targets, camera-based nav, fault campaigns

---

## License

Apache 2.0. See [LICENSE](LICENSE).

Built from scratch. Cross-validated against [Basilisk](https://hanspeterschaub.info/basilisk/) for confidence, not derived from it. Ephemeris data from NASA/JPL [NAIF SPICE](https://naif.jpl.nasa.gov/naif/).
