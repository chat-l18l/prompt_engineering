/**
 * Finite State Machine Demo for Arduino
 * 
 * Demonstrates:
 * - Clear enum usage for states (state_t)
 * - Separation between event generation and state handling
 * - State machine only handles transitions, no GPIO operations
 */

// Define the states using enum
typedef enum {
    STATE_IDLE,
    STATE_RUNNING,
    STATE_PAUSED,
    STATE_ERROR
} state_t;

// Define events that can trigger state transitions
typedef enum {
    EVENT_START,
    EVENT_STOP,
    EVENT_PAUSE,
    EVENT_RESUME,
    EVENT_ERROR,
    EVENT_RESET,
    EVENT_NONE
} event_t;

// Current state variable
static state_t current_state = STATE_IDLE;

/**
 * State transition function
 * Only handles state transitions based on events
 * No GPIO reading/writing here
 */
state_t handle_event(state_t current, event_t event) {
    switch (current) {
        case STATE_IDLE:
            if (event == EVENT_START) {
                return STATE_RUNNING;
            } else if (event == EVENT_ERROR) {
                return STATE_ERROR;
            }
            break;
            
        case STATE_RUNNING:
            if (event == EVENT_PAUSE) {
                return STATE_PAUSED;
            } else if (event == EVENT_STOP) {
                return STATE_IDLE;
            } else if (event == EVENT_ERROR) {
                return STATE_ERROR;
            }
            break;
            
        case STATE_PAUSED:
            if (event == EVENT_RESUME) {
                return STATE_RUNNING;
            } else if (event == EVENT_STOP) {
                return STATE_IDLE;
            }
            break;
            
        case STATE_ERROR:
            if (event == EVENT_RESET) {
                return STATE_IDLE;
            }
            break;
            
        default:
            break;
    }
    
    // No transition, stay in current state
    return current;
}

/**
 * Event generator - reads GPIO and generates events
 * This is where all hardware interaction happens
 */
event_t generate_events() {
    static bool last_button_start = HIGH;
    static bool last_button_stop = HIGH;
    
    // Read buttons (GPIO operations happen here)
    bool button_start = digitalRead(2);  // Start button on pin 2
    bool button_stop = digitalRead(3);   // Stop button on pin 3
    
    // Detect button presses (rising edge)
    if (button_start == LOW && last_button_start == HIGH) {
        last_button_start = LOW;
        return EVENT_START;
    }
    
    if (button_stop == LOW && last_button_stop == HIGH) {
        last_button_stop = LOW;
        return EVENT_STOP;
    }
    
    // Update last state
    last_button_start = button_start;
    last_button_stop = button_stop;
    
    return EVENT_NONE;
}

/**
 * Output handler - writes to GPIO based on current state
 * All output operations happen here
 */
void update_outputs(state_t state) {
    switch (state) {
        case STATE_IDLE:
            digitalWrite(13, LOW);  // LED off
            break;
            
        case STATE_RUNNING:
            digitalWrite(13, HIGH); // LED on
            break;
            
        case STATE_PAUSED:
            // Blink LED slowly (simplified for demo)
            digitalWrite(13, millis() % 1000 < 500 ? HIGH : LOW);
            break;
            
        case STATE_ERROR:
            // Blink LED fast
            digitalWrite(13, millis() % 200 < 100 ? HIGH : LOW);
            break;
    }
}

void setup() {
    // Setup pins
    pinMode(2, INPUT_PULLUP);  // Start button
    pinMode(3, INPUT_PULLUP);  // Stop button
    pinMode(13, OUTPUT);       // Status LED
    
    Serial.begin(9600);
    Serial.println("FSM Demo Started");
}

void loop() {
    // 1. Generate events from hardware inputs
    event_t event = generate_events();
    
    // 2. Process event through state machine (pure logic, no I/O)
    state_t new_state = handle_event(current_state, event);
    
    // 3. Check if state changed
    if (new_state != current_state) {
        Serial.print("State changed: ");
        Serial.print(current_state);
        Serial.print(" -> ");
        Serial.println(new_state);
        current_state = new_state;
    }
    
    // 4. Update outputs based on current state
    update_outputs(current_state);
    
    // Small delay for debouncing
    delay(50);
}
