Scriptname ImmersiveCastingVR_MCM extends SKI_ConfigBase

import StringUtil
import ImmersiveCastingVR_Helpers

String[] Property InputMethodLabels Auto Hidden
String[] Property InputMethodValues Auto Hidden

Int Property SelectedInputMethod Auto Hidden
Bool Property ShowDoubleBindingWarning Auto Hidden
Bool Property InputEnabled Auto Hidden
Bool Property InputCastAfterMenuExit Auto Hidden
Bool Property HapticsEnabled Auto Hidden

Event OnConfigInit()
	SetName("Immersive Casting VR")
	InitializeData()
	UpdateFromConfig()
EndEvent

Event OnConfigOpen()
	UpdateFromConfig()
EndEvent

Event OnPageReset(String page)
	UpdateFromConfig()
	If page == "" || page == "Input"
		DrawInputPage()
	ElseIf page == "Haptics"
		DrawHapticsPage()
	EndIf
EndEvent

State ICVR_InputEnable
	Event OnSelectST()
		InputEnabled = ToggleSetting("InputEnable", InputEnabled)
		SetToggleOptionValueST(InputEnabled)
	EndEvent

	Event OnHighlightST()
		SetInfoText("Use Grip-based Spellcasting System. Disable to use vanilla inputs.")
	EndEvent
EndState

State ICVR_InputCastAfterMenuExit
	Event OnSelectST()
		InputCastAfterMenuExit = ToggleSetting("InputCastAfterMenuExit", InputCastAfterMenuExit)
		SetToggleOptionValueST(InputCastAfterMenuExit)
	EndEvent

	Event OnHighlightST()
		SetInfoText("Immediately resume casting after closing menus if the hand is in the casting position. If disabled hands have to be closed/opened once after leaving a menu.")
	EndEvent
EndState

State ICVR_InputMethod
	Event OnMenuOpenST()
		SetMenuDialogOptions(InputMethodLabels)
		SetMenuDialogStartIndex(SelectedInputMethod)
		SetMenuDialogDefaultIndex(SelectedInputMethod)
	EndEvent

	Event OnMenuAcceptST(Int index)
		SelectedInputMethod = index
		ImmersiveCastingVR_Config.SetString("CastingInputMethod", InputMethodValues[index])
		SetMenuOptionValueST(InputMethodLabels[index])
	EndEvent

	Event OnHighlightST()
		SetInfoText("Select which OpenVR input is used for spell casting. RECCOMMENDED: [Oculus] -> Grip Press, [Index/Knuckles] -> Grip Touch")
	EndEvent
EndState

State ICVR_ShowDoubleBindingWarning
	Event OnSelectST()
		ShowDoubleBindingWarning = !ShowDoubleBindingWarning
		ImmersiveCastingVR_Config.SetBool("ShowBindingWarning", ShowDoubleBindingWarning)
		SetToggleOptionValueST(ShowDoubleBindingWarning)
	EndEvent

	Event OnHighlightST()
		SetInfoText("Enable/Disable warnings if the Input Method is already used for a gameplay action.")
	EndEvent
EndState

State ICVR_HapticsEnable
	Event OnSelectST()
		HapticsEnabled = ToggleSetting("HapticsEnable", HapticsEnabled)
		SetToggleOptionValueST(HapticsEnabled)
	EndEvent

	Event OnHighlightST()
		SetInfoText("Enable dynamic spellcasting haptics. Will suppress other mods Spellcasting haptics.")
	EndEvent
EndState

Function InitializeData()
	If !Pages || Pages.Length != 2
		Pages = new String[2]
	EndIf
	Pages[0] = "Input"
	Pages[1] = "Haptics"

	If !InputMethodLabels || InputMethodLabels.Length != 2
		InputMethodLabels = new String[2]
		InputMethodLabels[0] = "Grip Press"
		InputMethodLabels[1] = "Grip Touch"
	EndIf

	If !InputMethodValues || InputMethodValues.Length != 2
		InputMethodValues = new String[2]
		InputMethodValues[0] = "grip"
		InputMethodValues[1] = "grip_touch"
	EndIf
EndFunction

Function UpdateFromConfig()
	SelectedInputMethod = ImmersiveCastingVR_Helpers.IndexOfStr(InputMethodValues, ImmersiveCastingVR_Config.GetString("CastingInputMethod"))
	ShowDoubleBindingWarning = ImmersiveCastingVR_Config.GetBool("ShowBindingWarning")
	InputEnabled = ImmersiveCastingVR_Config.GetBool("InputEnable")
	InputCastAfterMenuExit = ImmersiveCastingVR_Config.GetBool("InputCastAfterMenuExit")
	HapticsEnabled = ImmersiveCastingVR_Config.GetBool("HapticsEnable")
EndFunction

Function DrawInputPage()
	SetCursorFillMode(TOP_TO_BOTTOM)
	AddHeaderOption("Input Redirection")
	AddToggleOptionST("ICVR_InputEnable", "Enable Immersive Input", InputEnabled)
	AddToggleOptionST("ICVR_InputCastAfterMenuExit", "Cast after leaving Menus", InputCastAfterMenuExit)
	AddHeaderOption("Casting Controls")
	AddMenuOptionST("ICVR_InputMethod", "Immersive Input Method", InputMethodLabels[SelectedInputMethod])
	AddToggleOptionST("ICVR_ShowDoubleBindingWarning", "Show \"buttons already used\" Warning", ShowDoubleBindingWarning)
EndFunction

Function DrawHapticsPage()
	SetCursorFillMode(TOP_TO_BOTTOM)
	AddHeaderOption("Spellcasting Haptics")
	AddToggleOptionST("ICVR_HapticsEnable", "Enable dynamic Spellcasting Haptics", HapticsEnabled)
EndFunction

Bool Function ToggleSetting(String settingKey, Bool currentValue)
	Bool newValue = !currentValue
	ImmersiveCastingVR_Config.SetBool(settingKey, newValue)
	return newValue
EndFunction
