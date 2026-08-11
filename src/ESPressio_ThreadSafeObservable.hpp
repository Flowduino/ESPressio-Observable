#pragma once

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

                bool _isObserverRegistered(IObserver* observer) const {
                    for (IObserverHandle* handle : _observers) {
                        if (handle->GetObserver() == observer) {
                            return true;
                        }
                    }
                    return false;
                }

                std::vector<IObserver*> _copyObserverPointers() const {
                    std::vector<IObserver*> observers;
                    observers.reserve(_observers.size());

                    for (IObserverHandle* handle : _observers) {
                        observers.push_back(handle->GetObserver());
                    }

                    return observers;
                }

                void _withObservers(std::function<void(IObserver*)> callback) {
                    std::lock_guard<std::recursive_mutex> lock(_mutex);
                    const std::vector<IObserver*> observers =
                        _copyObserverPointers();

                    for (IObserver* observer : observers) {
                        if (_isObserverRegistered(observer)) {
                            callback(observer);
                        }
                    }
                }

                template <class ObserverType>
                void _withObservers(std::function<void(ObserverType*)> callback) {
                    std::lock_guard<std::recursive_mutex> lock(_mutex);
                    const std::vector<IObserver*> observers =
                        _copyObserverPointers();

                    for (IObserver* observer : observers) {
                        if (!_isObserverRegistered(observer)) { continue; }
                        ObserverType* observerAsT =
                            dynamic_cast<ObserverType*>(observer);
                        if (!observerAsT) { continue; }
                        callback(observerAsT);
                    }
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
                        void WithObservers(std::function<void(IObserver*)> callback) {
                            _observable._withObservers(std::move(callback));
                        }

                        template <class ObserverType>
                        void WithObservers(std::function<void(ObserverType*)> callback) {
                            _observable._withObservers<ObserverType>(std::move(callback));
                        }
                };

                template <class Operation>
                void ExecuteNotification(Operation&& operation) {
                    NotificationContext context(
                        *this, AcquireNotificationLifetime());
                    operation(context);
                }
            public:
                ~ThreadSafeObservable() override {
                    BeginObservableDestruction();
                    std::lock_guard<std::recursive_mutex> lock(_mutex);
                    _observers.clear();
                }

                IObserverHandle* RegisterObserver(IObserver* observer) override {
                    if (observer == nullptr) {
                        throw InvalidObserverRegistrationException();
                    }
                    std::lock_guard<std::recursive_mutex> lock(_mutex);
                    for (auto thisObserver : _observers) {
                        if (thisObserver->GetObserver() == observer) {
                            return thisObserver;
                        }
                    }
                    std::unique_ptr<ObserverHandle> handle(
                        new ObserverHandle(GetLifetimeControl(), observer));
                    ObserverHandle* result = handle.get();
                    _observers.push_back(result);
                    handle.release();
                    return result;
                }

                void UnregisterObserver(IObserver* observer) override {
                    std::lock_guard<std::recursive_mutex> lock(_mutex);
                    for (auto thisObserver = _observers.begin(); thisObserver != _observers.end(); thisObserver++) {
                        if ((*thisObserver)->GetObserver() == observer) {
                            static_cast<ObserverHandle*>((*thisObserver))->InvalidateRegistration();
                            _observers.erase(thisObserver);
                            return;
                        }
                    }
                }

                bool IsObserverRegistered(IObserver* observer) override {
                    std::lock_guard<std::recursive_mutex> lock(_mutex);
                    return _isObserverRegistered(observer);
                }
        };

    }

}
