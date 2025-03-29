#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <iostream>
#include <atomic>
#include <mutex>
// Cheat
bool seeDeathTiles = false;
// Game State
std::mutex mtx;
std::atomic<bool> isRunning(true);
bool endGame = false;
bool win = false;
int playerPos[2] = { 6, 3 };
const int row = 8;
const int col = 8;



char myArray[row][col] = {
    {'#', '#', '#', '#', '#', '#', '#', '#'},
    {'#', '#', ' ', 'D', 'G', 'D', ' ', '#'},
    {'#', ' ', ' ', 'D', ' ', ' ', ' ', '#'},
    {'#', 'D', ' ', ' ', ' ', 'D', ' ', '#'},
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
// SDL2 Mixer
Mix_Music* backgroundMusic = nullptr;
Mix_Chunk* moveSound = nullptr;
Mix_Chunk* wallSound = nullptr;
// SDL2 TTF
TTF_Font* textFont = nullptr;
SDL_Texture* titleTexture = nullptr;
SDL_Texture* textTexture = nullptr;
SDL_Texture* textTexture2 = nullptr;
SDL_Texture* textTexture3 = nullptr;
SDL_Texture* optionsTexture = nullptr;
// Icon
SDL_Surface* icon = nullptr;

bool Init() {

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer could not initialize! Mix_Error: " << Mix_GetError() << std::endl;
        return 1;
    }
    window = SDL_CreateWindow("GridWorld", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 520, 520, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }


    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    if (TTF_Init() == -1) {
        std::cerr << "TTF_Init Error: " << TTF_GetError() << std::endl;
        return false;
    }

    icon = SDL_LoadBMP("icon.bmp");
    if (icon == nullptr) {
        std::cerr << "Icon Error: " << TTF_GetError() << std::endl;
        return false;
    }
    else {
        SDL_SetWindowIcon(window, icon);
        SDL_FreeSurface(icon);
    }
    
    SDL_SetWindowTitle(window, "SDL2 GridWorld by Jason D'Souza");
    return true;
}

bool LoadAndPlayMusic(Mix_Music*& backgroundMusic) {
    backgroundMusic = Mix_LoadMUS("background.mp3");

    if (!backgroundMusic) {
        std::cerr << "Failed to load music: " << Mix_GetError() << std::endl;
        return false;
    }

    Mix_PlayMusic(backgroundMusic, -1);
    Mix_VolumeMusic(5);
    return true;
}

void StopMusic(Mix_Music*& backgroundMusic) {
    // Stop and free the music
    Mix_HaltMusic();
    Mix_FreeMusic(backgroundMusic);
    Mix_CloseAudio();
}

bool LoadSoundEffect(Mix_Chunk*& moveSound, Mix_Chunk*& wallSound) {
    moveSound = Mix_LoadWAV("move.wav");
    wallSound = Mix_LoadWAV("wall.wav");
    if (!moveSound || !wallSound) {
        std::cerr << "Failed to load sound effects! Mix_Error: " << Mix_GetError() << std::endl;
        return false;
    }
    return true;
}

void PlayMoveSoundEffect(Mix_Chunk*& move) {
    Mix_PlayChannel(0, move, 0);
    Mix_Volume(0, 40);
}

void PlayWallSoundEffect(Mix_Chunk*& wall) {
    Mix_PlayChannel(0, wall, 0);
    Mix_Volume(0, 70);
}

void StopSounds(Mix_Chunk*& moveSound, Mix_Chunk*& wallSound) {
    Mix_HaltChannel(0);
    Mix_FreeChunk(moveSound);
    Mix_FreeChunk(wallSound);
}

void EndGame(bool& endgame) {
    endgame = true;
}

void Dies() {
    EndGame(endGame);
}

void FoundChest(bool& win) { 
    win = true;
    EndGame(endGame);
}

void Move(DIRECTION direction) { // Moves Player and Checks if player is standing on a D or a G
    int prevRow = playerPos[0];
    int prevCol = playerPos[1];
    switch (direction) {
    case DIRECTION::NORTH:
        if (myArray[prevRow - 1][prevCol] == '#'){
            PlayWallSoundEffect(wallSound);
        }
        else {
            PlayMoveSoundEffect(moveSound);
            playerPos[0] = prevRow - 1;
        }
        break;
    case DIRECTION::SOUTH:
        if (myArray[prevRow + 1][prevCol] == '#') {
            PlayWallSoundEffect(wallSound);
        }
        else {
            PlayMoveSoundEffect(moveSound);
            playerPos[0] = prevRow + 1;
        }
        break;
    case DIRECTION::EAST:
        if (myArray[prevRow][prevCol + 1] == '#') {
            PlayWallSoundEffect(wallSound);
        }
        else {
            PlayMoveSoundEffect(moveSound);
            playerPos[1] = prevCol + 1;
        }
        break;
    case DIRECTION::WEST:
        if (myArray[prevRow][prevCol - 1] == '#') {
            PlayWallSoundEffect(wallSound);
        }
        else {
            PlayMoveSoundEffect(moveSound);
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
        FoundChest(win);
        break;
    case 'D':
        Dies();
        break;
    default:
        break;
    }
}

void RenderGrid() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    int cellSize = 65;
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            SDL_Rect cell = { j * cellSize, i * cellSize, cellSize, cellSize };
            if (myArray[i][j] == '#') {
                SDL_SetRenderDrawColor(renderer, 163, 163, 163, 255); // grey walls
                SDL_RenderFillRect(renderer, &cell);
            }
            if (seeDeathTiles) {
                if (myArray[i][j] == 'D') {
                    SDL_SetRenderDrawColor(renderer, 139, 0, 0, 255); // Dark red for deathTiles
                    SDL_RenderFillRect(renderer, &cell);
                }
            }
            if (myArray[i][j] == 'G') {
                SDL_SetRenderDrawColor(renderer, 234, 250, 5, 255); // Yellow Exit
                SDL_RenderFillRect(renderer, &cell); 
            }

            // Draw player
            if (i == playerPos[0] && j == playerPos[1]) {
                SDL_SetRenderDrawColor(renderer, 20, 240, 0, 255); // Green player
                SDL_RenderFillRect(renderer, &cell);
            }
        }
    }
    SDL_RenderPresent(renderer);
}

