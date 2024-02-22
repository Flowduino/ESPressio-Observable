#pragma once

#include "ESPressio_IObservable.hpp"
#include "ESPressio_IObserver.hpp"

namespace ESPressio {

    namespace Observable {

        class ObserverHandle : public IObserverHandle {
            private:
                IObservable* _observable;
                IObserver* _observer;
            public:
                ObserverHandle(IObservable* observable, IObserver* observer) : _observable(observable), _observer(observer) {}

                ~ObserverHandle() {
                    Unregister();
                }

                virtual void __invalidate() {
                    _observable = nullptr;
                }

                virtual void Unregister() {
                    if (_observable == nullptr) { return; }
                    _observable->UnregisterObserver(_observer);
                }

                virtual IObservable* GetObservable() {
                    return _observable;
                }

                virtual IObserver* GetObserver() {
                    return _observer;
                }
        };

    }

}