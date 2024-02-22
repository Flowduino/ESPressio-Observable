#pragma once

#include <functional>
#include <vector>

#include "ESPressio_IObservable.hpp"
#include "ESPressio_IObserver.hpp"
#include "ESPressio_Observer.hpp"

namespace ESPressio {

    namespace Observable {
   
        class Observable : public IObservable {
            private:
                std::vector<IObserver*> _observers;
            protected:
                /// Will call the `callback` for each Observer
                virtual void WithObservers(std::function<void(IObserver*)> callback) {
                    for (auto observer : _observers) {
                        callback(observer);
                    }
                }

                /// Will call the `callback` for each Observer that is of type `ObserverType`
                template <class ObserverType>
                void WithObservers(std::function<void(ObserverType*)> callback) {
                    for (auto observer : _observers) {
                        ObserverType* observerAsT = dynamic_cast<ObserverType*>(observer);
                        if (!observerAsT) { continue; }
                        callback(observerAsT);
                    }
                }
            public:
                virtual IObserverHandle* RegisterObserver(IObserver* observer) {
                    for (auto thisObserver : _observers) {
                        if (thisObserver == observer) { return new ObserverHandle(this, observer); }
                    }
                    _observers.push_back(observer);
                    return new ObserverHandle(this, observer);
                }

                virtual void UnregisterObserver(IObserver* observer) {
                    for (auto thisObserver = _observers.begin(); thisObserver != _observers.end(); thisObserver++) {
                        if (*thisObserver == observer) {
                            _observers.erase(thisObserver);
                            return;
                        }
                    }
                }

                virtual bool IsObserverRegistered(IObserver* observer) {
                    for (auto thisObserver : _observers) {
                        if (thisObserver == observer) { return true; }
                    }
                    return false;
                }
        };

    }

}