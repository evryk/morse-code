#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h" 
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "hardware/watchdog.h"
#include "assign02.pio.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pico/float.h"
#include "pico/double.h"
#include <math.h>

/** \brief
 --- FUNCTION DECLARATIONS --- 
 */

void add_to_sequence();
void game_stats();
void handler();
void print_welcome();
void won_game();
void game_over();
void incorrect();
void correct();
void level_complete();
void print_question();

/** \brief
Declare the main assembly code entry point. 
*/


void main_asm();

/** @brief
 Initialise a GPIO pin – see SDK for detail on gpio_init() 
 */
void asm_gpio_init(uint pin) { 
    gpio_init(pin); 
} 

/** @brief
 Set direction of a GPIO pin – see SDK for detail on gpio_set_dir() 
 */
void asm_gpio_set_dir(uint pin, bool out) { 
    gpio_set_dir(pin, out); 
}

/** @brief
 Enable falling-edge interrupt – see SDK for detail on gpio_set_irq_enabled() 
 */
void asm_gpio_set_irq(uint pin) { 
    gpio_set_irq_enabled(pin, GPIO_IRQ_EDGE_FALL, true); 
    gpio_set_irq_enabled(pin, GPIO_IRQ_EDGE_RISE, true);
}

/** @brief
 Reset watchdog each time there's an input
 */
void watchdog_enable (uint32_t delay_ms, bool pause_on_debug);


/** @brief 
define game variables and statistics
*/
#define MAX_LIVES 3
#define GOAL_TO_WIN 5

///
/// Game Variables
///
int rand_index;
int level = 0;
int correct_count = 0 ;
int remaining_lives = MAX_LIVES;
int game_started = 0;

///
/// Game stats
///
int total_correct = 0;
int total_incorrect = 0;

/**
    @brief defining the sequence of dots and dashes relevant for morse code for all alphanumeric characters, as well as their corresponding regular characters at the same relative index.
*/
char LETTERS_MORSE[36][6] = {
    // A - Z (0-25)
    ".-\0", "-...\0", "-.-.\0", "-..\0", ".\0", "..-.\0", "--.\0", "....\0", "..\0", 
    ".---\0", "-.-\0", ".-..\0", "--\0", "-.\0", "---\0", ".--.\0", "--.-\0", ".-.\0", 
    "...\0", "-\0", "..-\0", "...-\0", ".--\0", "-..-\0", "-.--\0", "--..\0",
    // Digits 0 - 9 (26-35)
    "-----\0", ".----\0", "..---\0", "...--\0", "....-\0", 
    "....\0", "-...\0", "--...\0", "---..\0", "----.\0"
};

char LETTERS_ALPHANUM[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
    'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
    'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0',
    '1', '2', '3', '4', '5', '6', '7', '8', '9' 
};

/**
    @brief defining the morse words and their alphanumeric counterparts for levels 3 and 4.
*/
char WORDS_MORSE[50][50] = {
    ".-- .. -. -..\0",
    "-... --- .-.. -..\0",
    ".... --- .--. .\0",
    "..-. . .- .-.\0",
    "- --- .-. -.\0",
    "..-. .- ... -\0",
    ".-- .. ... .\0",
    "- . .- --\0",
    ".-.. --- ..- -..\0",
    "-.-. --- .-.. -..\0"
}; 

char WORDS_ALPHANUM[50][50] = {
    "WIND\0", "BOLD\0", "HOPE\0", "FEAR\0", "TORN\0", 
    "FAST\0", "WISE\0", "TEAM\0", "LOUD\0", "COLD\0"
};



/**
    @brief defining led variables which will be used to change the color based on the game state.
*/
#define IS_RGBW true        // Will use RGBW format
#define NUM_PIXELS 1        // There is 1 WS2812 device in the chain
#define WS2812_PIN 28       // The GPIO pin that the WS2812 connected t

// LED Functions
static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

/**
    @brief The function to change the color based on the game state.

    @details The function checks how many lives are remaining in the game state, and changes the color to the appropriate one using the previously defined functions.
    @param[in]      game_started    A boolean-ish variable that determines the game state (on or off)
    @param[in]      remaining_lives An int counter of the remaining lives in the game state.
*/
static inline void update_led() {
    if (game_started == 0) {
        // Set LED to BLUE once the game opens but hasnt started
        put_pixel(urgb_u32(0x00, 0x00, 0xFF));
    }
    else {
        if (remaining_lives >= 3)
        // GREEN
        put_pixel(urgb_u32(0x00, 0xFF, 0x00));

        if (remaining_lives == 2)
        // YELLOW
        put_pixel(urgb_u32(0xFF, 0xFF, 0x00));

        if (remaining_lives == 1)
        // ORANGE
        put_pixel(urgb_u32(0xFF, 0xA0, 0x11));

        if (remaining_lives == 0)
        // RED
        put_pixel(urgb_u32(0xFF, 0x00, 0x00));
    }
}


