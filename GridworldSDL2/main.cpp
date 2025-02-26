#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <vector>
#include <cstdlib>

#define KEY_ESC   27        // Escape Key

std::mutex mtx;
std::vector<std::string> walkIntoWallMsg = {
    "ARE YOU BLIND?? THAT'S A WALL!",
    "We Reeeeeally walking into walls huh?",
    "Ooft walked into a wall.",
    "That's a wall! Should've gone to specsavers!"
};

// Cheat
bool seeDeathTiles = true;
// Game State
bool endGame = false;
char userInput;
int playerPos[2] = { 6, 3 };

char myArray[8][8] = {
    {'#', '#', '#', '#', '#', '#', '#', '#'},
    {'#', '#', ' ', 'D', 'G', 'D', ' ', '#'},
    {'#', ' ', ' ', ' ', ' ', ' ', ' ', '#'},
    {'#', 'D', ' ', ' ', '#', 'D', ' ', '#'},
    {'#', ' ', ' ', ' ', '#', ' ', ' ', '#'},
    {'#', '#', ' ', '#', '#', '#', ' ', '#'},
    {'#', 'D', ' ', ' ', ' ', ' ', ' ', '#'},
    {'#', '#', '#', '#', '#', '#', '#', '#'}
};

enum DIRECTION {
    NORTH,
    EAST,
    SOUTH,
    WEST
};

// SDL2 variables
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Event event;

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("GridWorld", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 480, 470, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        std::cout << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        std::cout << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    return true;
}

bool LoadAndPlayMusic(const char* filename) {
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1;
    Mix_Music* music = Mix_LoadMUS(filename);
    if (!music) {
        std::cerr << "Failed to load music: " << Mix_GetError() << std::endl;
        return false;
    }
    
    Mix_PlayMusic(music, -1); 
    Mix_VolumeMusic(2);
    return true;
}

void close() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void EndProgram() { // Sets EndGame to True to end the loop and exit the program
    endGame = true;
}

void Quit() { // Sends Quit Message and calls EndProgram()
    std::cout << "Thanks For Playing.....fake gamer. ;O;" << std::endl;
    EndProgram();
}

void Dies() { // Prints DeathMessage and calls EndProgram()
    std::cout << "You've fallen down a pit. YOU HAVE DIED!\nIMAGINE DYING! XD LOL\nThanks for playing. Maybe next time BOZO!" << std::endl;
    EndProgram();
}

void FoundChest() { // Prints GoldChest Message and calls EndProgram()
    std::cout << "WOW - you've discovered a large chest filled with GOLD coins!\nTOO BAD IT'S NOT REAL... BUT HEY! AT LEAST YOU WIN!\nThanks for playing. There probably won’t be a next time." << std::endl;
    EndProgram();
}

void printWalkIntoWallMsg() {
    int index = rand() % walkIntoWallMsg.size();
    std::cout << walkIntoWallMsg[index] << std::endl;
    std::cout << std::endl;
}

void Move(DIRECTION direction) { // Moves Player and Checks if player is standing on a D or a G
    int prevRow = playerPos[0];
    int prevCol = playerPos[1];
    switch (direction) {
    case DIRECTION::NORTH:
        if (myArray[prevRow - 1][prevCol] == '#') {
            printWalkIntoWallMsg();
        }
        else {
            playerPos[0] = prevRow - 1;
        }
        break;
    case DIRECTION::SOUTH:
        if (myArray[prevRow + 1][prevCol] == '#') {
            printWalkIntoWallMsg();
        }
        else {
            playerPos[0] = prevRow + 1;
        }
        break;
    case DIRECTION::EAST:
        if (myArray[prevRow][prevCol + 1] == '#') {
            printWalkIntoWallMsg();
        }
        else {
            playerPos[1] = prevCol + 1;
        }
        break;
    case DIRECTION::WEST:
        if (myArray[prevRow][prevCol - 1] == '#') {
            printWalkIntoWallMsg();
        }
        else {
            playerPos[1] = prevCol - 1;
        }
        break;
    default:
        break;
    }

    char currentLocation = myArray[playerPos[0]][playerPos[1]];

    // Check if move kills player
    switch (currentLocation) {
    case 'G':
        FoundChest();
        break;
    case 'D':
        Dies();
        break;
    default:
        break;
    }
}

void UserCommand(SDL_Keycode key) {
    userInput = tolower(key);

    // WASD Command logic
    if (userInput == 'q' || userInput == KEY_ESC) {
        Quit();
    }
    else if (userInput == 'w') {
        Move(DIRECTION::NORTH);
    }
    else if (userInput == 'd') {
        Move(DIRECTION::EAST);
    }
    else if (userInput == 's') {
        Move(DIRECTION::SOUTH);
    }
    else if (userInput == 'a') {
        Move(DIRECTION::WEST);
    }
    else {
        std::cout << "Not a valid input. Try Again.\n:> ";
    }
}

void renderGrid() {
    // Clear screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background
    SDL_RenderClear(renderer);

    // Render grid
    int cellSize = 60;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            SDL_Rect cell = { j * cellSize, i * cellSize, cellSize, cellSize };
            if (myArray[i][j] == '#') {
                SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255); // White walls
                SDL_RenderFillRect(renderer, &cell);
            }
            if (seeDeathTiles) {
                if (myArray[i][j] == 'D') {
                    SDL_SetRenderDrawColor(renderer, 204, 0, 0, 255); // Dark red for deathTiles
                    SDL_RenderFillRect(renderer, &cell);
                }
            }
            if (myArray[i][j] == 'G') {
                SDL_SetRenderDrawColor(renderer, 20, 240, 0, 255); // Green Exit
                SDL_RenderFillRect(renderer, &cell); 
            }

            // Draw player
            if (i == playerPos[0] && j == playerPos[1]) {
                SDL_SetRenderDrawColor(renderer, 234, 250, 5, 255); // Yellow player
                SDL_RenderFillRect(renderer, &cell);
            }
        }
    }

    SDL_RenderPresent(renderer);
}

void GameLoop(std::atomic<bool>& isRunning) {
    SDL_Event e;

    while (isRunning && !endGame) {
        // Handle events
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                isRunning = false;
            }
            else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                case SDLK_w:
                    Move(DIRECTION::NORTH);
                    break;
                case SDLK_a:
                    Move(DIRECTION::WEST);
                    break;
                case SDLK_s:
                    Move(DIRECTION::SOUTH);
                    break;
                case SDLK_d:
                    Move(DIRECTION::EAST);
                    break;
                case SDLK_q:
                    isRunning = false; // Quit game
                    break;
                }
            }
        }

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background
        SDL_RenderClear(renderer);

        // Draw the grid
        renderGrid();

        // Show what was drawn
        SDL_RenderPresent(renderer);

        SDL_Delay(16); // Cap frame rate at ~60 FPS
    }
}


int main() {
    std::atomic<bool> isRunning(true);

    if (!init()) {
        std::cout << "Failed to initialize SDL2!" << std::endl;
        return -1;
    }

    if (!LoadAndPlayMusic("BGmusic.mp3")) {
        std::cout << "Failed to initialize Music!" << std::endl;
        return -1;
    }

    GameLoop(isRunning); // Run the game loop on the main thread

    close();
    return 0;
}

