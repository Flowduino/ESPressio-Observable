# ESPressio Observable

Synchronous Observer Pattern components for the Flowduino ESPressio Development Platform.

ESPressio Observable provides small, reusable building blocks for one-to-many synchronous notification without forcing an Observable object to know which concrete consumers are listening to it.

## Latest Stable Version

**3.0.1**

Version 3.0.1 preserves the Observable 3.0 ownership-safe registration API while reducing the cost of the no-observer path in `ThreadSafeObservable`.

## Why use ESPressio Observable?

Use Observable when a state change and its notification belong to the same synchronous operation, but the producer should remain independent of the concrete consumers.

Compared with a single callback, an Observable can notify any number of independently implemented Observer objects. Compared with ESPressio Event, Observable does not create an asynchronous scheduling boundary.

```text
Observable
   +--> Observer A
   +--> Observer B
   +--> Observer C
```

This makes Observable particularly useful for lifecycle notifications, state-change callbacks, diagnostics hooks, and low-overhead subsystem observation.

## Observer Pattern and dependency direction

A well-designed Observer relationship is one-way:

```text
Observer -----> Observable
Observable -X-> concrete Observer
```

The Observable defines the notification contract, but never stores knowledge of concrete application consumers. This is a useful way to avoid circular references while still allowing multiple components to react to the same state change.

### Do

- Define small, logically focused Observer interfaces.
- Keep Observer callbacks short unless the synchronous work is intentionally part of the notifying operation.
- Keep an Observer alive for as long as it remains registered.
- Retain the registration handle for exactly as long as the registration should exist.
- Use `ThreadSafeObservable` when registration/notification can occur from multiple threads.

### Do not

- Introduce a back-reference from an Observable implementation to a concrete Observer.
- Mutate shared notification arguments unless the notification contract explicitly permits it.
- Assume `ThreadSafeObservable` automatically makes the custom state in your derived class thread-safe.
- Retain a raw pointer returned by `IObserverHandle::GetObservable()` or `GetObserver()` beyond the immediate operation.

## ESPressio Development Platform

ESPressio is a collection of discrete, composable component libraries built around a common development ethos:

- **Light-weight** — minimise memory consumption and runtime overhead without sacrificing correctness.
- **Ease of use** — provide strongly typed, developer-friendly abstractions over lower-level facilities.
- **Object-oriented** — a type for everything, and everything in a type.
- **SOLID** — favour focused responsibilities, extensibility, substitutable abstractions, narrow interfaces, and dependency inversion wherever practical on embedded C++ platforms.

## License

Licensed under the **Apache License 2.0**. See [LICENSE](LICENSE).

## Namespace

```cpp
ESPressio::Observable
```

The most important public types are:

- `IObserver`
- `IObserverHandle`
- `ObserverHandlePtr`
- `IObservable`
- `IUntypedObservable`
- `Observable`
- `ThreadSafeObservable`
- `ObservableWithBuckets`

## Installation

PlatformIO:

```ini
lib_deps =
    flowduino/ESPressio-Observable@^3.0.1
```

Or, when intentionally tracking the latest development branch:

```ini
lib_deps =
    https://github.com/Flowduino/ESPressio-Observable.git
```

The library requires C++ RTTI because notification filtering uses `dynamic_cast` against Observer interfaces. If your toolchain disables RTTI by default, enable it for the application, for example:

```ini
build_unflags =
    -fno-rtti
```

## Basic usage: a Thermometer Observable

The following example deliberately uses a small, concrete problem: a `Thermometer` detects a changed reading and synchronously notifies any registered temperature Observers.

### 1. Define the Observer interface

```cpp
#pragma once

#include <ESPressio_IObserver.hpp>

class ITemperatureObserver :
    public virtual ESPressio::Observable::IObserver {
public:
    virtual ~ITemperatureObserver() = default;

    virtual void OnTemperatureChanged(
        float previous,
        float current
    ) {}

    virtual void OnTemperatureIncreased(float by) {}
    virtual void OnTemperatureDecreased(float by) {}
};
```

The callback bodies are intentionally optional. A concrete Observer can implement only the notifications it needs.

### 2. Implement the Observable

