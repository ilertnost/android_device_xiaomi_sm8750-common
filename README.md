#  Common device configuration for Xiaomi 8750 family

Currently supported devices
- Xiaomi 15 (Dada)
- Xiaomi 15 Pro (Haotian)
- Xiaomi 15 Ultra (Xuanyuan)
- Xiaomi Redmi K80 Pro / Poco F7 Ultra (Miro)
- Xiaomi Mix Flip 2 (Bixi)

Might (or might not) support 
- Xiaomi Redmi Turbo 4 Pro (Onyx)
- Xiaomi Civi 5 Pro (Luming)

## Face unlock on Xiaomi 15 (Dada)

Face unlock uses crDroid's Paranoid Sense (`FaceUnlock`), including its enrollment
activity, Settings resource overlay, and system_ext feature declaration. The stock
Xiaomi mifaced/MI-TEE face authentication stack and its QVirt/qcrosvm runtime are
not shipped. No changes to the Settings application are required.