SDL_Texture* RenderText(TTF_Font* font, SDL_Texture* texture, const int size, const char* text, int r, int g, int b, int yPos) {
    font = TTF_OpenFont("font/MegamaxJonathanToo.ttf", size);
    if (!font) {
        std::cerr << "Failed to load '" << font << "' Text font!TTF_Error: " << TTF_GetError() << std::endl;
    }
    SDL_Color TextColor = { r, g, b, 255 };
    SDL_Surface* Surface = TTF_RenderText_Solid(font, text, TextColor);
    texture = SDL_CreateTextureFromSurface(renderer, Surface);
    int Width = Surface->w;
    int Height = Surface->h;
    SDL_FreeSurface(Surface);
    SDL_Rect Rect = { (520 - Width) / 2, yPos, Width, Height };
    SDL_RenderCopy(renderer, texture, nullptr, &Rect);
    TTF_CloseFont(font);
    return texture;
}

void StopTTF() {
    SDL_DestroyTexture(titleTexture);
    SDL_DestroyTexture(textTexture);
    SDL_DestroyTexture(textTexture2);
    SDL_DestroyTexture(textTexture3);
    SDL_DestroyTexture(optionsTexture);
    TTF_CloseFont(textFont);
}

void RenderGameEnd(bool& win) {
    SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
    SDL_RenderClear(renderer);
    if (win)
    {
        titleTexture = RenderText(textFont, titleTexture, 60, "You Win!", 8, 138, 28, 50);
        textTexture = RenderText(textFont, textTexture, 20, "WOW - a chest of GOLD coins!", 217, 255, 0, 200);
        textTexture2 = RenderText(textFont, textTexture, 20, "Too bad it's fake... but you win!", 217, 255, 0, 230);
        textTexture3 = RenderText(textFont, textTexture, 18, "Thanks for playing! Till next time.", 217, 255, 0, 260);
    }
    else {
        titleTexture = RenderText(textFont, titleTexture, 70, "You Lose!", 209, 0, 0, 50);
        textTexture = RenderText(textFont, textTexture, 18, "You've fallen down a pit. YOU DIED!", 209, 0, 0, 200);
        textTexture2 = RenderText(textFont, textTexture, 24, "IMAGINE DYING! XD LOL", 209, 0, 0, 230);
        textTexture3 = RenderText(textFont, textTexture, 24, "Maybe next time BOZO!", 209, 0, 0, 260);
    }
    optionsTexture = RenderText(textFont, optionsTexture, 24, "ESC to Quit     R to Retry", 8, 138, 28, 450);
    SDL_RenderPresent(renderer);
    StopTTF();
}

void InGameState(SDL_Event& e, bool& endGame) {
    while (SDL_PollEvent(&e) != 0 && !endGame) {
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
            case SDLK_ESCAPE:
                isRunning = false;
                break;
            }
        }
    }
    RenderGrid();
    SDL_Delay(33);
}

void EndGameState(SDL_Event& e, bool& endGame, bool& win) {
    Mix_PauseMusic();
    RenderGameEnd(win);

    while (SDL_PollEvent(&e) != 0 && endGame) {
        if (e.type == SDL_QUIT) {
            isRunning = false;
        }
        else if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
            case SDLK_r:
                playerPos[0] = 6;
                playerPos[1] = 3;
                win = false; 
                endGame = false;
                Mix_ResumeMusic();
                break;
            case SDLK_ESCAPE:
                isRunning = false;
                break;
            }
        }
    }
}

void Close() {
    StopMusic(backgroundMusic);
    StopTTF();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

void GameLoop(std::atomic<bool>& isRunning, bool& endgame, bool& win) {
    SDL_Event e;
    while (isRunning) {
        if (!endGame) {
            InGameState(e, endgame);
        }
        else {
            EndGameState(e, endGame, win);
        }
    }
}

int SDL_main(int argc, char* argv[]) {

    if (!Init()) {
        std::cerr << "Failed to initialize SDL2!" << std::endl;
        return -1;
    }

    if (!LoadAndPlayMusic(backgroundMusic)) {
        std::cerr << "Failed to initialize Background Music!" << std::endl;
        return -1;
    }

    if (!LoadSoundEffect(moveSound, wallSound)) {
        std::cerr << "Failed to initialize Sound Effects!" << std::endl;
        return -1;
    }

    GameLoop(isRunning, endGame, win); // Run the game loop on the main thread

    Close();
    return 0;
}

