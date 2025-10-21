Scriptname ImmersiveCastingVR_Helpers

Int Function IndexOfStr(String[] arr, String search) Global
    If !arr
        return -1
    EndIf

    Int count = arr.Length
    Int i = 0
    While i < count
        If arr[i] == search
            return i
        EndIf
        i += 1
    EndWhile

    return -1 ; not found
EndFunction