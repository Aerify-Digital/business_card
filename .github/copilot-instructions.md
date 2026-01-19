# Copilot Instructions for the Aerify Digital Business Card Firmware Repository

This project is C++ firmware for a digital business card, using PlatformIO, FreeRTOS, and the Raspberry Pi Pico SDK.

- Use C++17 or later.
- Follow the code style in the repository: snake_case for variables and functions, PascalCase for types.

- Avoid dynamic memory allocation in real-time tasks.

- All inter-task messages must use the Message_t struct and usbQueue. All output should go through usbQueue.
- Use LogLevel_t for logging, and always set the level field in Message_t.
- Message bodies must always be 128 characters or fewer.

- Use FreeRTOS primitives (tasks, queues, semaphores, types) for all concurrency. Do not use standard C++ threads.

- Prefer standard C++ containers like std::vector when possible, but ensure compatibility with embedded constraints.

- Include all necessary headers for Pico SDK and hardware features in main.h.

- Place all pin definitions only in pindefs.h.

- Add comments for hardware interactions and any task logic that is too complex to be self-documenting. Otherwise, write clear, maintainable, self-documenting code.
- Use debug.h for all debug output.