```cpp
#pragma once

#include <ESPressio_Observable.hpp>
#include "ITemperatureObserver.hpp"

class Thermometer final :
    public ESPressio::Observable::Observable {
private:
    float _temperature = 0.0f;

public:
    float GetTemperature() const {
        return _temperature;
    }

    void SetTemperature(float temperature) {
        if (_temperature == temperature) {
            return;
        }

        const float previous = _temperature;
        _temperature = temperature;

        ExecuteNotification([&](NotificationContext& notification) {
            notification.WithObservers<ITemperatureObserver>(
                [&](ITemperatureObserver* observer) {
                    observer->OnTemperatureChanged(previous, temperature);

                    if (temperature > previous) {
                        observer->OnTemperatureIncreased(
                            temperature - previous
                        );
                    } else {
                        observer->OnTemperatureDecreased(
                            previous - temperature
                        );
                    }
                }
            );
        });
    }
};
```

`ExecuteNotification()` retains the Observable for the complete notification operation. In the 3.x API, an Observable participating in notifications must therefore be owned through `std::shared_ptr`.

### 3. Implement an Observer

```cpp
#pragma once

#include <Arduino.h>
#include "ITemperatureObserver.hpp"

class TemperatureLogger final :
    public ITemperatureObserver {
public:
    void OnTemperatureChanged(
        float previous,
        float current
    ) override {
        Serial.printf(
            "Temperature changed from %.2f to %.2f\n",
            previous,
            current
        );
    }

    void OnTemperatureIncreased(float by) override {
        Serial.printf("Temperature increased by %.2f\n", by);
    }

    void OnTemperatureDecreased(float by) override {
        Serial.printf("Temperature decreased by %.2f\n", by);
    }
};
```

### 4. Register the Observer and retain the handle

```cpp
#include <Arduino.h>
#include <memory>

#include "Thermometer.hpp"
#include "TemperatureLogger.hpp"

std::shared_ptr<Thermometer> thermometer;
TemperatureLogger temperatureLogger;
ESPressio::Observable::ObserverHandlePtr temperatureRegistration;

void setup() {
    Serial.begin(115200);

    thermometer = std::make_shared<Thermometer>();

    temperatureRegistration =
        thermometer->RegisterObserver(&temperatureLogger);

    thermometer->SetTemperature(21.5f);
    thermometer->SetTemperature(22.0f);
}

void loop() {}
```

The returned `ObserverHandlePtr` is a `std::unique_ptr<IObserverHandle>`. Destroying the handle automatically unregisters the Observer. You can also unregister explicitly:

```cpp
temperatureRegistration->Unregister();
```

or directly through the Observable:

```cpp
thermometer->UnregisterObserver(&temperatureLogger);
```

The application must keep `temperatureLogger` alive for the complete period in which the registration is active.

## Registration lifetime and ownership

Version 3.x deliberately makes registration lifetime explicit and ownership-safe:

```text
std::shared_ptr<Observable>
        |
        +-- registration --> ObserverHandlePtr
                               |
                               +-- destruction/unregister
                                   removes registration
```

Important rules:

- `RegisterObserver()` rejects `nullptr`.
- Duplicate registration against the same Observable is rejected.
- The Observer remains non-owning; registering it does not extend its lifetime.
- The registration handle does not own the Observable.
- Observable destruction invalidates outstanding registrations safely.
- Notification-aware Observable instances must be `std::shared_ptr`-owned so `ExecuteNotification()` can retain them while callbacks execute.

## One Observable, multiple Observer interfaces

A single Observable can expose several independent notification contracts. This is useful when different consumers care about different aspects of the same subsystem.

For example, a physical environmental sensor could define both:

```cpp
class ITemperatureObserver :
    public virtual ESPressio::Observable::IObserver {
public:
    virtual void OnTemperatureChanged(float previous, float current) {}
};

class IAirPressureObserver :
    public virtual ESPressio::Observable::IObserver {
public:
    virtual void OnAirPressureChanged(float previous, float current) {}
};
```

The Observable can then target each interface independently:

