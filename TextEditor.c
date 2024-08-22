#include <windows.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_ttf.h>
#include <time.h>

#define DEFAULT_FONT "C:/Windows/Fonts/arial.ttf"
#define FPS 60
#define FRAME_DURATION (1000/FPS)

typedef struct {
    SDL_Renderer* renderer;
    SDL_Window* window;
} App;

App app = {0};

char* currentFileText = NULL;
int textFileSize = 0;
SDL_Texture* curTextTexture = 0;
SDL_Rect curTextRect;
SDL_Color mainTextColor = { 255, 255, 0, 255 };

TTF_Font* font;

char specialKeyPressed = 0;

int textSelectedSize = 0;

char loc[256];


SDL_Texture* PrepareText(SDL_Rect* textRect, TTF_Font* font, SDL_Color textColor, const char* text, int x, int y) {

    SDL_Surface* textSurface = TTF_RenderText_Solid_Wrapped(font, text, textColor, 1270);


    SDL_Texture *textTexture = SDL_CreateTextureFromSurface(app.renderer, textSurface);
    if (!textTexture) {
        printf("Failed to create text texture: %s\n", SDL_GetError());
        exit(1);
    }

    *textRect = (SDL_Rect){ x, y, textSurface->w, textSurface->h };

    SDL_FreeSurface(textSurface);
    
    return textTexture;
}

void LoadFile(char** retFile, int* size) {
    OPENFILENAMEW ofn;
    wchar_t filePath[1024] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = sizeof(filePath) / sizeof(wchar_t);
    ofn.lpstrFilter = L"Text Files\0*.txt\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = L"C:\\";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;

    filePath[0] = L'\0';

    if (GetOpenFileNameW(&ofn)) {
        wprintf(L"Selected file (full path): %ls\n", ofn.lpstrFile);
        wprintf(L"File path length: %zu\n", wcslen(ofn.lpstrFile));

        FILE* fptr = _wfopen(ofn.lpstrFile, L"r");
        if (!fptr) {
            printf("Error opening file. errno: %d\n", errno);
            perror("Error details");
            return;
        }

        // Get file size
        fseek(fptr, 0L, SEEK_END);
        long sz = ftell(fptr);
        fseek(fptr, 0L, SEEK_SET);

        *retFile = (char*)malloc((sz + 1) * sizeof(char));
        if (*retFile == NULL) {
            perror("Error: Memory allocation failed");
            fclose(fptr);
            return;
        }

        fread(*retFile, 1, sz, fptr);
        (*retFile)[sz] = '\0';
        *size = sz;

        fclose(fptr);
        printf("File loaded successfully. Size: %d bytes\n", *size);
    }
    else {
        DWORD error = CommDlgExtendedError();
        if (error) {
            printf("Error: Open file dialog failed with code %lu\n", error);
        }
        else {
            printf("Open file dialog canceled.\n");
        }
    }
}


void SaveFile() {
    FILE* fptr = fopen(loc, "w");
    if (!fptr) {
        printf("Error: Could not open file\n");
        exit(1);
    }

    fprintf(fptr, currentFileText);

    printf("Saved");

    fclose(fptr);
}

TTF_Font* LoadFont(int fontSize) {
    TTF_Font* font = TTF_OpenFont(DEFAULT_FONT, fontSize);
    if (!font) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return NULL;
    }
    return font;
}

void HandleBackspace() {
    if (textFileSize > 0) {
        if (textSelectedSize > 0) {

        }
        else {
            currentFileText[--textFileSize] = '\0';
            curTextTexture = PrepareText(&curTextRect, font, mainTextColor, currentFileText, 10, 40);
        }

    }
}

void HandleEnter() {
    if (currentFileText != NULL) {
        currentFileText[textFileSize++] = '\n';
        currentFileText[textFileSize] = '\0';

        curTextTexture = PrepareText(&curTextRect, font, mainTextColor, currentFileText, 10, 40);
    }
}

void HandleCtrl() {
    specialKeyPressed = '1';
}

void HandleS() {
    if (specialKeyPressed == '1') {
        SaveFile();
    }
}



typedef void (*KeyHandler)();
KeyHandler keyHandlers[SDL_NUM_SCANCODES] = { NULL };

void InitializeKeyHandlers() {
    keyHandlers[SDL_SCANCODE_BACKSPACE] = HandleBackspace;
    keyHandlers[SDL_SCANCODE_RETURN] = HandleEnter;
    keyHandlers[SDL_SCANCODE_LCTRL] = HandleCtrl;
    keyHandlers[SDL_SCANCODE_S] = HandleS;
}


