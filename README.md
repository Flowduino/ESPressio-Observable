# ESPressio Observable

Synchronous Observer Pattern components for the Flowduino ESPressio
Development Platform.

## Latest Stable Version

**3.0.0**

## ESPressio Development Platform

ESPressio is a collection of discrete, composable component libraries
built around a common development ethos:

-   **Light-weight** --- minimise memory consumption and runtime
    overhead without sacrificing correctness.
-   **Ease of use** --- provide strongly typed, developer-friendly
    abstractions over lower-level facilities.
-   **Object-oriented** --- a type for everything, and everything in a
    type.
-   **SOLID** --- favour focused responsibilities, extensibility,
    substitutable abstractions, narrow interfaces, and dependency
    inversion wherever practical on embedded C++ platforms.

## License

Licensed under the **Apache License 2.0**. See [LICENSE](LICENSE).

## ESPressio Library Dependencies

ESPressio is designed as a modular ecosystem of independently useful
libraries, with required dependencies kept explicit and optional
integrations introduced only when the corresponding functionality is
selected.

For a complete overview of required dependencies, opt-in dependencies,
and the overall hierarchy, see:

**[ESPressio Library Dependency Chart](ESPRESSIO_DEPENDENCY_CHART.md)**

-   **Solid relationships** represent required ESPressio dependencies.
-   **Dashed relationships** represent opt-in dependencies introduced
    only by the corresponding feature, integration, type, or header.

### Required ESPressio dependencies

None.

## Namespace

``` cpp
ESPressio::Observable
```

## Observer Pattern

Observable is intended for synchronous relationships between an
operation and interested observers:

``` text
Observable
   +--> Observer A
   +--> Observer B
   +--> Observer C
```

A subsystem normally defines a focused Observer interface derived from
the library's `IObserver` contract and exposes registration through its
Observable surface.

## Registration lifetime

Version 3.x uses ownership-safe registration handles. Registrations can
be explicitly removed and naturally follow the lifetime of their
registration handles.

Observers remain non-owning: the application must keep an Observer alive
for as long as it remains registered.

## Synchronous semantics

Observer callbacks execute synchronously as part of the notifying
operation.

Use Observable when the notification belongs directly to the operation
and an asynchronous task boundary is undesirable.

Use ESPressio Event when producers and consumers should be independently
scheduled.

## Relationship with ESPressio Event

Observable does **not** depend on Event.

Higher-level bridges may consume synchronous Observer callbacks and emit
asynchronous Events:

``` text
Timing / Threads
    -> Observable callback
    -> optional Event bridge
    -> asynchronous Event
```

This dependency direction keeps foundational libraries independent.

## Design goals

-   Focused Observer contracts.
-   Explicit registration lifetime.
-   Non-owning Observer relationships.
-   Synchronous deterministic notification.
-   Reusable infrastructure.
-   No dependency on ESPressio Event.
