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

## Source patches

Changes to other projects are carried as `git format-patch` files under
`patches/<project>/` in this tree and in the trees listed by the device's
`vendorsetup.sh`, where `<project>` is the project path with `/` replaced by `_`.
A project's patch numbers form one sequence across those trees.

`source build/envsetup.sh` runs the device's `vendorsetup.sh`, which applies
them with `patches/apply-patches.sh` on top of each project's manifest revision.
Projects that already contain the patches are left alone, as are projects with
other local changes, which are reported instead. `repo sync` drops the applied
commits; the next `source build/envsetup.sh` applies them again.

## Face unlock on Xiaomi 15 (Dada)

Face unlock uses crDroid's Paranoid Sense (`FaceUnlock`), including its enrollment
activity, Settings resource overlay, and system_ext feature declaration. The stock
Xiaomi mifaced/MI-TEE face authentication stack and its QVirt/qcrosvm runtime are
not shipped. No changes to the Settings application are required.