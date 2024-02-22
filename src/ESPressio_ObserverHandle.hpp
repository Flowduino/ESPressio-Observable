#pragma once

#include <memory>
#include "ESPressio_IObservable.hpp"
#include "ESPressio_IObserver.hpp"

namespace ESPressio {

    namespace Observable {

        class ObserverHandle : public IObserverHandle {
            private:
                std::weak_ptr<IObservable> _observable;
                IObserver* _observer;
            public:
                ObserverHandle(std::weak_ptr<IObservable> observable, IObserver* observer) : _observable(observable), _observer(observer) {}
                ObserverHandle(IObservable* observable, IObserver* observer) : _observable(observable->shared_from_this()), _observer(observer) {}

                ~ObserverHandle() {
                    Unregister();
                }

                virtual void Unregister() {
                    std::shared_ptr<IObservable> observable = _observable.lock();
                    if (!observable) { return; }
                    observable->UnregisterObserver(_observer);
                }

                virtual std::weak_ptr<IObservable> GetObservable() {
                    return _observable;
                }

                virtual IObserver* GetObserver() {
                    return _observer;
                }
        };

    }

}