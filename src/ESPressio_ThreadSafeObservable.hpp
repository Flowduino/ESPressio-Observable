#pragma once

#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "ESPressio_IObservable.hpp"
#include "ESPressio_IObserver.hpp"
#include "ESPressio_ObserverHandle.hpp"

namespace ESPressio {

    namespace Observable {
   
        /// A `ThreadSafeObservable` is an object that can be observed by any number of `IObserver` descendant types
        /// This is a concrete implementation of `IObservable`, and is Thread Safe!
        /// Your Observers can Register or Unregister themselves at any time, and the `ThreadSafeObservable` will handle it!
        class ThreadSafeObservable : public IUntypedObservable {
            private:
                std::vector<IObserverHandle*> _observers;
                std::recursive_mutex _mutex;
                std::atomic<std::size_t> _observerCount{0};
                std::size_t _notificationDepth = 0;
                bool _needsCompaction = false;

                bool _isObserverRegistered(IObserver* observer) const {
                    for (IObserverHandle* handle : _observers) {
                        if (handle != nullptr && handle->GetObserver() == observer) {
                            return true;
                        }
                    }
                    return false;
                }

                void _finishNotification() {
                    if (--_notificationDepth == 0 && _needsCompaction) {
                        _observers.erase(
                            std::remove(_observers.begin(), _observers.end(), nullptr),
                            _observers.end());
                        _needsCompaction = false;
                    }
                }

                template <class Callback>
                void _withObservers(Callback&& callback) {
                    std::lock_guard<std::recursive_mutex> lock(_mutex);
                    ++_notificationDepth;
                    const std::size_t observerCount = _observers.size();
                    try {
                        for (std::size_t index = 0; index < observerCount; ++index) {
                            IObserverHandle* handle = _observers[index];
                            if (handle != nullptr) {
                                callback(handle->GetObserver());
                            }
                        }
                    } catch (...) {
                        _finishNotification();
                        throw;
                    }
                    _finishNotification();
                }

                template <class ObserverType, class Callback>
                void _withObservers(Callback&& callback) {
                    std::lock_guard<std::recursive_mutex> lock(_mutex);
                    ++_notificationDepth;
                    const std::size_t observerCount = _observers.size();
                    try {
                        for (std::size_t index = 0; index < observerCount; ++index) {
                            IObserverHandle* handle = _observers[index];
                            if (handle == nullptr) {
                                continue;
                            }
                            ObserverType* observerAsT =
                                dynamic_cast<ObserverType*>(handle->GetObserver());
                            if (observerAsT != nullptr) {
                                callback(observerAsT);
                            }
                        }
                    } catch (...) {
                        _finishNotification();
                        throw;
                    }
                    _finishNotification();
                }

            protected:
                class NotificationContext {
                    private:
                        friend class ThreadSafeObservable;
                        ThreadSafeObservable& _observable;
                        std::shared_ptr<IObservable> _notificationLifetime;
                        NotificationContext(
                            ThreadSafeObservable& observable,
                            std::shared_ptr<IObservable> notificationLifetime)
                            : _observable(observable),
                              _notificationLifetime(std::move(notificationLifetime)) {}

                    public:
                        template <class Callback>
                        void WithObservers(Callback&& callback) {
                            _observable._withObservers(
                                std::forward<Callback>(callback));
                        }

                        template <class ObserverType, class Callback>
                        void WithObservers(Callback&& callback) {
                            _observable._withObservers<ObserverType>(
                                std::forward<Callback>(callback));
                        }
                };

                template <class Operation>
                void ExecuteNotification(Operation&& operation) {
                    /*
                     * Notifications are intentionally very cheap when no observers
                     * are registered. The relaxed/acquire atomic read avoids taking
                     * the recursive mutex and avoids acquiring a notification-lifetime
                     * shared_ptr on the overwhelmingly common production fast path.
                     *
                     * A concurrently registering observer is not required to observe
                     * a notification that had already begun before registration.
                     */
                    if (
                        _observerCount.load(
                            std::memory_order_acquire
                        ) == 0
                    ) {
                        return;
                    }

                    NotificationContext context(
                        *this,
                        AcquireNotificationLifetime());
                    operation(context);
                }

            public:
                ~ThreadSafeObservable() override {
                    BeginObservableDestruction();
                    std::lock_guard<std::recursive_mutex> lock(_mutex);
                    for (IObserverHandle* handle : _observers) {
                        if (handle != nullptr) {
                            static_cast<ObserverHandle*>(handle)->InvalidateRegistration();
                        }
                    }
                    _observers.clear();
                    _observerCount.store(0, std::memory_order_release);
                }

                ObserverHandlePtr RegisterObserver(IObserver* observer) override {
                    if (observer == nullptr) {
                        throw InvalidObserverRegistrationException();
                    }
                    std::lock_guard<std::recursive_mutex> lock(_mutex);
                    for (auto thisObserver : _observers) {
                        if (thisObserver != nullptr &&
                            thisObserver->GetObserver() == observer) {
                            throw DuplicateObserverRegistrationException();
                        }
                    }
                    std::unique_ptr<ObserverHandle> handle(
                        new ObserverHandle(GetLifetimeControl(), observer));
                    _observers.push_back(handle.get());
                    _observerCount.fetch_add(1, std::memory_order_release);
                    return ObserverHandlePtr(handle.release());
                }

                void UnregisterObserver(IObserver* observer) override {
                    std::lock_guard<std::recursive_mutex> lock(_mutex);
                    for (
                        auto thisObserver = _observers.begin();
                        thisObserver != _observers.end();
                        ++thisObserver
                    ) {
                        if (
                            *thisObserver == nullptr ||
                            (*thisObserver)->GetObserver() != observer
                        ) {
                            continue;
                        }

                        static_cast<ObserverHandle*>(
                            *thisObserver
                        )->InvalidateRegistration();

                        _observerCount.fetch_sub(
                            1,
                            std::memory_order_acq_rel
                        );

                        if (_notificationDepth > 0) {
                            *thisObserver = nullptr;
                            _needsCompaction = true;
                        } else {
                            _observers.erase(thisObserver);
                        }
                        return;
                    }
                }

                bool IsObserverRegistered(IObserver* observer) override {
                    if (
                        _observerCount.load(
                            std::memory_order_acquire
                        ) == 0
                    ) {
                        return false;
                    }

                    std::lock_guard<std::recursive_mutex> lock(_mutex);
                    return _isObserverRegistered(observer);
                }
        };

    }

}
