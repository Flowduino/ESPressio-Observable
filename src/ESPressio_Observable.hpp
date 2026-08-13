#pragma once

#include <functional>
#include <memory>
#include <utility>
#include <vector>
#include <algorithm>

#include "ESPressio_IObservable.hpp"
#include "ESPressio_IObserver.hpp"
#include "ESPressio_ObserverHandle.hpp"

namespace ESPressio {

    namespace Observable {
   
        /// An `Observable` is an object that can be observed by any number of `IObserver` descendant types
        /// This is a concrete implementation of `IObservable`.
        /// THIS TYPE IS NOT THREAD-SAFE!
        /// Observers may register or unregister during a callback, but calls
        /// from multiple threads still require external synchronization.
        /// If you need a Thread-Safe Implementation, use the `ThreadSafeObservable` class instead.
        class Observable : public IUntypedObservable {
            private:
                std::vector<IObserverHandle*> _observers;
                std::size_t _notificationDepth = 0;
                bool _needsCompaction = false;

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
                    ++_notificationDepth;
                    const std::size_t observerCount = _observers.size();
                    try {
                        for (std::size_t index = 0; index < observerCount; ++index) {
                            IObserverHandle* handle = _observers[index];
                            if (handle != nullptr) { callback(handle->GetObserver()); }
                        }
                    } catch (...) {
                        _finishNotification();
                        throw;
                    }
                    _finishNotification();
                }

                template <class ObserverType, class Callback>
                void _withObservers(Callback&& callback) {
                    ++_notificationDepth;
                    const std::size_t observerCount = _observers.size();
                    try {
                        for (std::size_t index = 0; index < observerCount; ++index) {
                            IObserverHandle* handle = _observers[index];
                            if (handle == nullptr) { continue; }
                            ObserverType* observerAsT = dynamic_cast<ObserverType*>(handle->GetObserver());
                            if (observerAsT != nullptr) { callback(observerAsT); }
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
                        friend class Observable;
                        Observable& _observable;
                        std::shared_ptr<IObservable> _notificationLifetime;
                        NotificationContext(
                            Observable& observable,
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

                /// Executes the complete notification operation while retaining this
                /// Observable. All callback dispatch and subsequent member access must
                /// occur inside operation.
                template <class Operation>
                void ExecuteNotification(Operation&& operation) {
                    NotificationContext context(
                        *this, AcquireNotificationLifetime());
                    operation(context);
                }
            public:
                ~Observable() override {
                    BeginObservableDestruction();
                    for (IObserverHandle* handle : _observers) {
                        if (handle != nullptr) {
                            static_cast<ObserverHandle*>(handle)->InvalidateRegistration();
                        }
                    }
                    _observers.clear();
                }

                ObserverHandlePtr RegisterObserver(IObserver* observer) override {
                    if (observer == nullptr) {
                        throw InvalidObserverRegistrationException();
                    }
                    for (auto thisObserver : _observers) {
                        if (thisObserver != nullptr && thisObserver->GetObserver() == observer) {
                            throw DuplicateObserverRegistrationException();
                        }
                    }
                    std::unique_ptr<ObserverHandle> handle(
                        new ObserverHandle(GetLifetimeControl(), observer));
                    _observers.push_back(handle.get());
                    return ObserverHandlePtr(handle.release());
                }

                void UnregisterObserver(IObserver* observer) override {
                    for (auto thisObserver = _observers.begin(); thisObserver != _observers.end(); thisObserver++) {
                        if ((*thisObserver)->GetObserver() == observer) {
                            static_cast<ObserverHandle*>((*thisObserver))->InvalidateRegistration();
                            if (_notificationDepth > 0) {
                                *thisObserver = nullptr;
                                _needsCompaction = true;
                            } else {
                                _observers.erase(thisObserver);
                            }
                            return;
                        }
                    }
                }

                bool IsObserverRegistered(IObserver* observer) override {
                    for (auto thisObserver : _observers) {
                        if (thisObserver->GetObserver() == observer) { return true; }
                    }
                    return false;
                }
        };

    }

}
