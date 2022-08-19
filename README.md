<!--
    SPDX-License-Identifier: CC0-1.0
    SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
-->

# Requirements

- Must be installed to /usr! Polkit and DBus hardcode /usr as source for policies and system services
- KIO 5.98 (master at the time of writing)

# Functionality

kio-admin implements a new protocol `admin:///` which gives administrative access to the entire system. This is achieved
by talking, over dbus, with a root-level helper binary that in turn uses existing KIO infrastructure to run file://
operations in root-scope. Or simply put: `admin://` is exactly like `file://` but redirected over dbus to gain
administrative privileges.

```shell
dolphin admin:///
```
