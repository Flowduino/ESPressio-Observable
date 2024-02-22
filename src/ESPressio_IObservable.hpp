#pragma once

#include <functional>
#include <memory>

#include "ESPressio_IObserver.hpp"

namespace ESPressio {

    namespace Observable {
        class IObservable;

        /// An `IObserverHandle` is returned when registering your Observer with an `IObservable`
        /// It is used to not only check if the `IObservable` still exists, but also to unregister the Observer when desired!
        class IObserverHandle {
            public:
                /// Will null the `IObservable` reference to inform that the Observable is no longer valid
                /// Do not call this method directly, it is called by the `IObservable` when it is destroyed
                virtual void __invalidate() = 0;
                /// Will Unregister this Observer from the `IObservable` if it still exists
                virtual void Unregister() = 0;
                /// Will return a `weak_ptr` to the `IObservable`
                virtual IObservable* GetObservable() = 0;
                /// Will return a pointer to the `IObserver`
                virtual IObserver* GetObserver() = 0;
        };
    
        class IObservable : public std::enable_shared_from_this<IObservable> {
            public:
                /// Will Register the`IObserver` with this `IObservable`
                virtual IObserverHandle* RegisterObserver(IObserver* observer) = 0;
                /// Will Unregister the `IObserver` from this `IObservable`
                virtual void UnregisterObserver(IObserver* observer) = 0;
                /// Will return `true` if the `IObserver` is registered with this `IObservable`
                virtual bool IsObserverRegistered(IObserver* observer) = 0;
        };

    }

}