#pragma once

#include <functional>
#include <vector>

#include "ESPressio_IObservable.hpp"

namespace ESPressio {

    namespace Observable {
   
        class Observable : public IObservable {
            private:
                std::vector<void*> observers;
            protected:
                /// Will call the `callback` for each Observer
                virtual void WithObservers(std::function<void(void*)> callback) {
                    for (auto observer : observers) {
                        callback(observer);
                    }
                }

                /// Will call the `callback` for each Observer that is of type `ObserverType`
                template <class ObserverType>
                void WithObservers(std::function<void(ObserverType*)> callback) {
                    for (auto observer : observers) {
                        ObserverType* observerAsT = dynamic_cast<ObserverType*>(observer);
                        if (!observerAsT) { continue; }
                        callback(observerAsT);
                    }
                }
            public:
                virtual void RegisterObserver(void* observer) {
                    if (IsObserverRegistered(observer)) { return; }
                    observers.push_back(observer);
                }

                virtual void UnregisterObserver(void* observer) {
                    for (auto thisObserver = observers.begin(); thisObserver != observers.end(); thisObserver++) {
                        if (*thisObserver == observer) {
                            observers.erase(thisObserver);
                            return;
                        }
                    }
                }

                virtual bool IsObserverRegistered(void* observer) {
                    for (auto thisObserver : observers) {
                        if (thisObserver == observer) { return true; }
                    }
                    return false;
                }
        };

    }

}