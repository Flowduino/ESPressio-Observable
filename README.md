# ESPressio Observable
Observer Pattern Components of the Flowduino ESPressio Development Platform

Provides a foundation for designing, structuring, and implementing your embedded programs using Observer Pattern.

## Latest Stable Version
There is currently no stable released version.

## ESPressio Development Platform
The **ESPressio** Development Platform is a collection of discrete (sometimes intra-connected) Component Libraries developed with a particular development ethos in mind.

The key objectives of the ESPressio Development Platform are:
- **Light-weight** - The Components should always strive to optimize memory consumption and operational overhead as much as possible, but not to the detriment of...
- **Ease of Use** - Many of our components serve as Developer-Friendly Abstractions of existing procedural code libraries.
- **Object-Oriented** - A `type` for everything, and everything in a `type`!
- **SOLID**:
- -  > **S**ingle Responsibility Principle (SRP)
    Break your code into smaller, focused components.
- - > **O**pen/Closed Principle (OCP)
    Be open for extension but closed for modification.
- - > **L**iskov Substitution Principle (LSP)
    Be substitutable for the base type without altering correctness.
- - > **I**nterface Segregation Principle (ISP)
    Break interfaces into specific, client-focused ones.
- - > **D**ependency Inversion Principle (DIP)
    Be dependent on abstractions, not concretions.

To the maximum extent possible within the limitations/restrictons/constraints of the C++ langauge, the Arduino platform, and Microcontroller Programming itself, all Component Libraries of the **ESPressio** Development Platform must strive to honour the **SOLID** principles.

