#include <Windows.h>
#include <Wincon.h>
#include <iostream>
#include <cmath>
#include <wingdi.h>

//I don't know if this is the right way to do a struct, seems wrong since all of
// my members (is that what we call them?) are pointers.

//TODO use these vectors instead of the ones that I have in the player struct
struct posVec {
    float x, y, z;
};

struct aimVec {
    float x, y;
};

typedef struct {
    int *health;
    float *fPosX;
    float *fPosY;
    float *fPosZ;
    float *fAimX;
    float *fAimY;
    float *PlayerDistance;
    int *team;
    char **PlayerName;
} EntityData;

// pretty simple function to calculate distance between two three dimensional points
float distance3D(float pX, float pY, float pZ, float eX, float eY, float eZ) {
    return sqrt(pow(pX - eX, 2.0) + pow(pY - eY, 2.0) + pow(pZ - eZ, 2.0));
}

// If I have done this correctly it will take an address (base) and apply offsets
// provided in a list (offsets[]) to it until it has reached the end. At which point
// all you have to do is dereference and cast the value to edit it in memory. I could
// be wrong though since I haven't done much work in c++
uintptr_t addressFinder(uintptr_t *base, uintptr_t offsets[]) {
    int num_elements = sizeof(offsets) / sizeof(offsets[0]);
    uintptr_t address = *base;
    for (int i = 0; i < num_elements; i++) {
        address = (address + offsets[i]);
    }
    return address;
}


struct aimVec
aimbot(struct posVec my, struct aimVec aim, struct posVec enemy, float fovAllow, float *minDist, bool *found) {
    //TODO make the rest of this oo
    struct aimVec cloAngle;
    cloAngle.x = 0.0;
    cloAngle.y = 0.0;

    float enDist = distance3D(my.x, my.y, my.z, enemy.x, enemy.y, enemy.z);
    float angleX = (-(float) atan2(enemy.x - my.x, enemy.y - my.y)) / 3.14159265358979323846 * 180.0f + 180.0f;
    float angleY = (atan2(enemy.z - my.z, enDist)) * 180.0f / 3.14159265358979323846;
    if (!(abs(angleX - aim.x) > fovAllow || abs(angleY - aim.y) > fovAllow)) {


        if (enDist < *minDist) {
            *minDist = enDist;
            *found = true;
            cloAngle.x = angleX;
            cloAngle.y = angleY;
        }
    }
    return cloAngle;
}

