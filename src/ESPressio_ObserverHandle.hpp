#pragma once

#include <memory>
#include "ESPressio_IObservable.hpp"

namespace ESPressio {

    namespace Observable {

        class ObserverHandle : public IObserverHandle {
            private:
                std::weak_ptr<IObservable> _observable;
            public:
                ObserverHandle(std::weak_ptr<IObservable> observable) : _observable(observable) {}
                ObserverHandle(IObservable* observable) : _observable(observable->shared_from_this()) {}

                virtual void Unregister() {
                    std::shared_ptr<IObservable> observable = _observable.lock();
                    if (!observable) { return; }
                    observable->UnregisterObserver(this);
                }

                virtual std::weak_ptr<IObservable> GetObservable() {
                    return _observable;
                }
        };

    }

}