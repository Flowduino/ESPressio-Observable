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

                ~ObserverHandle() override {
                    Unregister();
                }

                void __invalidate() {
                    _observable = nullptr;
                }

                void Unregister() override {
                    if (_observable == nullptr) { return; }
                    _observable->UnregisterObserver(_observer);
                    __invalidate();
                }

                IObservable* GetObservable() override {
                    return _observable;
                }

                IObserver* GetObserver() override {
                    return _observer;
                }
        };

    }

}