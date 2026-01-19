# Instructions for agents

## Dev environment tips

- Use PlatformIO for building and uploading firmware.
- Run `pio run` to build the project and `pio run -t upload` to flash the device.
- Keep all pin definitions in pindefs.h and hardware includes in main.h.
- Use FreeRTOS primitives for all concurrency; do not use std::thread.
- Avoid dynamic memory allocation in real-time tasks.

## Testing instructions

- Check for CI plans in the .github/workflows folder.
- Run all unit and integration tests before merging.
- Ensure message bodies in Message_t are 128 characters or fewer.
- Use debug.h for all debug output.
- Add or update tests for any code you change.

## PR instructions

- Title format: [business_card] <Title>
- Always run all tests and static analysis before committing.
- Clearly indicate if a PR contains agent-generated code or suggestions.
