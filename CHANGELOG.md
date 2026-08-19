# Changelog

All notable changes to this project are documented in this file.

The structure follows the principles of [Keep a
Changelog](https://keepachangelog.com/en/1.1.0/) and [Semantic
Versioning](https://semver.org/).

> **Historical note:** This changelog was reconstructed retrospectively
> from published GitHub Releases, tags, release notes, repository
> history, and the documented public API. Where an historical release
> had little or no release-note detail, the entry is intentionally terse
> rather than inferring unsupported intent.

## \[3.0.0\] - 2026-08-13

### Changed

-   Changed Observer registration ownership from raw owning handles to
    RAII-managed smart-pointer handles.
-   Updated registration APIs to make ownership explicit and
    memory-safe.
-   Made notification mutation/lifetime handling safe when registrations
    change during notification.

### Fixed

-   Removed raw-handle ownership ambiguity and associated leak risks.
-   Corrected Observer-registration lifetime hazards during
    callback/notification mutation.

## \[2.0.0\]

### Changed

-   Established the Observable 2.x interface consumed by ESPressio Event
    2.1 and other ESPressio libraries.
-   Standardised the common `IObserver`-based synchronous observation
    contract.

## \[1.x\]

### Added

-   Initial ESPressio Observable synchronous Observer Pattern
    infrastructure.

> Detailed release notes for the earliest Observable history were not
> published on the current GitHub Releases pages; these entries are
> intentionally limited to the public API lineage that can be
> substantiated.