DWORD WINAPI hackthread(LPVOID param) {
//for debugging
    AllocConsole();
    AttachConsole(GetProcessId(GetCurrentProcess()));
    freopen("CONOUT$", "w", stdout);
    HANDLE han = GetStdHandle(STD_OUTPUT_HANDLE); //this doesn't work with minGW compiler
    printf_s("----------------------------------------------------------------------\n");
    printf_s("                kiyip's first internal aimbot\n");
    printf_s("          right click or left alt activates aimbot\n");
    printf_s("                         f3 to quit\n");
    printf_s("----------------------------------------------------------------------\n");

//-------------

    uintptr_t *localPlayerAddress = (uintptr_t *) (0x50F4F4);
    uintptr_t *entArrayBaseAdd = (uintptr_t *) 0x0050F4F8; //this is the pointer to the list of pointers that points to everyone entity in the GAME, what an ass sentence
    uintptr_t entSize = 0x4;

//player offsets
    uintptr_t xMoOffs[] = {0x40};
    uintptr_t yMoOffs[] = {0x44};
    uintptr_t xPosOffs[] = {0x34};
    uintptr_t yPosOffs[] = {0x38};
    uintptr_t zPosOffs[] = {0x3c};
    uintptr_t nameOffs[] = {0x225};
    uintptr_t teamOffs[] = {0x032c}; // 0 red 1 blue
    uintptr_t healthOffs[] = {0xf8};
    uintptr_t currEnOffs[] = {0x4};
    uintptr_t inGameOffs[] = {0x70}; //should be one I think

    EntityData my;

    uintptr_t pLocZAdd, pLocXAdd, pLocYAdd, aimXAdd, aimYAdd, myTeamAdd = 0;  // where we are standing
    uintptr_t entHealthAdd, entNameAdd, entTeamAdd, entZAdd, entXAdd, entYAdd = 0;
    uintptr_t *currentEntAdd = 0;  // this will use the same offsets as plyZ, but the baseAddress of who we are targeting
    float angleX, angleY, cloAngleX, cloAngleY = 0.0;
    uintptr_t smoothNum = 30;


    my.health = (int *) addressFinder(localPlayerAddress, healthOffs);
    my.fPosX = (float *) addressFinder(localPlayerAddress, xPosOffs);
    my.fPosY = (float *) addressFinder(localPlayerAddress, yPosOffs);
    my.fPosZ = (float *) addressFinder(localPlayerAddress, zPosOffs);
    my.fAimX = (float *) addressFinder(localPlayerAddress, xMoOffs);
    my.fAimY = (float *) addressFinder(localPlayerAddress, yMoOffs);
    my.team = (int *) addressFinder(localPlayerAddress, teamOffs);

    aimXAdd = addressFinder(localPlayerAddress, xMoOffs);
    aimYAdd = addressFinder(localPlayerAddress, yMoOffs);

    uintptr_t fovAllow = 20; //degrees of variance
    uintptr_t distAllow = 20;

//    if using CLion minGW debugger, or visual studio and you want to exit the cheat while in game change value of
//    debug to 0; press f6
    BOOL debug = 1;

    int i = 1;
    float dist = 0.0f;
    float minDist = 99999999.0;
    bool aimToggle, inAimFov, found = false;
    int closestEntNum = 1;//typically this should start at one
    float enDist;
    EntityData enemy = my;
    currEnOffs[0] = entSize * i;
    int *numOfPlayers = (int *) (0x50f500);

    while (!GetAsyncKeyState(VK_F3) ||
           !debug) //find out a variable to make it so this doesn't crash the program in single player mode
    {
        i = 1;
        dist = 0.0f;
        minDist = 99999999.0;
        aimToggle, inAimFov, found = false;
        closestEntNum = 1;//typically this should start at zero
        enDist = 0;

        //TODO make a mutable list that stores
        //If aim key is held
        if (GetAsyncKeyState(VK_RBUTTON) || GetAsyncKeyState(VK_LCONTROL)) {
            while (i < *numOfPlayers &&
                   !GetAsyncKeyState(VK_F3)) {//I know you shouldn't need the async, but better than an infinite loop
                //TODO: find the closest entity
                currEnOffs[0] = entSize * i;
                currentEntAdd = (uintptr_t *) addressFinder(entArrayBaseAdd, currEnOffs);
                int real = (int) *(uintptr_t *) addressFinder(currentEntAdd, inGameOffs);
                if (real != 1) { //checks if a player is ingame vs speccing
                    enemy.health = (int *) addressFinder(currentEntAdd, healthOffs);
                    if ((*enemy.health > 0 && *enemy.health <= 100)) { //checks if an enemy is alive
                        enemy.team = (int *) addressFinder(currentEntAdd, teamOffs);
                        if (*enemy.team != *my.team &&
                            (*enemy.team == 0 || *enemy.team == 1)) { //checks if the enemy is on your team

                            struct posVec myPos;
                            myPos.x = *my.fPosX;
                            myPos.y = *my.fPosY;
                            myPos.z = *my.fPosZ;

                            struct aimVec aim;
                            aim.x = *my.fAimX;
                            aim.y = *my.fAimY;

                            struct posVec enemyPos;
                            enemyPos.x = *(float *) addressFinder(currentEntAdd, xPosOffs);
                            enemyPos.y = *(float *) addressFinder(currentEntAdd, yPosOffs);
                            enemyPos.z = *(float *) addressFinder(currentEntAdd, zPosOffs);

                            struct aimVec angles = aimbot(myPos, aim, enemyPos, fovAllow, &minDist, &found);
                            if (found) {
                                cloAngleX = angles.x;
                                cloAngleY = angles.y;
                            }
                        }
                    }
                }
                i++;
            }

            if (found) {
                cloAngleX = *my.fAimX + (cloAngleX - *my.fAimX) / smoothNum;
                *my.fAimX = cloAngleX;

                cloAngleY = *my.fAimY + (cloAngleY - *my.fAimY) / smoothNum;
                *my.fAimY = cloAngleY;
            }
        }

//        currEnOffs[0] = entSize * closestEntNum;
//        currentEntAdd = (uintptr_t *)addressFinder(entArrayBaseAdd, currEnOffs);
//        enemy.health = (int *) addressFinder(currentEntAdd, healthOffs);

//        printf_s("enemy health: %i\n", *enemy.health );
        Sleep(1);
//        system ("CLS");
    }
    *my.health = 99;
    *my.health = 100;
    system("CLS");
    FreeConsole();
    FreeLibraryAndExitThread((HMODULE) param, NULL);

    return NULL;
}

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            CreateThread(0, 0, hackthread, hModule, 0, 0);
            break;

        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}