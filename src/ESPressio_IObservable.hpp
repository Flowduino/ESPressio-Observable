#pragma once

#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <stdexcept>

#include "ESPressio_IObserver.hpp"

namespace ESPressio {

    namespace Observable {
        class IObservable;
        class ObserverHandle;

        class ObservableException : public std::runtime_error {
            public:
                using std::runtime_error::runtime_error;
        };

        class ObserverRegistrationException : public ObservableException {
            public:
                using ObservableException::ObservableException;
        };

        class InvalidObserverRegistrationException : public ObserverRegistrationException {
            public:
                InvalidObserverRegistrationException()
                    : ObserverRegistrationException("Cannot register a null Observer pointer") {}
        };

        namespace Detail {
            class ObservableLifetimeControl {
                private:
                    mutable std::mutex _mutex;
                    std::condition_variable _condition;
                    IObservable* _observable;
                    std::size_t _activeOperations = 0;
                    bool _alive = true;

                public:
                    explicit ObservableLifetimeControl(IObservable* observable)
                        : _observable(observable) {}

                    IObservable* Acquire() {
                        std::lock_guard<std::mutex> lock(_mutex);
                        if (!_alive) { return nullptr; }
                        ++_activeOperations;
                        return _observable;
                    }

                    void Release() {
                        std::lock_guard<std::mutex> lock(_mutex);
                        if (--_activeOperations == 0) {
                            _condition.notify_all();
                        }
                    }

                    IObservable* Peek() const {
                        std::lock_guard<std::mutex> lock(_mutex);
                        return _alive ? _observable : nullptr;
                    }

                    void InvalidateAndWait() {
                        std::unique_lock<std::mutex> lock(_mutex);
                        _alive = false;
                        _observable = nullptr;
                        _condition.wait(lock, [this]() {
                            return _activeOperations == 0;
                        });
                    }
            };
        }

        /// An `IObserverHandle` is returned when registering your Observer with an `IObservable`
        /// It is used to not only check if the `IObservable` still exists, but also to unregister the Observer when desired!
        class IObserverHandle {
            public:
                IObserverHandle() = default;
                IObserverHandle(const IObserverHandle&) = delete;
                IObserverHandle& operator=(const IObserverHandle&) = delete;
                IObserverHandle(IObserverHandle&&) = delete;
                IObserverHandle& operator=(IObserverHandle&&) = delete;
                virtual ~IObserverHandle() = default;
                /// Will Unregister this Observer from the `IObservable` if it still exists
                virtual void Unregister() = 0;
                /// Returns the associated `IObservable`, or nullptr once it has begun destruction.
                /// The returned pointer is non-owning and must not be retained.
                virtual IObservable* GetObservable() = 0;
                /// Will return a pointer to the `IObserver`
                virtual IObserver* GetObserver() = 0;
        };
    
        /// An `IObservable` is an object that can be observed by any number of `IObserver` descendant types
        class IObservable {
            private:
                friend class ObserverHandle;
                std::shared_ptr<Detail::ObservableLifetimeControl> _lifetimeControl;

            protected:
                std::shared_ptr<Detail::ObservableLifetimeControl> GetLifetimeControl() const {
                    return _lifetimeControl;
                }

                /// Derived destructors must invoke this before destroying state used by
                /// RegisterObserver(), UnregisterObserver(), or IsObserverRegistered().
                void BeginObservableDestruction() noexcept {
                    _lifetimeControl->InvalidateAndWait();
                }

            public:
                IObservable()
                    : _lifetimeControl(
                        std::make_shared<Detail::ObservableLifetimeControl>(this)) {}
                IObservable(const IObservable&) = delete;
                IObservable& operator=(const IObservable&) = delete;
                IObservable(IObservable&&) = delete;
                IObservable& operator=(IObservable&&) = delete;
                virtual ~IObservable() {
                    BeginObservableDestruction();
                }
                /// Will Register the`IObserver` with this `IObservable`
                virtual IObserverHandle* RegisterObserver(IObserver* observer) = 0;
                /// Will Unregister the `IObserver` from this `IObservable`
                virtual void UnregisterObserver(IObserver* observer) = 0;
                /// Will return `true` if the `IObserver` is registered with this `IObservable`
                virtual bool IsObserverRegistered(IObserver* observer) = 0;
        };

    }

}
