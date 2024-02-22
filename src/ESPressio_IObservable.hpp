#pragma once

#include <functional>
#include <memory>

namespace ESPressio {

    namespace Observable {
        class IObservable;

        /// An `IObserverHandle` is returned when registering your Observer with an `IObservable`
        /// It is used to not only check if the `IObservable` still exists, but also to unregister the Observer when desired!
        class IObserverHandle {
            public:
                /// Will Unregister this Observer from the `IObservable` if it still exists
                virtual void Unregister() = 0;
                /// Will return a `weak_ptr` to the `IObservable`
                virtual std::weak_ptr<IObservable> GetObservable() = 0;
        };
    
        class IObservable : public std::enable_shared_from_this<IObservable> {
            public:
                /// Will Register the `void*` (Pointer to an Observer Object) with this `IObservable`
                virtual void RegisterObserver(void* observer) = 0;
                /// Will Unregister the `void*` (Pointer to an Observer Object) from this `IObservable`
                virtual void UnregisterObserver(void* observer) = 0;
                /// Will return `true` if the `void*` (Pointer to an Observer Object) is registered with this `IObservable`
                virtual bool IsObserverRegistered(void* observer) = 0;
        };

    }

}