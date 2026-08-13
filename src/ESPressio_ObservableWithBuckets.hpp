#pragma once

#include <algorithm>
#include <memory>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ESPressio_IObservable.hpp"
#include "ESPressio_IObserver.hpp"
#include "ESPressio_ObserverHandle.hpp"

namespace ESPressio {

    namespace Observable {

        namespace Detail {
            template <class... ObserverInterfaces>
            struct AllInterfacesPolymorphic;

            template <>
            struct AllInterfacesPolymorphic<> : std::true_type {};

            template <class ObserverInterface, class... RemainingInterfaces>
            struct AllInterfacesPolymorphic<ObserverInterface, RemainingInterfaces...>
                : std::integral_constant<
                    bool,
                    std::is_polymorphic<ObserverInterface>::value &&
                    AllInterfacesPolymorphic<RemainingInterfaces...>::value
                > {};
        }

        /// A non-thread-safe Observable optimized for typed dispatch. Observer
        /// interfaces are supplied explicitly at registration so notification
        /// performs no dynamic casts.
        class ObservableWithBuckets : public IObservable {
            private:
                struct BucketEntry {
                    ObserverHandle* handle;
                    void* observerInterface;
                };

                struct Registration {
                    ObserverHandle* handle;
                    std::vector<std::type_index> interfaces;
                };

                using Bucket = std::vector<BucketEntry>;

                std::unordered_map<std::type_index, Bucket> _buckets;
                std::unordered_map<IObserver*, Registration> _registrations;
                std::size_t _notificationDepth = 0;
                bool _needsCompaction = false;

                void _compactBuckets() {
                    for (auto bucketIterator = _buckets.begin();
                            bucketIterator != _buckets.end();) {
                        Bucket& bucket = bucketIterator->second;
                        bucket.erase(
                            std::remove_if(
                                bucket.begin(), bucket.end(),
                                [](const BucketEntry& entry) {
                                    return entry.handle == nullptr;
                                }),
                            bucket.end());
                        if (bucket.empty()) {
                            bucketIterator = _buckets.erase(bucketIterator);
                        } else {
                            ++bucketIterator;
                        }
                    }
                    _needsCompaction = false;
                }

                void _finishNotification() {
                    if (--_notificationDepth == 0 && _needsCompaction) {
                        _compactBuckets();
                    }
                }

                static bool _containsInterface(
                    const std::vector<std::type_index>& interfaces,
                    const std::type_index& observerInterface) {
                    return std::find(
                        interfaces.begin(), interfaces.end(), observerInterface
                    ) != interfaces.end();
                }

                static bool _sameInterfaces(
                    const std::vector<std::type_index>& left,
                    const std::vector<std::type_index>& right) {
                    if (left.size() != right.size()) { return false; }
                    for (const std::type_index& observerInterface : left) {
                        if (!_containsInterface(right, observerInterface)) {
                            return false;
                        }
                    }
                    return true;
                }

                template <class ObserverInterface>
                static bool _resolveInterface(
                    IObserver* observer,
                    std::vector<std::pair<std::type_index, void*> >& resolvedInterfaces) {
                    ObserverInterface* observerInterface =
                        dynamic_cast<ObserverInterface*>(observer);
                    if (observerInterface == nullptr) { return false; }

                    const std::type_index interfaceType(typeid(ObserverInterface));
                    const auto duplicate = std::find_if(
                        resolvedInterfaces.begin(), resolvedInterfaces.end(),
                        [&interfaceType](
                            const std::pair<std::type_index, void*>& resolved) {
                            return resolved.first == interfaceType;
                        }
                    );
                    if (duplicate == resolvedInterfaces.end()) {
                        resolvedInterfaces.emplace_back(
                            interfaceType,
                            static_cast<void*>(observerInterface)
                        );
                    }
                    return true;
                }

                void _removeFromBuckets(const Registration& registration) noexcept {
                    for (const std::type_index& observerInterface : registration.interfaces) {
                        auto bucketIterator = _buckets.find(observerInterface);
                        if (bucketIterator == _buckets.end()) { continue; }

                        Bucket& bucket = bucketIterator->second;
                        if (_notificationDepth > 0) {
                            for (BucketEntry& entry : bucket) {
                                if (entry.handle == registration.handle) {
                                    entry.handle = nullptr;
                                    entry.observerInterface = nullptr;
                                    _needsCompaction = true;
                                }
                            }
                        } else {
                            bucket.erase(
                                std::remove_if(
                                    bucket.begin(), bucket.end(),
                                    [&registration](const BucketEntry& entry) {
                                        return entry.handle == registration.handle;
                                    }),
                                bucket.end());
                        }

                        if (_notificationDepth == 0 && bucket.empty()) {
                            _buckets.erase(bucketIterator);
                        }
                    }
                }

