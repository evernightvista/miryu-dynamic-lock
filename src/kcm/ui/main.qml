/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2026 Miryu Dynamic Lock contributors
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kcmutils as KCMUtils
import org.kde.kirigami as Kirigami

// Root is SimpleKCM rather than ScrollViewKCM: our whole UI is a form, and
// SimpleKCM is the component whose documented purpose is a scrollable form
// (a Kirigami.FormLayout as its default child). ScrollViewKCM has no default
// content alias — it only scrolls the item assigned to its `view:` property
// (see plasma-firewall's kcm/ui/main.qml and ViewBase.qml, which both set
// `view: TableView`). The header-switch technique below is unaffected by the
// choice: both roots derive from Kirigami.Page, so System Settings renders the
// `actions` list identically, to the right of the page title.
KCMUtils.SimpleKCM {
    id: root

    implicitHeight: Kirigami.Units.gridUnit * 25
    implicitWidth: Kirigami.Units.gridUnit * 44

    // Master switch, placed in the page header to the right of the title.
    //
    // This is the technique plasma-firewall uses in its kcm/ui/main.qml: the
    // System Settings page header renders the "actions" list, and a checkable
    // Kirigami.Action whose displayComponent is a QQC2.Switch turns that entry
    // into a switch next to the title. A QWidget KCModule cannot do this —
    // System Settings only draws a plain text title for those.
    //
    // The properties are driven by the C++ module (exposed to QML as "kcm"),
    // whose load()/defaults() updates must be reflected here. Each control
    // therefore keeps its binding in a bindCurrent() helper and re-establishes
    // it after the user changes the value (an interactive change assigns the
    // property imperatively and would otherwise break the binding).
    actions: [
        Kirigami.Action {
            text: enableSwitch.text
            checkable: true
            enabled: enableSwitch.enabled
            checked: enableSwitch.checked
            onTriggered: enableSwitch.toggle()
            displayComponent: QQC2.Switch {
                id: enableSwitch
                text: i18ndc("miryu-dynamic-lock", "@option:check", "Enable Miryu Dynamic Lock")

                function bindCurrent() {
                    checked = Qt.binding(function() {
                        return kcm.enabled;
                    });
                }
                Component.onCompleted: bindCurrent()

                onToggled: {
                    kcm.enabled = checked;
                    bindCurrent();
                }
            }
        }
    ]

    // No-Bluetooth warning. Visible only while the master switch is on AND
    // the host has no Bluetooth support: the user is warned exactly when they
    // try to enable a feature this machine cannot provide, and never while the
    // feature is simply left off.
    //
    // SimpleKCM swaps the item assigned here into its header container. Setting
    // position: Header (as evernight-vista-logon's KCM does) attaches it to the
    // page header, and it collapses to zero height while hidden.
    header: Kirigami.InlineMessage {
        position: Kirigami.InlineMessage.Position.Header
        type: Kirigami.MessageType.Warning
        visible: kcm.enabled && !kcm.bluetoothAvailable
        text: i18ndc("miryu-dynamic-lock", "@info:warning",
                     "This computer does not have Bluetooth support. You cannot use this feature.")
    }

    Kirigami.FormLayout {
        Item {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18ndc("miryu-dynamic-lock", "@title:group", "Lock behavior")
        }

        QQC2.SpinBox {
            Kirigami.FormData.label: i18ndc("miryu-dynamic-lock", "@label:spinbox",
                                            "Lock delay after the phone is taken away:")
            enabled: kcm.enabled
            from: 1
            to: 600
            textFromValue: function(value) {
                return value + i18ndc("miryu-dynamic-lock", "suffix for seconds", " s");
            }
            valueFromText: function(text) {
                return parseInt(text, 10);
            }
            function bindCurrent() {
                value = Qt.binding(function() {
                    return kcm.lockTimeout;
                });
            }
            Component.onCompleted: bindCurrent()
            onValueModified: {
                kcm.lockTimeout = value;
                bindCurrent();
            }
        }

        QQC2.SpinBox {
            Kirigami.FormData.label: i18ndc("miryu-dynamic-lock", "@label:spinbox",
                                            "Signal strength threshold:")
            enabled: kcm.enabled
            from: -100
            to: 0
            textFromValue: function(value) {
                return value + i18ndc("miryu-dynamic-lock", "suffix for dBm", " dBm");
            }
            valueFromText: function(text) {
                return parseInt(text, 10);
            }
            function bindCurrent() {
                value = Qt.binding(function() {
                    return kcm.rssiThreshold;
                });
            }
            Component.onCompleted: bindCurrent()
            onValueModified: {
                kcm.rssiThreshold = value;
                bindCurrent();
            }
        }

        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            opacity: 0.7
            text: i18ndc("miryu-dynamic-lock", "@info:label",
                         "The phone is treated as taken away once its Bluetooth signal drops below this value. Higher values (e.g. -65 dBm) lock at a shorter distance; lower values (e.g. -85 dBm) lock only at a greater distance.")
        }

        QQC2.CheckBox {
            enabled: kcm.enabled
            text: i18ndc("miryu-dynamic-lock", "@option:check",
                         "Cancel a pending lock when the phone reconnects")
            function bindCurrent() {
                checked = Qt.binding(function() {
                    return kcm.cancelOnReconnect;
                });
            }
            Component.onCompleted: bindCurrent()
            onToggled: {
                kcm.cancelOnReconnect = checked;
                bindCurrent();
            }
        }

        QQC2.CheckBox {
            enabled: kcm.enabled
            text: i18ndc("miryu-dynamic-lock", "@option:check",
                         "Show a desktop notification before locking")
            function bindCurrent() {
                checked = Qt.binding(function() {
                    return kcm.notifyBeforeLock;
                });
            }
            Component.onCompleted: bindCurrent()
            onToggled: {
                kcm.notifyBeforeLock = checked;
                bindCurrent();
            }
        }

        Item {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18ndc("miryu-dynamic-lock", "@title:group", "Paired phone")
        }

        RowLayout {
            Kirigami.FormData.label: i18ndc("miryu-dynamic-lock", "@label:listbox", "Bluetooth device:")

            QQC2.ComboBox {
                id: deviceCombo
                Layout.preferredWidth: Kirigami.Units.gridUnit * 18
                enabled: kcm.enabled
                model: kcm.devices
                textRole: "display"

                function bindCurrent() {
                    currentIndex = Qt.binding(function() {
                        const idx = kcm.devices.findIndex(function(device) {
                            return device.address === kcm.deviceAddress;
                        });
                        return idx >= 0 ? idx : 0;
                    });
                }
                Component.onCompleted: bindCurrent()

                onActivated: {
                    kcm.deviceAddress = kcm.devices[currentIndex].address;
                    bindCurrent();
                }
            }

            QQC2.Button {
                text: i18ndc("miryu-dynamic-lock", "@action:button", "Refresh")
                icon.name: "view-refresh"
                onClicked: kcm.refreshDevices()
            }
        }

        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: kcm.statusText
            opacity: 0.7
        }

        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: i18ndc("miryu-dynamic-lock", "@info:label",
                         "Pair your phone in the KDE Bluetooth settings first, then press Refresh to populate this list. When the selected phone leaves Bluetooth range, the screen will lock after the configured delay.")
            opacity: 0.7
        }
    }
}
