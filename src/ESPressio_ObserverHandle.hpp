#pragma once

#include <atomic>
#include <memory>

#include "ESPressio_IObservable.hpp"
#include "ESPressio_IObserver.hpp"

namespace ESPressio {

    namespace Observable {

        class ObserverHandle : public IObserverHandle {
            private:
                friend class Observable;
                friend class ObservableWithBuckets;
                friend class ThreadSafeObservable;

                std::shared_ptr<Detail::ObservableLifetimeControl> _lifetimeControl;
                std::atomic<IObserver*> _observer;
                std::atomic<bool> _registered{true};

                static std::shared_ptr<Detail::ObservableLifetimeControl>
                GetValidatedLifetimeControl(IObservable* observable) {
                    if (observable == nullptr) {
                        throw InvalidObservableHandleException();
                    }
                    return observable->GetLifetimeControl();
                }

                static std::shared_ptr<Detail::ObservableLifetimeControl>
                GetValidatedLifetimeControl(
                    std::shared_ptr<Detail::ObservableLifetimeControl> lifetimeControl) {
                    if (!lifetimeControl) {
                        throw InvalidObservableHandleException();
                    }
                    return lifetimeControl;
                }

                static IObserver* GetValidatedObserver(IObserver* observer) {
                    if (observer == nullptr) {
                        throw InvalidObserverRegistrationException();
                    }
                    return observer;
                }

                void InvalidateRegistration() noexcept {
                    _registered.store(false);
                    _observer.store(nullptr);
                }

            private:
                ObserverHandle(IObservable* observable, IObserver* observer)
                    : ObserverHandle(GetValidatedLifetimeControl(observable), observer) {}

                ObserverHandle(
                    std::shared_ptr<Detail::ObservableLifetimeControl> lifetimeControl,
                    IObserver* observer)
                    : _lifetimeControl(
                        GetValidatedLifetimeControl(std::move(lifetimeControl))),
                      _observer(GetValidatedObserver(observer)) {}

            public:

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

                void Unregister() override {
                    IObserver* observer = _observer.load();
                    if (!_registered.exchange(false)) { return; }

                    IObservable* observable = _lifetimeControl->Acquire();
                    if (observable == nullptr) {
                        _observer.store(nullptr);
                        return;
                    }

                    try {
                        observable->UnregisterObserver(observer);
                    } catch (...) {
                        _lifetimeControl->Release();
                        _observer.store(observer);
                        _registered.store(true);
                        throw;
                    }
                    _lifetimeControl->Release();
                    _observer.store(nullptr);
                }

                IObservable* GetObservable() override {
                    if (!_registered.load()) { return nullptr; }
                    return _lifetimeControl->Peek();
                }

                IObserver* GetObserver() override {
                    return _observer.load();
                }
        };

    }

}