int main()
{
    InitializeKeyHandlers();
    int renderFlags, windowFlags;

    renderFlags = SDL_RENDERER_ACCELERATED;

    windowFlags = SDL_WINDOW_SHOWN;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Couldn't initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    if (TTF_Init() < 0) {
        printf("SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        exit(1);
    };

    font = LoadFont(12);

    app.window = SDL_CreateWindow("TextEditor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, windowFlags);

    if (!app.window)
    {
        printf("Failed to open %d x %d window: %s\n", 1280, 720, SDL_GetError());
        exit(1);
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    app.renderer = SDL_CreateRenderer(app.window, -1, renderFlags);

    if (!app.renderer) {
        printf("Failed to create renderer: %s\n", SDL_GetError());
        exit(1);
    }

    int running = 1;

    int drawDropDown = 0;

    SDL_Event event;


    SDL_Texture* fileTextTexture = 0;
    SDL_Rect fileTextRect;
    SDL_Color textColor = { 0, 0, 0, 255 };
    const char* fileText = "File";

    fileTextTexture = PrepareText(&fileTextRect, font, textColor, fileText, 10, 10);

    SDL_Texture* openFileTextTexture = 0;
    SDL_Rect openFileTextRect;
    const char* openFileText = "Open";

    openFileTextTexture = PrepareText(&openFileTextRect, font, textColor, openFileText, 10, 40);

    SDL_Texture* saveFileTextTexture = 0;
    SDL_Rect saveFileTextRect;
    const char* saveFileText = "Save";

    saveFileTextTexture = PrepareText(&saveFileTextRect, font, textColor, saveFileText, 10, 80);



    clock_t lastFrameTime = clock();
    while (running == 1)
    {
        clock_t frameStart = clock();
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
            else if (event.type == SDL_KEYDOWN) {
                SDL_Scancode scancode = event.key.keysym.scancode;
                if (keyHandlers[scancode] != NULL) {
                    keyHandlers[scancode]();
                }
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x = event.button.x;
                int y = event.button.y;


                if (x >= fileTextRect.x && x <= fileTextRect.x + fileTextRect.w && y >= fileTextRect.y && y <= fileTextRect.y + fileTextRect.h) {
 
                    drawDropDown = 1;
                }
                else if (x >= openFileTextRect.x && x <= openFileTextRect.x + openFileTextRect.w && y >= openFileTextRect.y && y <= openFileTextRect.y + openFileTextRect.h) {
                    drawDropDown = 0;
                    LoadFile(&currentFileText, &textFileSize);
                    curTextTexture = PrepareText(&curTextRect, font, mainTextColor, currentFileText, 10, 40);
                }
                else if (x >= saveFileTextRect.x && x <= saveFileTextRect.x + saveFileTextRect.w && y >= saveFileTextRect.y && y <= saveFileTextRect.y + saveFileTextRect.h) {
                    SaveFile();
                }
            }
            else if (event.type == SDL_TEXTINPUT && currentFileText != NULL) {
                strcat(currentFileText, event.text.text);
                textFileSize += strlen(event.text.text); 
                curTextTexture = PrepareText(&curTextRect, font, mainTextColor, currentFileText, 10, 40);
            }
        }

        SDL_RenderClear(app.renderer);

        SDL_SetRenderDrawColor(app.renderer, 255, 255, 255, 255);
        SDL_Rect menuBar = { 0, 0, 1280, 30 };
        SDL_RenderDrawRect(app.renderer, &menuBar);
        SDL_RenderFillRect(app.renderer, &menuBar);

        if (drawDropDown == 1) {
            SDL_Rect fileDropDown = { 0, fileTextRect.y + 10, 50, 100 };

            SDL_RenderDrawRect(app.renderer, &fileDropDown);
            SDL_RenderFillRect(app.renderer, &fileDropDown);

            SDL_RenderCopy(app.renderer, openFileTextTexture, NULL, &openFileTextRect);
            if (textFileSize > 0) {
                SDL_RenderCopy(app.renderer, saveFileTextTexture, NULL, &saveFileTextRect);
            }

        }


        SDL_RenderCopy(app.renderer, fileTextTexture, NULL, &fileTextRect);


        if (currentFileText) {
            SDL_RenderCopy(app.renderer, curTextTexture, NULL, &curTextRect);
        }

        SDL_RenderPresent(app.renderer);

        SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, 255);

        clock_t frameEnd = clock();
        int frameDuration = (int)(((frameEnd - frameStart) * 1000) / CLOCKS_PER_SEC);
        if (frameDuration < FRAME_DURATION) {
            SDL_Delay(FRAME_DURATION - frameDuration);
        }

    }

    return 0;
}

