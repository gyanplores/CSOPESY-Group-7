#include <iostream>
#include <string>
#include <Windows.h>
#include <cstdlib>
#include <conio.h>
#include <mutex>
#include <chrono>
#include <atomic>
#include <thread>
#include <sstream>

//  To compile: g++ -std=c++20 marquee.cpp -o marqueeApp
//  To run (in VSCode): .\marqueeApp

//  Global variable
std::atomic<bool> is_running{true};
std::atomic<bool> animation{false};
std::atomic<bool> help_check{false};
std::atomic<int> print_sleep{70};
std::string input_string;
std::string print_text = "CSOPESY Testing!";
std::mutex worker_mutex;           
                                  

//  Functions for the thread
void set_cursor(int posX, int posY){                                                    //  Function to set the cursor position for inputs
    COORD coord;                                                                        
    coord.X = posX;                                                                     
    coord.Y = posY;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
};

void set_cursor(int &posX, bool &directX){                                              //  Function to set cursor position for the text animation
    COORD coord;                                                                        //  Each loop in the animation thread changes the X coordinates
    coord.X = posX;
    coord.Y = 7;                                                                        
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);                   //  X and Y are changed loop which causes the shift in direction
    if(posX == 0 || posX == 93){                                                        //  If directX is false (going left) subtract 1 to posX
        directX = !directX;                                                             //  If directY is true (going down) add 1 to posY
    } 
    if(directX == true){
        posX++;
    }else{
        posX--;
    }
}

void print_design(){                                                                    //  Thread's print function
    int x =0;
    bool directX = false;
    
    while(is_running){
        std::cout << "Welcome to CSOpesy!\n\n";
        std::cout << "Developers: \n";
        std::cout << "Diamante, David\nFlores, Giancarlo\nFrancisco, Jacob";
        if(animation){                                                                  //  Starts or stops the animation
            set_cursor(x, directX);
        }else{
            set_cursor(0, 7);
        }
        std::cout << print_text;

        set_cursor(0, 15);
        if(help_check){                                                                 //  Enables the help comand list
            std::cout << "Command List:\n";
            std::cout << "start_marquee - starts the text animation\n";
            std::cout << "stop_marquee - stops the text animation\n";
            std::cout << "set_text <string> - sets the text of the animation\n";
            std::cout << "set_speed <int> - sets the speed of the animation\n";
            std::cout << "exit - closes the application\n\n";
        }
        std::cout << "Type a Command: " << input_string;                                //  Shows the inputs 

        std::this_thread::sleep_for(std::chrono::milliseconds(print_sleep));            //  Changes the sleep in milliseconds (default 70ms)
        system("cls");
    }
};


//  Main Function (Keyboard handler and Command Interpreter)
int main(){
    char ch;
    std::string text;
    std::string command;
    std::thread printer(print_design);                                                  //  Enables the print thread to run with the print_design function

    while(is_running){
        if(_kbhit()){
            std::lock_guard<std::mutex> lock(worker_mutex);                             //  Lock guard automatically locks and releases the lock when the code segment is done
            ch = _getch();

            if(ch == '\r'){                                                             //  Detects enter input
                std::stringstream split(input_string);                                  //  Tool for text parsing
                split >> command;                                                       //  Parses the first word as a command
                
                if(command == "help"){
                    help_check = true;
                }else
                if(command == "start_marquee"){
                    help_check = false;
                    animation = true;
                }else
                if(command == "set_text"){
                    help_check = false;
                    std::getline(split, text);
                    text.erase(0, 1); 

                    print_text = text;
                }else
                if(command == "set_speed"){
                    help_check = false;
                    std::getline(split, text);
                    text.erase(0, 1); 

                    print_sleep = std::stoi(text);
                }else
                if(input_string == "stop_marquee"){
                    help_check = false;
                    animation = false;
                }
                else
                if(command == "exit"){
                    animation = false;
                    is_running = false;
                }
                input_string.clear(); 
                text.clear();
            }else{
                input_string += ch;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(30));                     //  Changes the sleep duration of the KB handler and interpreter

    }

    printer.join();
    std::cout << "\n\nExiting the app..... Thank you!";
    return 0;
};