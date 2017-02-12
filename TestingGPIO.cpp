// TestingGPIO.cpp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <sys/time.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include "jetsonGPIO.h"

using namespace std;

int getkey() {
    int character;
    struct termios orig_term_attr;
    struct termios new_term_attr;

    /* set the terminal to raw mode */
    tcgetattr(fileno(stdin), &orig_term_attr);
    memcpy(&new_term_attr, &orig_term_attr, sizeof(struct termios));
    new_term_attr.c_lflag &= ~(ECHO|ICANON);
    new_term_attr.c_cc[VTIME] = 0;
    new_term_attr.c_cc[VMIN] = 0;
    tcsetattr(fileno(stdin), TCSANOW, &new_term_attr);

    /* read a character from the stdin stream without blocking */
    /*   returns EOF (-1) if no character is available */
    character = fgetc(stdin);

    /* restore the original terminal attributes */
    tcsetattr(fileno(stdin), TCSANOW, &orig_term_attr);

    return character;
}

int main(int argc, char *argv[]){
    cout << "Testing the GPIO Pins" << endl;

    jetsonTX1GPIONumber ledRing1 = gpio219;     // Ouput maps to PIN# 29
    jetsonTX1GPIONumber ledRing2 = gpio186;     // Output maps to PIN# 31

    // Make the button and led available in user space
    gpioExport(ledRing1);
    gpioSetDirection(ledRing1, outputPin);
    gpioExport(ledRing2);
    gpioSetDirection(ledRing2, outputPin);

    // Reverse the button wiring; this is for when the button is wired
    // with a pull up resistor


    // Flash the LED 5 times
    for(int i=0; i<5; i++){
        cout << "Setting the LED on" << endl;
        gpioSetValue(ledRing1, on);
        gpioSetValue(ledRing2, on);
        usleep(200000);         // on for 200ms
        cout << "Setting the LED off" << endl;
        gpioSetValue(ledRing1, off);
        gpioSetValue(ledRing2, off);
        usleep(200000);         // off for 200ms
    }

    // Wait for the push button to be pressed
    cout << "Please press the button! ESC key quits the program" << endl;

    unsigned int value = low;
    int ledValue = low ;
    // Turn off the LED
    gpioSetValue(ledRing1, low);
    gpioSetValue(ledRing2, low);
    while(getkey() != 27) {
        /*
        // Useful for debugging
        // cout << "Button " << value << endl;
        if (value==high && ledValue != high) {
            // button is pressed ; turn the LED on
            ledValue = high ;
            gpioSetValue(ledRing1, on);
            gpioSetValue(ledRing2, on);
        } else {
            // button is *not* pressed ; turn the LED off
            if (ledValue != low) {
                ledValue = low ;
                gpioSetValue(ledRing1, off);
                gpioSetValue(ledRing2, off);
            }

        }
        */
        usleep(1000); // sleep for a millisecond
    }

    cout << "GPIO example finished." << endl;
    gpioUnexport(ledRing1);     // unexport the LED
    gpioUnexport(ledRing2);     // unexport the LED
    return 0;
}
