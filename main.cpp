#include <iostream>
#include "mainMenu.h"

int main(){
    MainMenu menu;
    menu.setupCommands();
    menu.run();
        
    return 0;
    
}