```cpp
ExecuteNotification([&](NotificationContext& notification) {
    notification.WithObservers<ITemperatureObserver>(
        [&](ITemperatureObserver* observer) {
            observer->OnTemperatureChanged(oldTemperature, newTemperature);
        }
    );
});

ExecuteNotification([&](NotificationContext& notification) {
    notification.WithObservers<IAirPressureObserver>(
        [&](IAirPressureObserver* observer) {
            observer->OnAirPressureChanged(oldPressure, newPressure);
        }
    );
});
```

A concrete Observer may implement one interface or several of them. This keeps notification contracts focused and supports the Interface Segregation Principle without forcing the Observable to know which combinations exist.

## Thread-safe Observables

`Observable` is intentionally not thread-safe. It supports registration/unregistration during notification, but simultaneous operations from multiple threads require external synchronization.

When registrations or notifications can cross thread boundaries, derive from:

```cpp
#include <ESPressio_ThreadSafeObservable.hpp>

class Thermometer final :
    public ESPressio::Observable::ThreadSafeObservable {
    // notification code follows the same model
};
```

The Observer-facing model remains the same: register an `IObserver`, retain the returned handle, and use the protected notification operation supplied by the Observable implementation.

> **Important:** `ThreadSafeObservable` protects its Observer registration/notification machinery. It does not automatically protect members such as `_temperature`, sensor buffers, configuration state, or any other fields added by your derived class.

## Mutation during notification

The current implementation deliberately supports an Observer unregistering while a notification is in progress. Registration containers are compacted safely after the outer notification operation completes.

This makes patterns such as one-shot observers practical without invalidating the iteration currently delivering a notification.

## `ObservableWithBuckets`: faster typed dispatch

`ObservableWithBuckets` is a non-thread-safe alternative for applications that repeatedly notify specific Observer interfaces and want to avoid a `dynamic_cast` for every Observer on every notification.

Unlike `Observable`, the interface set is declared at registration time:

```cpp
#include <ESPressio_ObservableWithBuckets.hpp>

class Sensor :
    public ESPressio::Observable::ObservableWithBuckets {
public:
    void NotifyTemperature(float value) {
        ExecuteNotification([&](NotificationContext& notification) {
            notification.WithObservers<ITemperatureObserver>(
                [&](ITemperatureObserver* observer) {
                    observer->OnTemperatureChanged(value, value);
                }
            );
        });
    }
};

auto sensor = std::make_shared<Sensor>();
TemperatureLogger logger;

auto handle = sensor->RegisterObserverAs<ITemperatureObserver>(
    &logger
);
```

An Observer implementing several interfaces can register all of them in one operation:

```cpp
auto handle = sensor->RegisterObserverAs<
    ITemperatureObserver,
    IAirPressureObserver
>(&environmentDisplay);
```

The library validates each requested interface when registering and stores the resolved interface pointer in a type-specific bucket. Later `WithObservers<T>()` calls therefore iterate only the relevant bucket.

Use `ObservableWithBuckets` when:

- notification frequency is high enough that repeated RTTI filtering matters;
- Observer interface sets are known when registering; and
- the Observable does not require concurrent thread-safe registration/notification.

Registration remains ownership-safe and uses the same `ObserverHandlePtr` lifetime model. Registering the same Observer again with a different interface set is rejected rather than silently changing its contract.

## Observable vs Event

Use Observable when the notification is synchronous and naturally belongs to the operation being performed:

```text
operation -> state changes -> Observer callbacks -> operation continues
```

Use ESPressio Event when producers and consumers should be independently scheduled:

```text
producer -> dispatch Event -> producer continues
                         \
                          -> asynchronous consumer(s)
```

Observable does **not** depend on Event. Higher-level libraries may consume Observable callbacks and optionally translate them into Events, preserving a one-way dependency graph.

## ESPressio Library Dependencies

**Required ESPressio dependencies: none.**

For the complete ESPressio hierarchy, including optional downstream integrations, see [ESPRESSIO_DEPENDENCY_CHART.md](ESPRESSIO_DEPENDENCY_CHART.md).

- Solid relationships represent required dependencies.
- Dashed relationships represent opt-in dependencies.

## Design goals

- Focused Observer contracts.
- Explicit registration lifetime.
- Non-owning Observer relationships.
- Synchronous deterministic notification.
- Safe registration mutation during callbacks.
- A thread-safe implementation when required.
- Reusable infrastructure with no dependency on ESPressio Event.
