Scriptname ImmersiveCastingVR_MCM extends SKI_ConfigBase

import StringUtil
import ImmersiveCastingVR_Helpers

String[] Property InputMethodLabels Auto Hidden
String[] Property InputMethodValues Auto Hidden

Int Property SelectedInputMethod Auto Hidden
Bool Property ShowDoubleBindingWarning Auto Hidden

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
	If page == ""
		SetCursorFillMode(TOP_TO_BOTTOM)
		AddHeaderOption("Casting Controls")
		AddMenuOptionST("ICVR_InputMethod", "Input Method", InputMethodLabels[SelectedInputMethod])
		AddToggleOptionST("ICVR_ShowDoubleBindingWarning", "Show \"buttons already used\" Warning", ShowDoubleBindingWarning)
	EndIf
EndEvent

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
		SetInfoText("Select which OpenVR input triggers casting for both hands.")
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

Function InitializeData()
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
EndFunction