/**
    @brief defining variables to handle player input
*/
#define MAX_CHAR_INPUT 20
#define MAX_WORD_INPUT 200

int char_index = 0;
int seq_index = 0;
char input_buffer[MAX_CHAR_INPUT]; // Holds player input
char sequence[MAX_WORD_INPUT]; // Alphanum equivalent sequence input

/**
    @brief Function Call from ASM: Add dot or dash to buffer
*/
void add_input(int input) {
    if (char_index < MAX_CHAR_INPUT - 2) {
        if (input == 0) {
            printf(".");
            input_buffer[char_index] = '.';
        }
        else if (input == 1) {
            printf("-");
            input_buffer[char_index] = '-';
        }
        char_index++;
    }
    watchdog_enable(9000, 1);
}

/**
    @brief Function Call from ASM: Character input finished
*/
void end_char() {
    if (char_index < MAX_CHAR_INPUT - 1) {
        // NULL terminator (space equivalent)
        input_buffer[char_index] = '\0';
        // Reset char_index
        char_index = 0;

        // Reset watchdog
        watchdog_enable(9000, 1);

        printf(" ");

        // Add char buffer to sequence
        add_to_sequence();
    }
}

/**
    @brief Add input buffer character to alphanum sequence

    @details Takes the user input and appends its morse equivalent to an input buffer.
    @param          input_buffer[out]   a placeholder for the finalized string in alphanumerics.
*/
void add_to_sequence() {
    bool found = true;

    // Find character in LETTERS_MORSE
    for (int i = 0; i < 36; i++) {
        found = true;
        for (int j = 0; j < 6; j++) {
            // Check if character is found
            if (input_buffer[j] != LETTERS_MORSE[i][j]) {
                found = false;
                break;
            }
            // Make sure not to include the NULL terminator
            if (input_buffer[j] == '\0' || LETTERS_MORSE[i][j] == '\0') break;
        }
        if (found) {
            // Add char to sequence if found
            if (char_index < MAX_WORD_INPUT - 2) {
                sequence[seq_index] = LETTERS_ALPHANUM[i];
                seq_index++;
            }
            return;
        }
    }
    // Character not found!!! Add a '?'
    if (char_index < MAX_WORD_INPUT - 2) {
        sequence[seq_index] = '?';
        seq_index++;
    }
    return; 
}

/**
    @brief Function Call from ASM: Word input finished -> check whether input is correct
*/
void end_word() {
    // NULL terminator to input string sequence
    if (seq_index < MAX_WORD_INPUT - 1) 
        sequence[seq_index] = '\0';

    watchdog_enable(9000, 1);

    printf("\n\n");
    handler();

    // Reset indices
    char_index = 0;
    seq_index = 0;
}



/**
    @brief init the game
*/
int random_letter() {
    return rand() % 36;
}
int random_word() {
    return rand() % 10;
}


/**
    @brief handles the games logic

    @details Determines the game state and selects the appropriate challenge (character/word) from the initialized arrays
    @param      level[in]       Game state of the player
    @param      sequence[in]    the sequence of alphanumeric characters selected by the game
    @param      correct_count   The goal of the player
*/
void handler() {
    if (game_started) {
        // Check input against generated question
        bool found;
        if (level == 1 || level == 2) {
            if (sequence[0] == LETTERS_ALPHANUM[rand_index] && seq_index < 3) found = true;
        } 
        else if (level == 3 || level == 4) {
            for (int i = 0; i < 10; i++) {
                found = true;
                // Check if character is found
                if (sequence[i] != WORDS_ALPHANUM[rand_index][i]) {
                    found = false;
                    break;
                }
                // Make sure not to include the NULL terminator
                if (sequence[i] == '\0' || WORDS_ALPHANUM[rand_index][i] == '\0') break;
            }
        }

        if (found) {
            // User got it correct
            correct_count++;
            total_correct++;
            correct();
        
            // Check if user won the level
            if (correct_count >= GOAL_TO_WIN) {
                level_complete();
                correct_count = 0;
                total_correct = 0;
                total_incorrect = 0;

                if (level == 4) {
                    won_game();
                    remaining_lives = MAX_LIVES;
                    game_started = 0;
                    print_welcome();
                }

                level++;
            }
            if (remaining_lives < MAX_LIVES) remaining_lives++;
            print_question();
        }
        else {
            // User got it incorrect
            incorrect();
            remaining_lives--;
            correct_count = 0;
            total_incorrect++;

            // Check if user died
            if (remaining_lives <= 0) {
                // Reset game
                remaining_lives = MAX_LIVES;
                game_started = 0;
                game_over();
                total_correct = 0;
                total_incorrect = 0;
                print_welcome();
            }
            else {
                print_question();
            }
        }
    } 
    else if (!game_started) {
        // Home Screen
        if (seq_index < 3 && seq_index > 0) {
            if (sequence[0] > '0' && sequence[0] < '5') {
                // Make input level number into an integer
                level = sequence[0] - 0x30;
                // Reset for new level input
                correct_count = 0;
                remaining_lives = MAX_LIVES;
                game_started = 1;

                print_question();
            }
        } 
    }

    update_led();
}



