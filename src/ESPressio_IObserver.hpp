#pragma once

namespace ESPressio {

    namespace Observable {

        /// An `IObserver` is an object that can observe an `IObservable`
        /// You MUST inherit from this type for ANY object from which you intend to Observe any `IObservable` descendant types
        /// This is required to satisfy the `dynamic_cast` requirements of the `Observable` class, and cannot be avoided due to C++ language limitations.
        class IObserver {
            public:
                virtual ~IObserver() = default;
        };

    }

}