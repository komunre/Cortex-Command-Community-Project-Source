#include "SettingsInputGUI.h"
#include "UInputMan.h"

#include "GUI.h"
#include "GUICollectionBox.h"
#include "GUIButton.h"
#include "GUISlider.h"
#include "GUIRadioButton.h"
#include "GUILabel.h"

namespace RTE {

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	SettingsInputGUI::SettingsInputGUI(GUIControlManager *parentControlManager) : m_GUIControlManager(parentControlManager) {
		m_InputSettingsBox = dynamic_cast<GUICollectionBox *>(m_GUIControlManager->GetControl("CollectionBoxInputSettings"));

		for (int player = Players::PlayerOne; player < Players::MaxPlayerCount; ++player) {
			std::string playerNum = std::to_string(player + 1);

			m_PlayerInputSettingsBoxes.at(player).SelectedDeviceLabel = dynamic_cast<GUILabel *>(m_GUIControlManager->GetControl("LabelP" + playerNum + "SelectedDevice"));

			m_PlayerInputSettingsBoxes.at(player).NextDeviceButton = dynamic_cast<GUIButton *>(m_GUIControlManager->GetControl("ButtonP" + playerNum + "NextDevice"));
			m_PlayerInputSettingsBoxes.at(player).PrevDeviceButton = dynamic_cast<GUIButton *>(m_GUIControlManager->GetControl("ButtonP" + playerNum + "PrevDevice"));
			m_PlayerInputSettingsBoxes.at(player).ConfigureControlsButton = dynamic_cast<GUIButton *>(m_GUIControlManager->GetControl("ButtonP" + playerNum + "Config"));
			m_PlayerInputSettingsBoxes.at(player).ResetControlsButton = dynamic_cast<GUIButton *>(m_GUIControlManager->GetControl("ButtonP" + playerNum + "Clear"));

			m_PlayerInputSettingsBoxes.at(player).SensitivtyLabel = dynamic_cast<GUILabel *>(m_GUIControlManager->GetControl("LabelP" + playerNum + "Sensitivity"));
			m_PlayerInputSettingsBoxes.at(player).SensitivitySlider = dynamic_cast<GUISlider *>(m_GUIControlManager->GetControl("SliderP" + playerNum + "Sensitivity"));

			m_PlayerInputSettingsBoxes.at(player).DeadZoneControlsBox = dynamic_cast<GUICollectionBox *>(m_GUIControlManager->GetControl("CollectionBoxP" + playerNum + "DeadzoneControls"));
			m_PlayerInputSettingsBoxes.at(player).CircleDeadZoneRadioButton = dynamic_cast<GUIRadioButton *>(m_GUIControlManager->GetControl("RadioP" + playerNum + "DeadzoneCircle"));
			m_PlayerInputSettingsBoxes.at(player).SquareDeadZoneRadioButton = dynamic_cast<GUIRadioButton *>(m_GUIControlManager->GetControl("RadioP" + playerNum + "DeadzoneSquare"));
		}
		for (int player = Players::PlayerOne; player < Players::MaxPlayerCount; ++player) {
			UpdatePlayerSelectedDeviceLabel(player);
			ShowPlayerInputDeviceSensitivityControls(player);
		}
		m_InputConfigMenu = std::make_unique<SettingsInputMappingGUI>(parentControlManager);
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void SettingsInputGUI::SetEnabled(bool enable) const {
		if (enable) {
			m_InputSettingsBox->SetVisible(true);
			m_InputSettingsBox->SetEnabled(true);
		} else {
			m_InputSettingsBox->SetVisible(false);
			m_InputSettingsBox->SetEnabled(false);
		}
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void SettingsInputGUI::ResetPlayerInputSettings(int player) {
		if (m_PlayerInputSettingsBoxes.at(player).ResetControlsButton->GetText() == "Reset") {
			// Reset any other pending mapping reset confirmations
			for (int otherPlayer = Players::PlayerOne; otherPlayer < Players::MaxPlayerCount; ++otherPlayer) {
				if (otherPlayer != player) { m_PlayerInputSettingsBoxes.at(otherPlayer).ResetControlsButton->SetText("Reset"); }
			}
			m_PlayerInputSettingsBoxes.at(player).ResetControlsButton->SetText("CONFIRM?");
		} else {
			InputScheme *playerControlScheme = g_UInputMan.GetControlScheme(player);
			playerControlScheme->SetDevice(static_cast<InputDevice>(player));
			playerControlScheme->SetPreset(static_cast<InputPreset>(-(player + 1))); // Player 1's default preset is at -1 and so on.

			if (playerControlScheme->GetDevice() == InputDevice::DEVICE_MOUSE_KEYB) {
				g_UInputMan.SetMouseSensitivity(0.6F);
			} else if (playerControlScheme->GetDevice() != InputDevice::DEVICE_MOUSE_KEYB && playerControlScheme->GetDevice() != InputDevice::DEVICE_KEYB_ONLY) {
				playerControlScheme->SetJoystickDeadzone(0);
				playerControlScheme->SetJoystickDeadzoneType(DeadZoneType::CIRCLE);
			}
			UpdatePlayerSelectedDeviceLabel(player);
			ShowPlayerInputDeviceSensitivityControls(player);
			UpdatePlayerInputSensitivityControlValues(player);

			m_PlayerInputSettingsBoxes.at(player).ResetControlsButton->SetText("Reset");
			g_GUISound.ExitMenuSound()->Play();
		}
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void SettingsInputGUI::SetPlayerNextOrPrevInputDevice(int player, bool nextDevice) {
		int currentDevice = static_cast<int>(g_UInputMan.GetControlScheme(player)->GetDevice());

		if (nextDevice) {
			currentDevice++;
			if (currentDevice >= InputDevice::DEVICE_COUNT) { currentDevice = InputDevice::DEVICE_KEYB_ONLY; }
		} else {
			currentDevice--;
			if (currentDevice < InputDevice::DEVICE_KEYB_ONLY) { currentDevice = InputDevice::DEVICE_GAMEPAD_4; }
		}
		g_UInputMan.GetControlScheme(player)->SetDevice(static_cast<InputDevice>(currentDevice));
		UpdatePlayerSelectedDeviceLabel(player);
		ShowPlayerInputDeviceSensitivityControls(player);
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void SettingsInputGUI::UpdatePlayerSelectedDeviceLabel(int player) {
		std::string deviceLabel;

		switch (g_UInputMan.GetControlScheme(player)->GetDevice()) {
			case InputDevice::DEVICE_KEYB_ONLY:
				deviceLabel = "Keyboard Only";
				break;
			case InputDevice::DEVICE_MOUSE_KEYB:
				deviceLabel = "Mouse + Keyboard";
				break;
			case InputDevice::DEVICE_GAMEPAD_1:
				deviceLabel = "Gamepad 1";
				break;
			case InputDevice::DEVICE_GAMEPAD_2:
				deviceLabel = "Gamepad 2";
				break;
			case InputDevice::DEVICE_GAMEPAD_3:
				deviceLabel = "Gamepad 3";
				break;
			case InputDevice::DEVICE_GAMEPAD_4:
				deviceLabel = "Gamepad 4";
				break;
			default:
				break;
		}
		m_PlayerInputSettingsBoxes.at(player).SelectedDeviceLabel->SetText(deviceLabel);
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void SettingsInputGUI::ShowPlayerInputDeviceSensitivityControls(int player) {
		m_PlayerInputSettingsBoxes.at(player).SensitivtyLabel->SetVisible(false);
		m_PlayerInputSettingsBoxes.at(player).SensitivitySlider->SetVisible(false);
		m_PlayerInputSettingsBoxes.at(player).DeadZoneControlsBox->SetVisible(false);

		switch (g_UInputMan.GetControlScheme(player)->GetDevice()) {
			case InputDevice::DEVICE_KEYB_ONLY:
				break;
			case InputDevice::DEVICE_MOUSE_KEYB:
				// Mouse sensitivity doesn't seem to really work so keep the controls disabled for now, also it's shared between all mouse+keyboard using players.
				/*
				m_PlayerInputSettingsBoxes.at(player).SensitivtyLabel->SetVisible(true);
				m_PlayerInputSettingsBoxes.at(player).SensitivitySlider->SetVisible(true);
				m_PlayerInputSettingsBoxes.at(player).SensitivitySlider->SetMaximum(100);
				*/
				break;
			default:
				m_PlayerInputSettingsBoxes.at(player).SensitivtyLabel->SetVisible(true);
				m_PlayerInputSettingsBoxes.at(player).SensitivitySlider->SetVisible(true);
				m_PlayerInputSettingsBoxes.at(player).SensitivitySlider->SetMaximum(50);
				m_PlayerInputSettingsBoxes.at(player).DeadZoneControlsBox->SetVisible(true);
				break;
		}
		UpdatePlayerInputSensitivityControlValues(player);
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void SettingsInputGUI::UpdatePlayerInputSensitivityControlValues(int player) {
		switch (g_UInputMan.GetControlScheme(player)->GetDevice()) {
			case InputDevice::DEVICE_KEYB_ONLY:
				break;
			case InputDevice::DEVICE_MOUSE_KEYB:
				// Mouse sensitivity is shared between all players
				for (int otherPlayer = Players::PlayerOne; otherPlayer < Players::MaxPlayerCount; ++otherPlayer) {
					if (g_UInputMan.GetControlScheme(otherPlayer)->GetDevice() == InputDevice::DEVICE_MOUSE_KEYB) {
						m_PlayerInputSettingsBoxes.at(otherPlayer).SensitivitySlider->SetValue(static_cast<int>(g_UInputMan.GetMouseSensitivity() * 100));
						m_PlayerInputSettingsBoxes.at(otherPlayer).SensitivtyLabel->SetText("Mouse Sensitivity: " + std::to_string(m_PlayerInputSettingsBoxes.at(otherPlayer).SensitivitySlider->GetValue()));
					}
				}
				break;
			default:
				m_PlayerInputSettingsBoxes.at(player).SensitivitySlider->SetValue(static_cast<int>(g_UInputMan.GetControlScheme(player)->GetJoystickDeadzone() * 250));
				m_PlayerInputSettingsBoxes.at(player).SensitivtyLabel->SetText("Stick Deadzone: " + std::to_string(m_PlayerInputSettingsBoxes.at(player).SensitivitySlider->GetValue()));

				if (g_UInputMan.GetControlScheme(player)->GetJoystickDeadzoneType() == DeadZoneType::CIRCLE) {
					m_PlayerInputSettingsBoxes.at(player).CircleDeadZoneRadioButton->SetCheck(true);
				} else {
					m_PlayerInputSettingsBoxes.at(player).SquareDeadZoneRadioButton->SetCheck(true);
				}
				break;
		}
	}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void SettingsInputGUI::HandleInputEvents(GUIEvent &guiEvent) {
		for (int player = Players::PlayerOne; player < Players::MaxPlayerCount; ++player) {
			if (guiEvent.GetType() == GUIEvent::Command) {
				if (guiEvent.GetControl() == m_PlayerInputSettingsBoxes.at(player).NextDeviceButton) {
					g_GUISound.ButtonPressSound()->Play();
					SetPlayerNextOrPrevInputDevice(player, true);
				} else if (guiEvent.GetControl() == m_PlayerInputSettingsBoxes.at(player).PrevDeviceButton) {
					g_GUISound.ButtonPressSound()->Play();
					SetPlayerNextOrPrevInputDevice(player, false);
				} else if (guiEvent.GetControl() == m_PlayerInputSettingsBoxes.at(player).ConfigureControlsButton) {
					g_GUISound.ButtonPressSound()->Play();
					m_InputConfigMenu->SetEnabled(true, player);
				} else if (guiEvent.GetControl() == m_PlayerInputSettingsBoxes.at(player).ResetControlsButton) {
					g_GUISound.ButtonPressSound()->Play();
					ResetPlayerInputSettings(player);
				}
			} else if (guiEvent.GetType() == GUIEvent::Notification) {
				if (guiEvent.GetControl() == m_PlayerInputSettingsBoxes.at(player).SensitivitySlider) {
					if (g_UInputMan.GetControlScheme(player)->GetDevice() == InputDevice::DEVICE_MOUSE_KEYB) {
						g_UInputMan.SetMouseSensitivity(static_cast<float>(m_PlayerInputSettingsBoxes.at(player).SensitivitySlider->GetValue()) / 100.0F);
					} else {
						g_UInputMan.GetControlScheme(player)->SetJoystickDeadzone(static_cast<float>(m_PlayerInputSettingsBoxes.at(player).SensitivitySlider->GetValue()) / 250.0F);
					}
					UpdatePlayerInputSensitivityControlValues(player);
				} else if (guiEvent.GetControl() == m_PlayerInputSettingsBoxes.at(player).CircleDeadZoneRadioButton && guiEvent.GetMsg() == GUIRadioButton::Pushed) {
					g_UInputMan.GetControlScheme(player)->SetJoystickDeadzoneType(DeadZoneType::CIRCLE);
					UpdatePlayerInputSensitivityControlValues(player);
				} else if (guiEvent.GetControl() == m_PlayerInputSettingsBoxes.at(player).SquareDeadZoneRadioButton && guiEvent.GetMsg() == GUIRadioButton::Pushed) {
					g_UInputMan.GetControlScheme(player)->SetJoystickDeadzoneType(DeadZoneType::SQUARE);
					UpdatePlayerInputSensitivityControlValues(player);
				}
			}
			m_InputConfigMenu->HandleInputEvents(guiEvent, player);
		}
	}
}