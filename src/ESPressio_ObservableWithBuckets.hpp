#ifndef ESPRESSIO_OBSERVABLE_EXPERIMENTAL
    #error This implementation is not yet complete. Please check back later for updates.
#endif

#pragma once

#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "ESPressio_IObservable.hpp"
#include "ESPressio_IObserver.hpp"
#include "ESPressio_ObserverHandle.hpp"

namespace ESPressio {

    namespace Observable {
   
        /// An `ObservableWithBuckets` is an object that can be observed by any number of `IObserver` descendant types
        /// This is a concrete implementation of `IObservable`, but it is NOT Thread-Safe.
        /// This variation uses "Buckets" (a Map, effectively) keyed on the `IObserver` type.
        /// It may be more performant when your descendant Observable is observed by a large number of DIFFERENT Observer types!
        /// Registering or Unregistering Observers while Observers are being notified can lead to undefined behavior.
        /// If you need a Thread-Safe Implementation, use the `ThreadSafeObservableWithBuckets` class instead.
        class ObservableWithBuckets : public IObservable {
            private:
                std::unordered_map<std::type_index, std::vector<IObserverHandle*>*> _observers;
            protected:
                /// Will call the `callback` for each Observer that is of type `ObserverType`
                template <class ObserverType>
                void WithObservers(std::function<void(ObserverType*)> callback) {
                    auto observerType = std::type_index(typeid(ObserverType)); // Get the Type Index of the ObserverType
                    std::vector<IObserverHandle*>* observers = _observers[observerType]; // Get the Observers for this Type Index
                    if (observers == nullptr || observers->empty()) { return; } // If there are no Observers, return

                    for (auto observer : *observers) { // For each Observer...
                        callback(observer); // ...call the callback (we know that it is of type `ObserverType`)
                    }
                }
            public:
                ~ObservableWithBuckets() {
                    for (auto& observer : _observers) { // Iterate all of the Type Buckets...
                        for (auto& observerHandle : *observer.second) { // ...and for each Observer in the Bucket...
                            static_cast<ObserverHandle*>(observerHandle)->__invalidate(); // ...invalidate it
                        }
                        delete observer.second; // ...and delete the Bucket
                    }
                    _observers.clear(); // Clear the Map
                }

                virtual IObserverHandle* RegisterObserver(IObserver* observer) {
                    auto observerType = std::type_index(typeid(*observer)); // Get the Type Index of the Observer
                    std::vector<IObserverHandle*>* observers = _observers[observerType]; // Get the Observers for this Type Index
                    if (observers == nullptr) { // If there are no Observers for this Type Index...
                        observers = new std::vector<IObserverHandle*>(); // ...create a new vector
                        _observers[observerType] = observers; // ...and add it to the Map
                    }

                    for (auto thisObserver : *observers) { // For each Observer in the vector...
                        if (thisObserver->GetObserver() == observer) { return thisObserver; } // ...if it is the same as the one we are trying to register, return it
                    }

                    IObserverHandle* handle = new ObserverHandle(this, observer); // Create a new ObserverHandle
                    observers->push_back(handle); // Add it to the vector
                    return handle; // Return the ObserverHandle
                }

                virtual void UnregisterObserver(IObserver* observer) {
                    auto observerType = std::type_index(typeid(*observer)); // Get the Type Index of the Observer
                    std::vector<IObserverHandle*>* observers = _observers[observerType]; // Get the Observers for this Type Index
                    if (observers == nullptr || observers->empty()) { return; } // If there are no Observers, return

                    for (auto thisObserver = observers->begin(); thisObserver != observers->end(); thisObserver++) { // For each Observer in the vector...
                        if ((*thisObserver)->GetObserver() == observer) { // ...if it is the same as the one we are trying to unregister...
                            static_cast<ObserverHandle*>((*thisObserver))->__invalidate(); // ...invalidate it
                            observers->erase(thisObserver); // ...and erase it from the vector
                            return; // ...and return
                        }
                    }
                }

                virtual bool IsObserverRegistered(IObserver* observer) {
                    auto observerType = std::type_index(typeid(*observer)); // Get the Type Index of the Observer
                    std::vector<IObserverHandle*>* observers = _observers[observerType]; // Get the Observers for this Type Index
                    if (observers == nullptr || observers->empty()) { return false; } // If there are no Observers, return false

                    for (auto thisObserver : *observers) { // For each Observer in the vector...
                        if (thisObserver->GetObserver() == observer) { return true; } // ...if it is the same as the one we are checking, return true
                    }
                    return false; // If we didn't find it, return false
                }
        };

    }

}