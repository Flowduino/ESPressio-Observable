#pragma once

#include <functional>
#include <vector>

#include "ESPressio_IObservable.hpp"
#include "ESPressio_IObserver.hpp"
#include "ESPressio_ObserverHandle.hpp"

namespace ESPressio {

    namespace Observable {
   
        /// An `Observable` is an object that can be observed by any number of `IObserver` descendant types
        /// This is a concrete implementation of `IObservable`, but it is NOT Thread-Safe.
        /// Registering or Unregistering Observers while Observers are being notified can lead to undefined behavior.
        /// If you need a Thread-Safe Implementation, use the `ThreadSafeObservable` class instead.
        class Observable : public IObservable {
            private:
                std::vector<IObserverHandle*> _observers;
            protected:
                /// Will call the `callback` for each Observer
                void WithObservers(std::function<void(IObserver*)> callback) {
                    for (auto observer : _observers) {
                        callback(observer->GetObserver());
                    }
                }

                /// Will call the `callback` for each Observer that is of type `ObserverType`
                template <class ObserverType>
                void WithObservers(std::function<void(ObserverType*)> callback) {
                    for (auto observer : _observers) {
                        ObserverType* observerAsT = dynamic_cast<ObserverType*>(observer->GetObserver());
                        if (!observerAsT) { continue; }
                        callback(observerAsT);
                    }
                }
            public:
                ~Observable() {
                    for (auto observer : _observers) {
                        static_cast<ObserverHandle*>(observer)->__invalidate();
                    }
                }

                virtual IObserverHandle* RegisterObserver(IObserver* observer) {
                    for (auto thisObserver : _observers) {
                        if (thisObserver->GetObserver() == observer) { return thisObserver; }
                    }
                    IObserverHandle* handle = new ObserverHandle(this, observer);
                    _observers.push_back(handle);
                    return handle;
                }

                virtual void UnregisterObserver(IObserver* observer) {
                    for (auto thisObserver = _observers.begin(); thisObserver != _observers.end(); thisObserver++) {
                        if ((*thisObserver)->GetObserver() == observer) {
                            static_cast<ObserverHandle*>((*thisObserver))->__invalidate();
                            _observers.erase(thisObserver);
                            return;
                        }
                    }
                }

                virtual bool IsObserverRegistered(IObserver* observer) {
                    for (auto thisObserver : _observers) {
                        if ((*thisObserver).GetObserver() == observer) { return true; }
                    }
                    return false;
                }
        };

    }

}