                /// Dispatch uses the interface pointer resolved during registration.
                template <class ObserverType, class Callback>
                void _withObservers(Callback&& callback) {
                    const auto bucketIterator =
                        _buckets.find(std::type_index(typeid(ObserverType)));
                    if (bucketIterator == _buckets.end()) { return; }

                    ++_notificationDepth;
                    Bucket& bucket = bucketIterator->second;
                    const std::size_t observerCount = bucket.size();
                    try {
                        for (std::size_t index = 0; index < observerCount; ++index) {
                            const BucketEntry& entry = bucket[index];
                            if (entry.handle != nullptr) {
                                callback(static_cast<ObserverType*>(entry.observerInterface));
                            }
                        }
                    } catch (...) {
                        _finishNotification();
                        throw;
                    }
                    _finishNotification();
                }

            protected:
                class NotificationContext {
                    private:
                        friend class ObservableWithBuckets;
                        ObservableWithBuckets& _observable;
                        std::shared_ptr<IObservable> _notificationLifetime;
                        NotificationContext(
                            ObservableWithBuckets& observable,
                            std::shared_ptr<IObservable> notificationLifetime)
                            : _observable(observable),
                              _notificationLifetime(std::move(notificationLifetime)) {}

                    public:
                        template <class ObserverType, class Callback>
                        void WithObservers(Callback&& callback) {
                            _observable._withObservers<ObserverType>(
                                std::forward<Callback>(callback));
                        }
                };

                template <class Operation>
                void ExecuteNotification(Operation&& operation) {
                    NotificationContext context(
                        *this, AcquireNotificationLifetime());
                    operation(context);
                }

            public:
                ~ObservableWithBuckets() override {
                    BeginObservableDestruction();
                    for (auto& registration : _registrations) {
                        registration.second.handle->InvalidateRegistration();
                    }
                    _buckets.clear();
                    _registrations.clear();
                }

                template <class... ObserverInterfaces>
                ObserverHandlePtr RegisterObserverAs(IObserver* observer) {
                    static_assert(
                        sizeof...(ObserverInterfaces) > 0,
                        "At least one Observer interface must be specified"
                    );
                    static_assert(
                        Detail::AllInterfacesPolymorphic<ObserverInterfaces...>::value,
                        "Every Observer interface must be polymorphic"
                    );

                    if (observer == nullptr) {
                        throw InvalidObserverRegistrationException();
                    }

                    std::vector<std::pair<std::type_index, void*> > resolvedInterfaces;
                    resolvedInterfaces.reserve(sizeof...(ObserverInterfaces));

                    bool interfacesMatch = true;
                    const int resolveInterfaces[] = {
                        0,
                        (interfacesMatch =
                            _resolveInterface<ObserverInterfaces>(
                                observer, resolvedInterfaces
                            ) && interfacesMatch,
                         0)...
                    };
                    (void)resolveInterfaces;

                    if (!interfacesMatch) {
                        throw ObserverInterfaceMismatchException();
                    }

                    std::vector<std::type_index> interfaceTypes;
                    interfaceTypes.reserve(resolvedInterfaces.size());
                    for (const auto& resolved : resolvedInterfaces) {
                        interfaceTypes.push_back(resolved.first);
                    }

                    const auto existing = _registrations.find(observer);
                    if (existing != _registrations.end()) {
                        if (!_sameInterfaces(existing->second.interfaces, interfaceTypes)) {
                            throw ObserverRegistrationConflictException();
                        }
                        throw DuplicateObserverRegistrationException();
                    }

                    std::unique_ptr<ObserverHandle> handle(
                        new ObserverHandle(GetLifetimeControl(), observer));
                    ObserverHandle* result = handle.get();
                    std::vector<std::type_index> insertedBuckets;
                    insertedBuckets.reserve(resolvedInterfaces.size());

                    try {
                        for (const auto& resolved : resolvedInterfaces) {
                            _buckets[resolved.first].push_back(
                                BucketEntry{result, resolved.second}
                            );
                            insertedBuckets.push_back(resolved.first);
                        }

                        _registrations.emplace(
                            observer,
                            Registration{result, std::move(interfaceTypes)}
                        );
                    } catch (...) {
                        Registration partial{result, std::move(insertedBuckets)};
                        _removeFromBuckets(partial);
                        throw;
                    }

                    return ObserverHandlePtr(handle.release());
                }

                void UnregisterObserver(IObserver* observer) override {
                    const auto registration = _registrations.find(observer);
                    if (registration == _registrations.end()) { return; }

                    registration->second.handle->InvalidateRegistration();
                    _removeFromBuckets(registration->second);
                    _registrations.erase(registration);
                }

                bool IsObserverRegistered(IObserver* observer) override {
                    return _registrations.find(observer) != _registrations.end();
                }
        };

    }

}
