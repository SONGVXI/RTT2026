

# Release Notes for RoT Provisioning SW Pack

[![ST logo](_htmresc/st_logo_2020.png)](https://www.st.com)

# Purpose

STM32Cube enables developers to achieve design success. With a comprehensive suite of professional development tools and embedded software components, STM32Cube allows developers to differentiate products, streamline design cycles, and reduce costs. STM32Cube ecosystem supports all design steps, including selection, configuration, development, debugging, programming, and monitoring.

The STM32Cube embedded software offer provides ready-to-use software components that can be added to a project. It includes STM32 peripheral driver APIs with two levels of abstraction, middleware, board drivers, and examples. There are several distribution channels, including the STM32CubeMX2 tool, the ST website, and GitHub. All embedded software comes with enhanced online documentation, with flowcharts and user sequences.

**RoT Provisioning** is an STMicroelectronics SW pack that provides shared Python scripts to configure, build, and provision Root of Trust (RoT) examples on STM32 devices.

- It centralizes common RoT provisioning logic such as memory flash layout computation, image preparation (signing/encryption), and device configuration, so that individual RoT examples can use a common framework.

# Update history


<label for="collapse-section2" aria-hidden="true">__V2.0.1 / 12-June-2026__</label>
<div>

## Main changes

- Introduced a common project factory to reduce project-specific imports and simplify project handling.
- Renamed `oemirot/generic_provisioning.py` to `oemirot/provisioning.py`.

## Contents

- Core RoT provisioning logic for memory flash layouts, image preparation, and device configuration.
- Shared Python scripts used by STM32 RoT examples to run prebuild/postbuild and provisioning flows.
- Supported RoT example types:
  - oemirot_dualslot
  - oemirot_dualslot_hwcrypto

## Known limitations

- None

## Development toolchains and compilers

- None

## Supported devices and boards

- STM32C5xx series.

## Backward compatibility

- None

## Dependencies

- Python 3.10 or later.

</div>

<label for="collapse-section1" aria-hidden="true">__V2.0.0 / 13-March-2026__</label>
<div>


## Main changes

- First official release of RoT Provisioning SW Pack.

## Contents

- Core RoT provisioning logic for memory flash layouts, image preparation, and device configuration.
- Shared Python scripts used by STM32 RoT examples to run prebuild/postbuild and provisioning flows.
- Supported RoT example types:
  - oemirot_overwrite

## Known limitations

- None

## Development toolchains and compilers

- None

## Supported devices and boards

- STM32C5xx series.

## Backward compatibility

- None

## Dependencies

- Python 3.10 or later.

</div>


For complete documentation on **RoT Provisioning** ,
visit: [STM32 Trust](https://www.st.com/stm32trust)
<abbr title="Based on template cx566953 version 2.1">Info</abbr>