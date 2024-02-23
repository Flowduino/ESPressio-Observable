#pragma once

#include <functional>
#include <vector>
#include <mutex>

#include "ESPressio_IObservable.hpp"
#include "ESPressio_IObserver.hpp"
#include "ESPressio_ObserverHandle.hpp"

namespace ESPressio {

    namespace Observable {
   
        /// A `ThreadSafeObservable` is an object that can be observed by any number of `IObserver` descendant types
        /// This is a concrete implementation of `IObservable`, and is Thread Safe!
        class ThreadSafeObservable : public IObservable {
            private:
                std::vector<IObserverHandle*> _observers;
                std::mutex _mutex;

                std::vector<IObserverHandle*>* CopyObservers() {
                    _mutex.lock();
                    std::vector<IObserverHandle*>* observers = new std::vector<IObserverHandle*>(_observers);
                    _mutex.unlock();
                    return observers;
                }
                
            protected:
                /// Will call the `callback` for each Observer
                virtual void WithObservers(std::function<void(IObserver*)> callback) {
                    std::vector<IObserverHandle*>* observers = CopyObservers();
                    for (auto observer : *observers) {
                        callback(observer->GetObserver());
                    }
                    delete observers;
                }

                /// Will call the `callback` for each Observer that is of type `ObserverType`
                template <class ObserverType>
                void WithObservers(std::function<void(ObserverType*)> callback) {
                    std::vector<IObserverHandle*>* observers = CopyObservers();

                    for (auto observer : *observers) {
                        ObserverType* observerAsT = dynamic_cast<ObserverType*>(observer->GetObserver());
                        if (!observerAsT) { continue; }
                        callback(observerAsT);
                    }

                    delete observers;
                }
            public:
                ~ThreadSafeObservable() {
                    _mutex.lock();
                    for (auto observer : _observers) {
                        static_cast<ObserverHandle*>(observer)->__invalidate();
                    }

                    _observers.clear();
                    _mutex.unlock();
                }

                virtual IObserverHandle* RegisterObserver(IObserver* observer) {
                    _mutex.lock();
                    for (auto thisObserver : _observers) {
                        if (thisObserver->GetObserver() == observer) {
                            _mutex.unlock();
                            return thisObserver;
                        }
                    }
                    IObserverHandle* handle = new ObserverHandle(this, observer);
                    _observers.push_back(handle);
                    _mutex.unlock();
                    return handle;
                }

                virtual void UnregisterObserver(IObserver* observer) {
                    _mutex.lock();
                    for (auto thisObserver = _observers.begin(); thisObserver != _observers.end(); thisObserver++) {
                        if ((*thisObserver)->GetObserver() == observer) {
                            static_cast<ObserverHandle*>((*thisObserver))->__invalidate();
                            _observers.erase(thisObserver);
                            _mutex.unlock();
                            return;
                        }
                    }
                    _mutex.unlock();
                }

                virtual bool IsObserverRegistered(IObserver* observer) {
                    _mutex.lock();
                    for (auto thisObserver : _observers) {
                        if ((*thisObserver).GetObserver() == observer) {
                            _mutex.unlock();
                            return true;
                        }
                    }
                    _mutex.unlock();
                    return false;
                }
        };

    }

}