/**
    @brief print statements for the console GUI
*/
void print_welcome() {
    update_led();
    printf("\n\n");
    printf("███╗   ███╗ ██████╗ ██████╗ ███████╗███████╗     ██████╗ ██████╗ ██████╗ ███████╗\n");
    printf("████╗ ████║██╔═══██╗██╔══██╗██╔════╝██╔════╝    ██╔════╝██╔═══██╗██╔══██╗██╔════╝\n");
    printf("██╔████╔██║██║   ██║██████╔╝███████╗█████╗      ██║     ██║   ██║██║  ██║█████╗  \n");
    printf("██║╚██╔╝██║██║   ██║██╔══██╗╚════██║██╔══╝      ██║     ██║   ██║██║  ██║██╔══╝  \n");
    printf("██║ ╚═╝ ██║╚██████╔╝██║  ██║███████║███████╗    ╚██████╗╚██████╔╝██████╔╝███████╗\n");
    printf("╚═╝     ╚═╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝╚══════╝     ╚═════╝ ╚═════╝ ╚═════╝ ╚══════╝\n");
    printf("                                                                                 \n");                                                                          
    printf("+-------------------------------------------------------------------------------+\n");
    printf("|                    Welcome to our tutorial on Morse Code!                     |\n");
    printf("+-------------------------------------------------------------------------------+\n");
    printf("|                               We are group 3!                                 |\n");
    printf("+-------------------------------------------------------------------------------+\n");
    printf("|                Using GP21, enter a sequence to choose a level:                |\n");
    printf("+-------------------------------------------------------------------------------+\n");
    printf("|                        [.----] LEVEL 1 - EASY MODE                            |\n");
    printf("|                        [..---] LEVEL 2 - MEDIUM MODE                          |\n");
    printf("|                        [...--] LEVEL 3 - HARD MODE                            |\n");
    printf("|                        [....-] LEVEL 4 - EXPERT MODE                          |\n");
    printf("+-------------------------------------------------------------------------------+\n");
    printf("|          Use short presses (<0.25s) for a '.', any longer will be a '-'.      |\n");
    printf("|                   Enter the character displayed in morse,                     |\n");
    printf("|            you'll procede to the next level after 5 correct inputs!           |\n");
    printf("|       You start with 3 lives, you can lose or win lives during the game!      |\n");
    printf("|           We'll keep track of your remaining lives for you. ENJOY!            |\n");
    printf("+-------------------------------------------------------------------------------+\n\n\n");
}

void won_game() {
    printf("\n");
    printf("██╗   ██╗ ██████╗ ██╗   ██╗    ██╗    ██╗ ██████╗ ███╗   ██╗    ██╗\n");
    printf("╚██╗ ██╔╝██╔═══██╗██║   ██║    ██║    ██║██╔═══██╗████╗  ██║    ██║\n");
    printf(" ╚████╔╝ ██║   ██║██║   ██║    ██║ █╗ ██║██║   ██║██╔██╗ ██║    ██║\n");
    printf("  ╚██╔╝  ██║   ██║██║   ██║    ██║███╗██║██║   ██║██║╚██╗██║    ╚═╝\n");
    printf("   ██║   ╚██████╔╝╚██████╔╝    ╚███╔███╔╝╚██████╔╝██║ ╚████║    ██╗\n");
    printf("   ╚═╝    ╚═════╝  ╚═════╝      ╚══╝╚══╝  ╚═════╝ ╚═╝  ╚═══╝    ╚═╝\n\n");
    printf("            CONGRATULATIONS ON FINISHING THE GAME !!!!\n\n");
}

