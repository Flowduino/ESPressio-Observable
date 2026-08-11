#pragma once

#include <atomic>
#include <memory>

#include "ESPressio_IObservable.hpp"
#include "ESPressio_IObserver.hpp"

namespace ESPressio {

    namespace Observable {

        class ObserverHandle : public IObserverHandle {
            private:
                std::shared_ptr<Detail::ObservableLifetimeControl> _lifetimeControl;
                IObserver* _observer;
                std::atomic<bool> _registered{true};
            public:
                ObserverHandle(IObservable* observable, IObserver* observer)
                    : ObserverHandle(observable->GetLifetimeControl(), observer) {}

                ObserverHandle(
                    std::shared_ptr<Detail::ObservableLifetimeControl> lifetimeControl,
                    IObserver* observer)
                    : _lifetimeControl(std::move(lifetimeControl)), _observer(observer) {}

                ObserverHandle(const ObserverHandle&) = delete;
                ObserverHandle& operator=(const ObserverHandle&) = delete;
                ObserverHandle(ObserverHandle&&) = delete;
                ObserverHandle& operator=(ObserverHandle&&) = delete;

                ~ObserverHandle() noexcept override {
                    try {
                        Unregister();
                    } catch (...) {
                        // Destructors must not propagate exceptions. Explicitly call
                        // Unregister() when registration errors need to be observed.
                    }
                }

                void __invalidate() noexcept {
                    _registered.store(false);
                }

                void Unregister() override {
                    if (!_registered.exchange(false)) { return; }

                    IObservable* observable = _lifetimeControl->Acquire();
                    if (observable == nullptr) { return; }

                    try {
                        observable->UnregisterObserver(_observer);
                    } catch (...) {
                        _lifetimeControl->Release();
                        _registered.store(true);
                        throw;
                    }
                    _lifetimeControl->Release();
                }

                IObservable* GetObservable() override {
                    if (!_registered.load()) { return nullptr; }
                    return _lifetimeControl->Peek();
                }

                IObserver* GetObserver() override {
                    return _observer;
                }
        };

    }

}
