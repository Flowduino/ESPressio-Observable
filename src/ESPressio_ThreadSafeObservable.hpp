#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "ESPressio_IObservable.hpp"
#include "ESPressio_IObserver.hpp"
#include "ESPressio_ObserverHandle.hpp"

namespace ESPressio {

    namespace Observable {
   
        /// A `ThreadSafeObservable` is an object that can be observed by any number of `IObserver` descendant types
        /// This is a concrete implementation of `IObservable`, and is Thread Safe!
        /// Your Observers can Register or Unregister themselves at any time, and the `ThreadSafeObservable` will handle it!
        class ThreadSafeObservable : public IObservable {
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
                
            protected:
                /// Will call the `callback` for each Observer
                void WithObservers(std::function<void(IObserver*)> callback) {
                    std::lock_guard<std::recursive_mutex> lock(_mutex);
                    const std::vector<IObserver*> observers =
                        _copyObserverPointers();

                    for (IObserver* observer : observers) {
                        if (_isObserverRegistered(observer)) {
                            callback(observer);
                        }
                    }
                }

                /// Will call the `callback` for each Observer that is of type `ObserverType`
                template <class ObserverType>
                void WithObservers(std::function<void(ObserverType*)> callback) {
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
                    std::unique_ptr<ObserverHandle> handle =
                        std::make_unique<ObserverHandle>(GetLifetimeControl(), observer);
                    ObserverHandle* result = handle.get();
                    _observers.push_back(result);
                    handle.release();
                    return result;
                }

                void UnregisterObserver(IObserver* observer) override {
                    std::lock_guard<std::recursive_mutex> lock(_mutex);
                    for (auto thisObserver = _observers.begin(); thisObserver != _observers.end(); thisObserver++) {
                        if ((*thisObserver)->GetObserver() == observer) {
                            static_cast<ObserverHandle*>((*thisObserver))->__invalidate();
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
