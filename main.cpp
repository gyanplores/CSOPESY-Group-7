#include <iostream>
#include "mainMenu.h"

int main(){
    mainMenu menu;
    menu.setupCommands();
    menu.run();
        
    return 0;
    
}