## License
ESPressio (and its component libraries, including this one) are subject to the *Apache License 2.0*
Please see the [![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE) accompanying this library for full details.

## Namespace
Every type/variable/constant/etc. related to *ESPressio* Observable are located within the `Observable` submaspace of the `ESPressio` parent namespace.

The namespace provides the following (*click on any declaration to navigate to more info*):
- [`ESPressio::Observable::IObserverHandle`](#iobserverhandle)
- [`ESPressio::Observable::IObservable`](#iobservable)
- [`ESPressio::Observable::IObserver`](#iobserver)
- [`ESPressio::Observable::ObserverHandle`](#observerhandle)
- [`ESPressio::Observable::ThreadSafeObservable`](#threadsafeobservable)

## Platformio.ini
You can quickly and easily add this library to your project in PlatformIO by simply including the following in your `platformio.ini` file:

```ini
lib_deps =
    flowduino/ESPressio-Observable@^1.0.0
```

Alternatively, if you want to use the bleeding-edge (effectively "Developer Integration Testing" or "DIT") sources, you can instead use:

```ini
lib_deps = 
    https://github.com/Flowduino/ESPressio-Observable.git
```
Please note that this will use the very latest commits pushed into the repository, so volatility is possible.

## RTTI is required for this library!
This library leverages fundamnetal C++ language features that in turn necessitate the use of RTTI (**R**un**T**ime **T**ype **I**nformation).

If you are developing with the Arduino framework but with the ESPressif platform, as of Febraury 22nd 2024, you may need to modify your Platformio.ini configuration as shown below to use a newer (pre-release) version of the packages where RTTI does not break any functionality when using `#include <FS.h>` in your code:
```ini
platform = https://github.com/platformio/platform-espressif32.git
platform_packages = framework-arduinoespressif32 @ https://github.com/espressif/arduino-esp32.git
```
>Note that we have been informed that the next major release of the platform will resolve this issue, eliminating this requirement. However, as of February 22nd 2024, the above lines included in your Platformio.ini file is required.

Additionally, you should always define the following in your Platformio.ini file's build configurations:
```ini
build_unflags =
	-fno-rtti
```
Where the above explicitly enables RTTI in your build configuration.

## Understanding Observer Pattern
Observer Pattern is a way of enabling any number of objects to be notified of specific operations or state changes occuring in another object.

Whereas *callbacks* are inherently limited to a single observer being notified of an operation or state change, Observer Pattern (particularly as implemented in this library) enables *any number* of Observers to be notified of the same.

Better still, this library provides Interface-Mapped Observer behaviour, meaning that you can define any number of Interfaces containing notification callback method declarations grouped however makes the most sense for your use-case.

In practice, Observer Pattern facilitates "one-way dependency" even when two objects need to communicate bi-directionally. This eliminates the unwanted complexity of "Circular Referencing", and improves the abstraction of code.

> Observer Pattern is one step removed from *total decoupling*, which can be achieved using our [ESPressio Event](https://github.com/Flowduino/ESPressio-Event) library: a fully-functional solution for true "Event-Driven Observer Pattern".

### The Do's and Don'ts of Observer Pattern:
#### The Do's:
* Always define logically-distinct Interfaces for Observers (demonstrated below in *Basic Usage*)
* Always ensure that your object references are one-way (I.E. `Observer` references `Observable` but `Observable` never references `Observer`)
* Try to keep `Observer` notification methods as short as reasonably possible, especially when working with multiple Threads. A notification should typically just update one or more state values.

#### The Don'ts:
* Never allow for circular references (`Observer` must know of `Observable`, but `Observable` must never know of `Observer`)
* Never modify any object or value passed into an `Observer` notification method! The value should remain unchanged for *all* `Observer`s receiving the same notification, therefore none should ever modify any state or value being passed in an `Observer` notification.

## Basic Usage
ESPressio Observable has been designed with ease of use in mind.

What we shall do now is define a simple use-case, and show how to implement it using the ESPressio Observable library.

For our use-case, let's presume that we have a class called `Thermometer`, whose sole purpose is to check for changes in temperature and notify any observers when a change occurs.

### The `Observable` Implementation...
> We shall presume taht the following code is in a header file called `Thermometer.hpp`
```cpp
#include <ESPressio_Observable>

using namespace ESPressio::Observable

class Thermometer : public Observable::Observable {
    private:
        int _temperature;

        void NotifyObservers(int oldTemperature, int newTemperature) {
            // Here is where we shall implement our Observer Notification
        }
    public:
        void UpdateTemperature() {
            // Code to read the sensor data will go here (and obviously depends on the sensor being used and method of reading the data)
            // Let's presume that the outcome is that we have an `int` variable called `temperature`
            if (_temperature == temperature) { return; } // If the temperature hasn't changed, there's nothing to do!
            // Now we know that the temperature has changed
            NotifyObservers(_temperature, newTemperature); // Notify the Observers of the change...
            _temperature = temperature; // ... then store the new Temperature value.
        }

        int GetTemperature() { return _temperature; }
};
```

The above implementation does not presume what kind of temperature sensor you are using, as that is not relevant to what we are covering in this usage example.

You will notice that there is a function named `NotifyObservers` that will be invoked every time the temperature changes.

In order for us to implement that function, we must first define an `Observer` type, which defines the *Interface* for our Observers.

> We shall presume that the following code is in a header file called `ITemperatureObserver.hpp`

```cpp
class ITemperatureObserver {
    public:
        virtual void OnTemperatureChanged(int oldTemperature, int newTemperature) {}
        virtual void OnTemperatureIncreased(int increasedBy) {}
        virtual void OnTemperatureDecreased(int decreasedBy) {}
};
```

The above defines an *Interface* describing the methods available for any Observer of `Thermometer`.

Pay special attention to the fact that each of these `virtual` methods has a stubbed (empty) implementation. This means that you can choose which of the avialable Observer Methods to implement on each respective Observer.
If you were to make them Abstract (e.g. `virtual void OnTemperatureChanged(int oldTemperature, int newTemperature) = 0;`) then you would be *forced* to provide an explicit implementation for that method on each Observer type.

Before we implement `NotifyObservers` we need to add the additional `include` for our file containing the `ITemperatureObserver` interface:
```cpp
#include <ESPressio_Observable.hpp>
#include "ITemperatureObserver.hpp" // < We add this line

using namespace ESPressio::Observable
```

Now we can implement the `NotifyObservers` method accordingly:
```cpp
        void NotifyObservers(int oldTemperature, int newTemperature) {
            WithObservers<ITemperatureObserver>([oldTemperature, newTemperature](ITemperatureObserver* observer) {
                observer->OnTemperatureChanged(oldTemperature, newTemperature);
                if (newTemperature > oldTemperature) { // Temperature has Increased
                    observer->OnTemperatureIncreased(newTemperature - oldTemperature);
                }
                else if (oldTemperature > newTemperature) { // Temperature has Decreased
                    observer->OnTemperatureDecreased(oldTemperature - newTemperature);
                }
            });
        }
```
The above implementation is performing a number of distinct processes.
* It iterates through all registered *Observers* implementing the `ITemperatureObserver` interface
  * For each of them...
    * it will invoke the `OnTemperatureChanged` method, passing both the previous Temperature and the new Temperature values.
    * If the temperature has *Increased*, it will invoke `OnTemperatureIncreased` with the temperature difference (delta) as its parameter value.
    * If the temperature has *Decreased*, it will invoke `OnTempeeratureDecreased` with the temperature difference (delta) as its parameter value.

So, from the *Observable* side of this exchange, the implementation is now complete.

### The `Observer` implementation...
Now that we have the `Observable` implemented, or - more specifically - given that the `ITemperatureObserver` interface has been defined, we can now implement the `Observer` side of this exchange.

> Remember: The `Observable` does not care whether any other objects are Observing it. An `Observable` will operate exactly the same way no matter what.

Now, let's define an `Observer` in a new header file called `TemperatureLogger.hpp` to do something very simple:
```cpp
#include <ESPressio_IObserver.hpp>
#include "ITemperatureObserver.hpp" // < Remember to include the file containing our interface!

using namespace ESPressio::Observable

class TemperatureLogger : public IObserver, public ITemperatureObserver {
    public:
        void OnTemperatureChanged(int oldTemperature, int newTemperature) override {
            Serial.printf("Temperature Changed from %d to %d.\n", oldTemperature, newTemperature);
        }

        void OnTemperatureIncreased(int increasedBy) override {
            Serial.printf("Temperature Increased by %d.\n", increasedBy);
        }

        void OnTemperatureDecreased(int decreasedBy) override {
            Serial.printf("Temperature Decreased by %d.\n", decreasedBy);
        }
};
```
The above implementation of `TemperatureLoger` simply prints lines to the Serial console each time the corresponding Notification Method is invoked.

> Your `Observer`s can of course perform any behaviour applicable to your use-cases, this example is intentionally simple.

So, now we have our `Observable` and we have a simple `Observer` type, we need to register our `Observer` with our `Observable` to complete the exchange.

So, our `ino` or `main.cpp` file will look something like this:

```cpp
#include <ESPressio_IObserver.hpp>
#include "TemperatureLogger.hpp"
#include "Thermometer.hpp"

Thermometer thermometer;
TemperatureLogger temperatureLogger;
IObserverHandle* observerHandle;

void setup() {
    observerHandle = thermometer.RegisterObserver(temperatureLogger); // Register our Observer with the Observable
}

void loop() {
    thermometer.UpdateTemperature(); // We will update the temperature reading on the loop
}
```
It really is as simple as that!

`RegisterObserver` registers the given `Observer` with the `Observable` against which it is invoked.
It returns an `IObserverHandle*` (pointer) reference which you should retain for the lifetime of your `Observer`. 

Invoking `delete observerHandle` will not only destroy the Observer Handle (freeing its memory), it will also unregister the `Observer` from the `Observable`.
In the case of the above (simple) example, both the `Observer` and the `Observable` exist for the entire lifetime of execution, however - you can fully control the lifetimes in your applications.

### Remember: You can have as many `Observer`s as you want for a single `Observable`!
As this section title says, you can register as many `Observer`s as you require for any `Observable` type.

### Thread-Safe `Observable`s
ESPressio Observable provides both a non-Thread-Safe and a Thread-Safe implementation.

The above example uses the non-Thread-Safe `Observable` implementation, but we can very easily substitute this with our `ThreadSafeObservable` counterpart:

> Modifying `Thermometer.hpp`

First, let's replace:
```cpp
    #include <ESPressio_Observable.hpp>
```
with:
```cpp
    #include <ESPressio_ThreadSafeObservable.hpp>
```

Now we simply modify the `Thermometer` class declaration from:
```cpp
class Thermometer : public Observable::Observable {
```
to:
```cpp
class Thermometer : public ThreadSafeObservable {
```
There is no need to modify any further implementation, as both `Observable` and `ThreadSafeObservable` identically satisfy the `IObservable` interface. Only their internal implementations differ (the latter appropriately employing thread-safe locks to ensure safe and predictable behaviour when operating across multiple Threads).

>**CRITICAL** Just because you're using `ThreadSafeObservable` instead of `Observable`, that does not mean that custom members of **your** `Observable` type (e.g. `_temperature` in `Thermometer`) are implicitly thread-safe!

>It is your responsibility to ensure that each member of your `Observable` type is appropriately encapsulated by the most suitable thread-safe locking mechanism for your use-case.

For broader Thread-Safe implementation needs, you should check out our [ESPressio Threads](https://github.com/Flowduino/ESPressio-Threads) library, which provides a simple and elegant means of making members of your own classes thread-safe.

### One `Observable`, multiple `Observer` types
The previous examples have all illustrated how to implement a singular `Observer` type (`ITemperatureObserver`) for a singular `Observable` type (`Thermometer`).

However, ESPressio Observable's clever implementation enables you to define and consume any number of `Observer` types for a singular `Observable` type.

This functionality is extremely useful, as it enables us to group related `Observer` notifications into discrete *Interfaces*, strengthing our adherence to the SOLID principles of software development; yielding cleaner, more maintainable code.

To expand on our previous example with the `Thermometer` `Observable` type, let us presume that our physical sensor provides Air Pressure in addition to Temperature. With that in mind, let's extend and modify our previous implementation to enable individual `Observer`s to be notified of each respective sensor value change.

We shall begin by adding a new file called `IAirPressureObserver.hpp`:
```cpp
class IAirPressureObserver {
    public:
        virtual void OnAirPressureChanged(int oldPressure, int newPressure) {}
        virtual void OnAirPressureIncreased(int increasedBy) {}
        virtual void OnAirPressureDecreased(int decreasedBy) {}
};
```
The above code simply defines a new interface specific for Air Pressure events, and entirely separate from our previous Temperature events interface.

Now we need to modify the `Observable` (`Thermometer`) to notify any `Observer` implementing `IAirPressureObserver`, so let's modify our `Thermometer.hpp` file accordingly:
```cpp
#include <ESPressio_Observable>
#include "ITemperatureObserver.hpp"
#include "IAirPressureObserver.hpp" // < We add the include for the new Interface

using namespace ESPressio::Observable

class Thermometer : public Observable::Observable {
    private:
        int _temperature;
        int _airPressure;

        // We will rename `NotifyObservers` to `NotifyTemperatureObservers` to avoid ambiguity
        void NotifyTemperatureObservers(int oldTemperature, int newTemperature) {
            WithObservers<ITemperatureObserver>([oldTemperature, newTemperature](ITemperatureObserver* observer) {
                observer->OnTemperatureChanged(oldTemperature, newTemperature);
                if (newTemperature > oldTemperature) { // Temperature has Increased
                    observer->OnTemperatureIncreased(newTemperature - oldTemperature);
                }
                else if (oldTemperature > newTemperature) { // Temperature has Decreased
                    observer->OnTemperatureDecreased(oldTemperature - newTemperature);
                }
            });
        }

        // We will add `NotifyAirPressureObservers` per our expansion
        void NotifyAirPressureObservers(int oldPressure, int newPressure) {
            WithObservers<IAirPressureObserver>([oldPressure, newPressure](IAirPressureObserver* observer) {
                observer->OnAirPressureChanged(oldPressure, newPressure);
                if (newPressure > oldPressure) { // Air Pressure has Increased
                    observer->OnAirPressureIncreased(newPressure - oldPressure);
                }
                else if (oldPressure > newPressure) { // Air Pressure has Decreased
                    observer->OnAirPressureDecreased(oldPressure - newPressure);
                }
            });
        }
    public:
        void UpdateTemperature() {
            // Code to read the sensor data will go here (and obviously depends on the sensor being used and method of reading the data)
            // Let's presume that the outcome is that we have an `int` variable called `temperature`
            if (_temperature == temperature) { return; } // If the temperature hasn't changed, there's nothing to do!
            // Now we know that the temperature has changed
            NotifyTemperatureObservers(_temperature, newTemperature); // Notify the Observers of the change...
            _temperature = temperature; // ... then store the new Temperature value.
        }

        void UpdateAirPressure() {
            // Code to read the Air Pressure data will go here (different depending on sensor type being used)
            // Let's presume that the outcome is that we have an `int` variable called `airPressure`
            if (_airPressure == airPressure) { return; } // If the temperature hasn't changed, there's nothing to do!
            // Now we know that the temperature has changed
            NotifyAirPressureObservers(_airPressure, airPressure); // Notify the Observers of the change...
            _airPressure = airPressure; // ... then store the new Temperature value.
        }

        int GetTemperature() { return _temperature; }
        int GetAirPressure() { return _airPressure; }
};
```
Above is the completed expansion of our `Thermometer.hpp` header file, expanding on Temperature with the new Air Pressure implementation.

The important thing to note here is that we define which *Interface* to use in the call to `WithObservers`, and the implementation behind the scenes will ensure that the given *Lambda Function* is only ever invoked where an `Observer` satisfies (implements) that given *Interface* type.

This is a crucial feature of ESPressio Observable, and one which sets it apart from more rudimentary (and common) Obsever Pattern implementations, which are typically inflexible and enforce a singular (fixed) `Observer` interface.

So, with the `Observable` (`Thermometer`) now expanded to accept both `ITemperatureObserver` and `IAirPressureObserver` implementing `Observer`s, we can introduce a new `Observer` type specifically for `IAirPressureObserver`...

Now, let's define a new `Observer` in a new header file called `AirPressureLogger.hpp` to do something very simple:
```cpp
#include <ESPressio_IObserver.hpp>
#include "IAirPressureObserver.hpp" // < Remember to include the file containing our interface!

using namespace ESPressio::Observable

class AirPressureLogger : public IObserver, public IAirPressureObserver {
    public:
        void OnAirPressureChanged(int oldPressure, int newPressure) override {
            Serial.printf("Air Pressure Changed from %d to %d.\n", oldPressure, newPressure);
        }

        void OnAirPressureIncreased(int increasedBy) override {
            Serial.printf("Air Pressure Increased by %d.\n", increasedBy);
        }

        void OnAirPressureDecreased(int decreasedBy) override {
            Serial.printf("Air Pressure Decreased by %d.\n", decreasedBy);
        }
};
```

Just like that, we have another (very simple) `Observer` type specifically interested in Air Pressure specific notifications, which will simply output these changes into the Serial console (just as we did with the `TemperatureLogger` implementation prevously)

Again, remember that your `Observer` implementation can perform whatever operations are relevant to your use-case for each Notificiation Function Implementation. This example is intentionally simplified.

Okay, we're almost done... now we need only modify our `.ino` or `main.cpp` implementation to instantiate and register our new `Observer` (`AirPressureLogger`) with our `Observable` (`Thermometer`):
```cpp
#include <ESPressio_IObserver.hpp>
#include "TemperatureLogger.hpp"
#include "AirPressureLogger.hpp" // We add the new Include for our new Air Pressure Logger class
#include "Thermometer.hpp"

Thermometer thermometer;
TemperatureLogger temperatureLogger;
AirPressureLogger airPressureLogger;
IObserverHandle* temperatureObserverHandle; // We shall rename `observerHandle` to `temperatureObserverHandle` to avoid ambiguity.
IObserverHandle* airPressureObserverHandle; // We add a new variable to hold the Air Pressure Observer's Handle.

void setup() {
    temperatureObserverHandle = thermometer.RegisterObserver(temperatureLogger); // Register our Temperature Observer with the Observable
    airPressureObserverHandle = thermometer.RegisterObserver(airPressureLogger); // Register our Air PRessure Observer with the Observable
}

void loop() {
    thermometer.UpdateTemperature(); // We will update the temperature reading on the loop
    thermometer.UpdateAirPressure(); // We will update the Air Pressure reading on the loop too.
}
```
And that's all there is to it!

We now have a complete example of how a single `Observable` type can notify different `Observer` types using specific *Interfaces*.

But, there's one last feature you need to know about!

### A single `Observer` can implement any number of *Observer Interfaces*!
As the title suggests, we can also define a single `Observer` type that implements any number of *Observer Interfaces*.

This is useful where your use-case requires that certain discrete objects in your code need to be notified of operations and state changes from multiple different (context-specific) objects elsewhere within your code.

As one example, let's presume we're programming a device which has a screen (a display module), and at any given moment it needs to display the Temperature, the Air Pressure, and perhaps even the Battery Level of your device.

We won't go into a complete working example (because there are a vast number of display modules out there, each with different SDKs and APIs) but we can at least define a *stub* for example:
```cpp
#include <ESPressio_IObserver.hpp>
#include "ITemperatureObserver.hpp"
#include "IAirPressureObserver.hpp"
#include "IBatteryObserver.hpp" // Hypothetical Battery Observer interface

using namespace ESPressio::Observable

class MyDisplay : public IObserver, public ITemperatureObserver, public IAirPressureObserver, public IBatteryObserver {
    private:
        float _currentBatteryPercent;
        bool _isBatteryCharging;
        int _currentTemperature;
        int _lastTemperatureChange;
        int _currentAirPressure;
        int _lastAirPressureChange;

        void Redraw() {
            // Draw the Battery Level from _currentBatteryPercent on the screen
            // Draw the Battery Charging Indicator based on _isBatteryCharging on the screen
            // Draw the Current Temperature from _currentTemperature on the screen
            // Draw the Last Temperature Change By from _lastTemperatureChange on the screen
            // Draw the Current Air Pressure from _currentAirPressure on the screen
            // Draw the Last Air Pressure Change By from _lastAirPressureChange on the screen
        }
    public:
    // ITemperatureObserver implementations

        void OnTemperatureChanged(int oldTemperature, int newTemperature) override {
            _currentTemperature = newTemperature;
            Redraw();
        }

        void OnTemperatureIncreased(int increasedBy) override {
            _lastTemperatureChange = increasedBy;
            Redraw();
        }

        void OnTemperatureDecreased(int decreasedBy) override {
            _lastTemperatureChange = -decreasedBy // Gives us a negative number to render
            Redraw();
        }

    // IAirPressureObserver implementations

        void OnAirPressureChanged(int oldPressure, int newPressure) override {
            _currentAirPressure = newPressure;
            Redraw();
        }

        void OnAirPressureIncreased(int increasedBy) override {
            _lastAirPressureChange = increasedBy;
            Redraw();
        }

        void OnAirPressureDecreased(int decreasedBy) override {
            _lastAirPressureChange = -decreasedBy; // Gives us a negative number to render
            Redraw();
        }

    // IBatteryObserver implementations

        void OnBatteryLevelChanged(float oldPercent, float newPercent) override {
            _currentBatteryPercent = newPercent;
            Redraw();
        }

        void OnBatteryIsCharging() override {
            _isBatteryCharging = true;
            Redraw();
        }

        void OnBatteryIsDischarging() override {
            _isBatteryCharging = false;
            Redraw();
        }
};
```
The above `Observer` implementation will be notified of changes for all of:
* Temperature
* Air Pressure
* Battery State

Presuming an appropriate implementation in `Redraw()`, you will then display the latest data in response to any state change involving the aforementioned (Observed) state changes.

So, as you can see, a single `Observer` can implement as many `Observer` *Interfaces* as it requires to satisfy its distinct functional requirements.

This illustrates the powerful capabilities of ESPressio Observable, which we are absolutely certain will become an invaluable tool for developers intent on producing highly-maintainable, clean, and scalable software and firmware.