#pragma once

#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "ESPressio_IObservable.hpp"
#include "ESPressio_IObserver.hpp"
#include "ESPressio_ObserverHandle.hpp"

namespace ESPressio {

    namespace Observable {
   
        /// An `Observable` is an object that can be observed by any number of `IObserver` descendant types
        /// This is a concrete implementation of `IObservable`.
        /// THIS TYPE IS NOT THREAD-SAFE!
        /// Registering or Unregistering Observers while Observers are being notified can lead to undefined behavior.
        /// If you need a Thread-Safe Implementation, use the `ThreadSafeObservable` class instead.
        class Observable : public IUntypedObservable {
            private:
                std::vector<IObserverHandle*> _observers;

                void _withObservers(std::function<void(IObserver*)> callback) {
                    for (auto observer : _observers) {
                        callback(observer->GetObserver());
                    }
                }

                template <class ObserverType>
                void _withObservers(std::function<void(ObserverType*)> callback) {
                    for (auto observer : _observers) {
                        ObserverType* observerAsT = dynamic_cast<ObserverType*>(observer->GetObserver());
                        if (!observerAsT) { continue; }
                        callback(observerAsT);
                    }
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
                        void WithObservers(std::function<void(IObserver*)> callback) {
                            _observable._withObservers(std::move(callback));
                        }

                        template <class ObserverType>
                        void WithObservers(std::function<void(ObserverType*)> callback) {
                            _observable._withObservers<ObserverType>(std::move(callback));
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
                    _observers.clear();
                }

                IObserverHandle* RegisterObserver(IObserver* observer) override {
                    if (observer == nullptr) {
                        throw InvalidObserverRegistrationException();
                    }
                    for (auto thisObserver : _observers) {
                        if (thisObserver->GetObserver() == observer) { return thisObserver; }
                    }
                    std::unique_ptr<ObserverHandle> handle(
                        new ObserverHandle(GetLifetimeControl(), observer));
                    ObserverHandle* result = handle.get();
                    _observers.push_back(result);
                    handle.release();
                    return result;
                }

                void UnregisterObserver(IObserver* observer) override {
                    for (auto thisObserver = _observers.begin(); thisObserver != _observers.end(); thisObserver++) {
                        if ((*thisObserver)->GetObserver() == observer) {
                            static_cast<ObserverHandle*>((*thisObserver))->InvalidateRegistration();
                            _observers.erase(thisObserver);
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
