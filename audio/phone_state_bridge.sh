#!/system/bin/sh
# Phone state bridge - syncs Java AudioService mode to native AudioPolicyManager
# Required because AudioSystem.setPhoneState() JNI fails on QTI AIDL audio HAL
LAST_STATE=0
while true; do
    MODE=$(dumpsys audio 2>/dev/null | grep "Requested mode" | head -1)
    if echo "$MODE" | grep -q "MODE_IN_CALL"; then
        if [ "$LAST_STATE" != "2" ]; then
            service call media.audio_policy 5 i32 2 i32 1000
            LAST_STATE=2
        fi
    elif echo "$MODE" | grep -q "MODE_IN_COMMUNICATION"; then
        if [ "$LAST_STATE" != "3" ]; then
            service call media.audio_policy 5 i32 3 i32 1000
            LAST_STATE=3
        fi
    else
        if [ "$LAST_STATE" != "0" ]; then
            service call media.audio_policy 5 i32 0 i32 1000
            LAST_STATE=0
        fi
    fi
    sleep 1
done