void game_over() {
    printf("\n");
    printf(" ██████╗  █████╗ ███╗   ███╗███████╗     ██████╗ ██╗   ██╗███████╗██████╗     ██╗\n");
    printf("██╔════╝ ██╔══██╗████╗ ████║██╔════╝    ██╔═══██╗██║   ██║██╔════╝██╔══██╗    ██║\n");
    printf("██║  ███╗███████║██╔████╔██║█████╗      ██║   ██║██║   ██║█████╗  ██████╔╝    ██║\n");
    printf("██║   ██║██╔══██║██║╚██╔╝██║██╔══╝      ██║   ██║╚██╗ ██╔╝██╔══╝  ██╔══██╗    ╚═╝\n");
    printf("╚██████╔╝██║  ██║██║ ╚═╝ ██║███████╗    ╚██████╔╝ ╚████╔╝ ███████╗██║  ██║    ██╗\n");
    printf(" ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝     ╚═════╝   ╚═══╝  ╚══════╝╚═╝  ╚═╝    ╚═╝\n\n");
    printf("                                RESTARTING...\n\n");
    game_stats();
}

void incorrect() {
    printf("\n");
    printf("+-----------------------------------------------+\n");
    printf("|                INCORRECT ANSWER:              |\n");
    printf("|           The sequence doesn't match.         |\n");
    printf("+-----------------------------------------------+\n\n");
}

void correct() {
    printf("\n");
    printf("+-----------------------------------------------+\n");
    printf("|                 Congratulations!              |\n");
    printf("|       CORRECT ANSWER. On to the next one!     |\n");
    printf("+-----------------------------------------------+\n\n");
}

void level_complete() {
    printf("\n");
    printf("  _                _                             _      _\n");
    printf(" | | _____   _____| |   ___ ___  _ __ ___  _ __ | | ___| |_ ___\n");
    printf(" | |/ _ \\ \\ / / _ \\ |  / __/ _ \\| '_ ` _ \\| '_ \\| |/ _ \\ __/ _ \\ \n");
    printf(" | |  __/\\ V /  __/ | | (_| (_) | | | | | | |_) | |  __/ ||  __/\n");
    printf(" |_|\\___| \\_/ \\___|_|  \\___\\___/|_| |_| |_| .__/|_|\\___|\\__\\___|\n");
    printf("                                          |_|\n\n");
    game_stats();
}

/**
    @brief print statements for the statistics of the game
*/
void game_stats() {
    // Statistics at the end of each level that is successfully completed 
    // or when the player runs out of lives
    double accuracy = ((double)total_correct / (double)(total_correct + total_incorrect)) * 100.00;
    double integral;
    double fractional;
    fractional = modf(accuracy, &integral);
    int fractional_int = (int)(fractional * 100);

    char temp_str[50];
    sprintf(temp_str, "|             Total accuracy: %d.%d %%", (int)integral, fractional_int);

    printf("+-----------------------------------------------+\n");
    printf("|            GAME STATS for this level:         |\n");
    printf("|               %d correct answers.              |\n", total_correct);
    printf("|               %d incorrect answers.            |\n", total_incorrect);
    printf("%-45s   |\n", temp_str);
    printf("+-----------------------------------------------+\n\n");
}

/**
    @brief print statement for the question currently on.
*/
void print_question() {
    if (level == 1 || level == 2) {
        rand_index = random_letter();
    } else if (level == 3 || level == 4) {
        rand_index = random_word();
    } else return;

    printf("+------------------------------------------------+\n");
    printf("|                                                |\n");
    printf("|                   GAME STATUS:                 |\n");
    printf("|                  Lives left: %d                 |\n", remaining_lives);
    printf("|        Current streak: %d correct answers       |\n", correct_count);   
    printf("|                                                |\n");
    printf("|           Enter the morse code for:            |\n");
    if (level > 2) printf("|                      %-10s                |\n", WORDS_ALPHANUM[rand_index]);
    else printf("|                        %c                       |\n", LETTERS_ALPHANUM[rand_index]);
    printf("|                                                |\n");
    if (level % 2 == 1) {
        if (level > 2) printf("|                Morse : %-24s|\n", WORDS_MORSE[rand_index]);
        else printf("|                Morse : %-24s|\n", LETTERS_MORSE[rand_index]);
    }
    printf("|                                                |\n");
    printf("+------------------------------------------------+\n\n");
}


/**
    @brief the main call, referencing the interrupts. 
*/
int main() { 
    stdio_init_all();              // Initialise all basic IO

    // Initialise the PIO interface with the WS2812 code
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, 0, offset, WS2812_PIN, 800000, IS_RGBW);
    watchdog_enable(9000, 1);

    main_asm();                    // Jump into the ASM code

    return 0;                      // Application return code 
} 

