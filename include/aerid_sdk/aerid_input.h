#ifndef AERID_INPUT_H
#define AERID_INPUT_H
#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        AERID_BUTTON_UP,
        AERID_BUTTON_DOWN,
        AERID_BUTTON_LEFT,
        AERID_BUTTON_RIGHT,
        AERID_BUTTON_CENTER,
        AERID_BUTTON_A,
        AERID_BUTTON_B,
        AERID_INPUT_COUNT
    } aerid_input_t;

    typedef enum
    {
        AERID_INPUT_EVENT_PRESS,   // initial press
        AERID_INPUT_EVENT_RELEASE, // release
        AERID_INPUT_EVENT_REPEAT   // held down
    } aerid_input_event_t;

    // Poll the state of the specified input
    // Returns 1 if pressed, 0 if not pressed
    int aerid_input_read(aerid_input_t input);

    // Callback function to handle input events
    typedef void (*aerid_input_callback_t)(aerid_input_t input, aerid_input_event_t event);

    // Register a callback function for a specific input
    void aerid_input_register_callback(aerid_input_t input, aerid_input_callback_t callback);

    // Clear all registered input callbacks
    void aerid_input_clear_callbacks();

    // Clear registered callback for a specific input
    void aerid_input_clear_callback(aerid_input_t input);

#ifdef __cplusplus
}
#endif
#endif